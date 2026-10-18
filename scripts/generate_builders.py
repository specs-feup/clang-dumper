#!/usr/bin/env python3
"""Generate the payload builders of the AST wire schema from its fields and generation attributes.

For each table FooData declared in a family's schema file (types.fbs, ...), the builder

    std::unique_ptr<fb::FooDataT> makeFooData(const clang::Foo *node, Context &c)

fills the base payload with its own builder and each field from the Clang node. A field's Clang value is, by
convention, ``node->isBar()`` for ``is_bar`` and ``has_bar``, and ``node->getBar()`` otherwise. The value is wrapped
by the field's type: node references (``ref``) become wire ids, enums go through the ``clava::<ENUM_NAME>`` table
(``clang_enum`` names another), lists convert each item, and compound tables use their ``make<Table>`` helper.

Attributes override the convention: ``clang: "<expr>"`` gives the Clang value, ``clang_value: "<expr>"`` the final
field value, and a table marked ``clang_manual`` keeps a hand-written builder.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import tempfile
from pathlib import Path

INTEGER_TYPES = {
    "Byte": "int8_t", "UByte": "uint8_t", "Short": "int16_t", "UShort": "uint16_t",
    "Int": "int32_t", "UInt": "uint32_t", "Long": "int64_t", "ULong": "uint64_t",
}


def schema_json(flatc: str, reflection: Path, bfbs: Path) -> dict:
    with tempfile.TemporaryDirectory() as temporary:
        subprocess.run([flatc, "--json", "--strict-json", "--raw-binary", "-o", temporary, str(reflection), "--",
                        str(bfbs)], check=True)
        return json.loads((Path(temporary) / (bfbs.stem + ".json")).read_text())


def short(name: str) -> str:
    return name.rsplit(".", 1)[-1]


def attributes(item: dict) -> dict[str, str]:
    return {a["key"]: a.get("value", "") for a in item.get("attributes", [])}


def pascal(snake: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in snake.split("_"))


def upper_snake(name: str) -> str:
    return re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", re.sub(r"([A-Z])([A-Z][a-z])", r"\1_\2", name)).upper()


def convention(field: str) -> str:
    """The Clang accessor of a field: is_bar -> isBar(), has_bar -> hasBar(), bar -> getBar()."""
    for prefix in ("is_", "has_"):
        if field.startswith(prefix):
            return f"node->{prefix[:-1]}{pascal(field[len(prefix):])}()"
    return f"node->get{pascal(field)}()"


class Generator:

    def __init__(self, schema: dict):
        self.objects = schema["objects"]
        self.enums = schema["enums"]

    def builder(self, table: dict) -> tuple[str, str]:
        name = short(table["name"])
        clang_class = name[: -len("Data")]
        signature = (f"std::unique_ptr<fb::{name}T> make{name}(const clang::{clang_class} *node, Context &c)")
        lines = [signature + " {", f"  auto out = std::make_unique<fb::{name}T>();"]
        for field in sorted(table["fields"], key=lambda f: f.get("id", 0)):
            if field["type"]["base_type"] == "UType":
                continue
            if field["name"] == "base":
                base = short(self.objects[field["type"]["index"]]["name"])
                lines.append(f"  out->base = make{base}(node, c);")
                continue
            lines.append(self.assignment(name, field))
        lines += ["  return out;", "}"]
        return signature + ";", "\n".join(lines)

    def assignment(self, table: str, field: dict) -> str:
        attrs = attributes(field)
        target = f"out->{field['name']}"
        if "clang_value" in attrs:
            return f"  {target} = {attrs['clang_value']};"
        value = attrs.get("clang") or convention(field["name"])
        kind = field["type"]
        base_type = kind["base_type"]
        if base_type == "Vector":
            item = self.convert("item", kind["element"], kind.get("index", -1), attrs, table, field)
            return f"  for (auto &&item : {value})\n    {target}.push_back({item});"
        return f"  {target} = {self.convert(value, base_type, kind.get('index', -1), attrs, table, field)};"

    def convert(self, value: str, base_type: str, index: int, attrs: dict, table: str, field: dict) -> str:
        if "ref" in attrs:
            return f"clava::getId({value}, c.id)"
        if base_type == "Obj":
            return f"make{short(self.objects[index]['name'])}({value}, c)"
        if index >= 0:
            enum = short(self.enums[index]["name"])
            mapping = attrs.get("clang_enum") or upper_snake(enum)
            return f"enumValue<fb::{enum}>(clava::{mapping}[llvm::to_underlying({value})])"
        if base_type == "Bool":
            return f"static_cast<bool>({value})"
        if base_type in INTEGER_TYPES:
            return f"static_cast<{INTEGER_TYPES[base_type]}>({value})"
        if base_type in ("Float", "Double"):
            return f"static_cast<{'float' if base_type == 'Float' else 'double'}>({value})"
        if base_type == "String":
            return value
        raise SystemExit(f"{table}.{field['name']}: cannot generate a {base_type} field; use clang_value")

    def family(self, schema_file: str) -> str:
        tables = [t for t in self.objects if t.get("declaration_file", "").endswith("/" + schema_file)
                  and short(t["name"]).endswith("Data") and "clang_manual" not in attributes(t)]
        built = [self.builder(t) for t in tables]
        out = [f"// Generated by scripts/generate_builders.py from wire/v2/{schema_file}. Do not edit.", ""]
        out += [declaration for declaration, _ in built] + [""]
        out += ["\n\n".join(definition for _, definition in built), ""]
        return "\n".join(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--flatc", required=True)
    parser.add_argument("--reflection", type=Path, required=True, help="FlatBuffers' reflection/reflection.fbs")
    parser.add_argument("--bfbs", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("families", nargs="+", help="Schema files whose tables get generated builders")
    args = parser.parse_args()
    generator = Generator(schema_json(args.flatc, args.reflection, args.bfbs))
    for family in args.families:
        path = args.out / (Path(family).stem + "_builders.inc")
        text = generator.family(family)
        if not path.exists() or path.read_text() != text:
            path.write_text(text)


if __name__ == "__main__":
    main()
