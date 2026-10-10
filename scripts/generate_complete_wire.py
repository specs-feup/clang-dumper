#!/usr/bin/env python3
"""Generate the native FlatBuffers bindings, enum lookup and schema hash for the AST wire schema."""

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


def generate(flatc: str, output: Path) -> None:
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
        generate_enum_support(generated, files)
        (generated / "FlatSchemaHash.h").write_text(
            "#pragma once\n"
            "namespace clava::flat {\n"
            f'inline constexpr const char *SchemaHash = "{schema_hash(files)}";\n'
            "}\n",
            encoding="utf-8",
        )

        # Only touch changed headers, so unchanged schemas do not trigger rebuilds.
        for source in generated.iterdir():
            target = output / source.name
            if not target.exists() or target.read_bytes() != source.read_bytes():
                shutil.copyfile(source, target)
        (output / "complete.stamp").touch()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flatc", required=True, help="Pinned FlatBuffers compiler")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    generate(args.flatc, args.out)


if __name__ == "__main__":
    main()
