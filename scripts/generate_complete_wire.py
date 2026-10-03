#!/usr/bin/env python3
"""Generate the native FlatBuffers bindings and dispatch for the AST wire schema."""

from __future__ import annotations

import argparse
import hashlib
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCHEMA_DIR = ROOT / "wire" / "v2"


def schema_files() -> list[Path]:
    return sorted(SCHEMA_DIR.glob("*.fbs"))


def schema_hash(files: list[Path]) -> str:
    """Hash sorted schema paths and bytes using the published bundle contract."""
    digest = hashlib.sha256()
    for path in files:
        relative_path = path.relative_to(ROOT).as_posix().encode("utf-8")
        digest.update(relative_path + b"\0" + path.read_bytes() + b"\0")
    return digest.hexdigest()


def pinned_flatbuffers_version() -> str:
    for line in (ROOT / "flatbuffers-version.env").read_text(encoding="utf-8").splitlines():
        if line.startswith("FLATBUFFERS_VERSION="):
            return line.partition("=")[2]
    raise RuntimeError("flatbuffers-version.env has no FLATBUFFERS_VERSION")


def verify_flatc(flatc: str) -> None:
    result = subprocess.run(
        [flatc, "--version"], check=True, text=True, capture_output=True
    )
    expected = pinned_flatbuffers_version()
    if result.stdout.strip() != f"flatc version {expected}":
        raise RuntimeError(
            f"Expected flatc {expected}, got {result.stdout.strip()!r}"
        )


def generate_handler_declarations(destination: Path) -> None:
    declarations: set[str] = set()
    for path in sorted((ROOT / "src" / "Clava").glob("Flat*.cpp")):
        source = path.read_text(encoding="utf-8")
        declarations.update(
            match.group(1) + ";"
            for match in re.finditer(
                r"(std::unique_ptr<fb::\w+T>\s+make\w+\([^{};]+\))\s*\{",
                source,
            )
        )
    (destination / "FlatHandlerDeclarations.inc").write_text(
        "\n".join(sorted(declarations)) + "\n", encoding="utf-8"
    )


def generate_enum_support(destination: Path, files: list[Path]) -> None:
    names: set[str] = set()
    for path in files:
        names.update(
            re.findall(r"\benum\s+([A-Za-z_]\w*)\s*:", path.read_text(encoding="utf-8"))
        )

    lines = [
        "#pragma once",
        '#include "enums_generated.h"',
        "#include <string>",
        "#include <string_view>",
        "#include <stdexcept>",
        "namespace clava::flat {",
        "template<class E> struct EnumNames;",
    ]
    for name in sorted(names):
        lines.append(
            f"template<> struct EnumNames<astwire::v2::{name}> {{ "
            f"static const auto &values() {{ return astwire::v2::EnumValues{name}(); }} "
            f"static const char *name(astwire::v2::{name} value) "
            f"{{ return astwire::v2::EnumName{name}(value); }} }};"
        )
    lines.extend(
        [
            "inline std::string enumToken(std::string_view value) {",
            "  std::string result;",
            "  for (unsigned char character : value) {",
            "    if (character == '_' || character == '-') continue;",
            "    result += static_cast<char>(character >= 'A' && character <= 'Z' "
            "? character + ('a' - 'A') : character);",
            "  }",
            "  return result;",
            "}",
            "template<class E> E enumValue(std::string_view name) {",
            "  for (auto value : EnumNames<E>::values()) {",
            "    if (name == EnumNames<E>::name(value)) return value;",
            "  }",
            "  const auto token = enumToken(name);",
            "  for (auto value : EnumNames<E>::values()) {",
            "    if (token == enumToken(EnumNames<E>::name(value))) return value;",
            "  }",
            '  throw std::invalid_argument("Unknown wire enum: " + std::string(name));',
            "}",
            "}",
        ]
    )
    (destination / "FlatEnumSupport.h").write_text("\n".join(lines) + "\n", encoding="utf-8")


def generate_dispatch(destination: Path) -> None:
    dispatch: list[str] = []
    families = (
        ("DECL", "Decl", "Decls", "Decl"),
        ("STMT", "Stmt", "Stmts", "Stmt"),
        ("EXPR", "Expr", "Stmts", "Expr"),
        ("TYPE", "Type", "Types", "Type"),
        ("ATTR", "Attr", "Attrs", "Attribute"),
    )
    for family, clang_type, source_name, default in families:
        path = ROOT / "src" / "ClavaDataDumper" / f"ClavaDataDumper{source_name}.cpp"
        source = path.read_text(encoding="utf-8")
        body = source[source.index(f"::{family}_DATA_DUMPERS =") :]
        body = body[: body.index("};")]
        entries = re.findall(
            rf"{family}_DATA_ENTRY(_AS)?\((\w+)(?:,\s*(\w+))?\)", body
        )
        dispatch.append(
            f"fb::NodeT makeNode(const clang::{clang_type} *node, Context &c) {{ "
            "fb::NodeT out; out.id=wireId(clava::getId(node,c.id)); "
            "out.class_name=clava::getClassName(node);"
        )
        for _alias, class_name, section in entries:
            section = section or class_name
            dispatch.append(
                f'if(out.class_name=="{class_name}"){{'
                f"out.payload.Set(std::move(*make{section}Data("
                f"static_cast<const clang::{class_name}*>(node),c)));return out;}}"
            )
        dispatch.append(
            f"out.payload.Set(std::move(*make{default}Data(node,c)));return out;}}"
        )
    dispatch.append(
        "fb::NodeT makeNode(const clang::QualType &node, Context &c) {"
        "fb::NodeT out;out.id=wireId(clava::getId(node,c.id));"
        'out.class_name="QualType";'
        "out.payload.Set(std::move(*makeQualTypeData(node,c)));return out;}"
    )
    (destination / "FlatDispatch.inc").write_text(
        "\n".join(dispatch) + "\n", encoding="utf-8"
    )


def generated_inventory(directory: Path) -> str:
    lines = []
    for path in sorted(directory.rglob("*")):
        if path.is_file() and path.name != "complete.stamp":
            relative = path.relative_to(directory).as_posix()
            lines.append(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {relative}")
    return "\n".join(lines) + "\n"


def generate(flatc: str, output: Path, manifest: Path | None, update_manifest: bool) -> None:
    verify_flatc(flatc)
    output.mkdir(parents=True, exist_ok=True)
    files = schema_files()

    with tempfile.TemporaryDirectory(prefix="clang-dumper-wire-") as temporary:
        generated = Path(temporary)
        subprocess.run(
            [
                flatc,
                "--cpp",
                "--scoped-enums",
                "--gen-object-api",
                "-I",
                str(SCHEMA_DIR),
                "-o",
                str(generated),
                *(str(path) for path in files),
            ],
            check=True,
        )
        generate_handler_declarations(generated)
        generate_enum_support(generated, files)
        generate_dispatch(generated)
        schema_digest = schema_hash(files)
        (generated / "FlatSchemaHash.h").write_text(
            "#pragma once\n"
            "namespace clava::flat {\n"
            f'inline constexpr const char *SchemaHash = "{schema_digest}";\n'
            "}\n",
            encoding="utf-8",
        )

        inventory = generated_inventory(generated)
        if manifest is not None:
            if update_manifest:
                manifest.parent.mkdir(parents=True, exist_ok=True)
                manifest.write_text(inventory, encoding="utf-8")
            elif not manifest.is_file() or manifest.read_text(encoding="utf-8") != inventory:
                raise RuntimeError(
                    "Generated FlatBuffers outputs differ from "
                    f"{manifest.relative_to(ROOT) if manifest.is_relative_to(ROOT) else manifest}. "
                    "Review the schema or generator change, then regenerate the inventory."
                )

        for source in generated.iterdir():
            target = output / source.name
            if not target.exists() or target.read_bytes() != source.read_bytes():
                shutil.copyfile(source, target)
        (output / "complete.stamp").touch()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flatc", required=True, help="Pinned FlatBuffers compiler")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument(
        "--manifest",
        type=Path,
        help="Tracked generated-output inventory to verify (or update)",
    )
    parser.add_argument(
        "--update-manifest",
        action="store_true",
        help="Write the generated-output inventory instead of verifying it",
    )
    args = parser.parse_args()
    generate(args.flatc, args.out, args.manifest, args.update_manifest)


if __name__ == "__main__":
    main()
