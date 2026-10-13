#!/usr/bin/env python3
"""Check packed type reachability and typed C++ header expression payloads."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
import unittest
from pathlib import Path

from google.protobuf import descriptor_pb2, descriptor_pool, message_factory

SOURCE = Path(__file__).resolve().parent / "fixtures" / "proto_header_expression_support.cpp"
STREAM_MAGIC = b"CLAVAPB1"


def read_varint(data: bytes, offset: int) -> tuple[int, int]:
    value = 0
    shift = 0
    while offset < len(data) and shift < 64:
        byte = data[offset]
        offset += 1
        if shift == 63 and byte > 1:
            raise ValueError("overlong Protobuf frame length")
        value |= (byte & 0x7F) << shift
        if byte < 0x80:
            return value, offset
        shift += 7
    raise ValueError("truncated Protobuf frame length")


def read_envelopes(data: bytes, envelope_type: type) -> list:
    if not data.startswith(STREAM_MAGIC):
        raise ValueError("missing Protobuf AST stream magic")
    offset = len(STREAM_MAGIC)
    envelopes = []
    while offset < len(data):
        length, offset = read_varint(data, offset)
        end = offset + length
        if length == 0 or end > len(data):
            raise ValueError("invalid Protobuf frame length")
        envelopes.append(envelope_type.FromString(data[offset:end]))
        offset = end
    return envelopes


def make_envelope_type(descriptor_path: Path) -> type:
    descriptors = descriptor_pb2.FileDescriptorSet.FromString(descriptor_path.read_bytes())
    pool = descriptor_pool.DescriptorPool()
    for descriptor in descriptors.file:
        pool.Add(descriptor)
    descriptor = pool.FindMessageTypeByName("astwire.v1.Envelope")
    get_message_class = getattr(message_factory, "GetMessageClass", None)
    if get_message_class is not None:
        return get_message_class(descriptor)
    return message_factory.MessageFactory(pool).GetPrototype(descriptor)


def nested_template_arguments(argument) -> list:
    if argument.WhichOneof("template") == "template_pack":
        return [nested for child in argument.template_pack.arguments
                for nested in nested_template_arguments(child)]
    return [argument]


def decl_name(payload) -> str | None:
    while payload is not None:
        if "decl_name" in payload.DESCRIPTOR.fields_by_name:
            return payload.decl_name
        if "base" not in payload.DESCRIPTOR.fields_by_name or not payload.HasField("base"):
            return None
        payload = payload.base
    return None


class HeaderExpressionSupportTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.envelope_type = make_envelope_type(ARGS.descriptor)

    def test_packed_types_and_fold_expression_links_are_emitted(self) -> None:
        command = [
            str(ARGS.clang),
            f"-fplugin={ARGS.plugin}",
            "-Xclang", "-plugin", "-Xclang", "DumpAst",
            "-Xclang", "-plugin-arg-DumpAst", "-Xclang", "-file-id=43",
            "-std=c++20", "-fsyntax-only", str(SOURCE),
        ]
        emitted = subprocess.run(command, capture_output=True, check=False)
        self.assertEqual(emitted.returncode, 0,
                         f"plugin failed: stdout={emitted.stdout!r}, stderr={emitted.stderr!r}")

        with tempfile.TemporaryDirectory(prefix="clang-dumper-header-expressions-") as temp:
            stream = Path(temp) / "ast.pb"
            stream.write_bytes(emitted.stdout)
            checked = subprocess.run([str(ARGS.verifier), str(stream)], capture_output=True,
                                     text=True, check=False)
        self.assertEqual(checked.returncode, 0, checked.stderr)

        envelopes = read_envelopes(emitted.stdout, self.envelope_type)
        records = [record for envelope in envelopes if envelope.WhichOneof("payload") == "chunk"
                   for record in envelope.chunk.records]
        nodes = {record.node.id: record.node for record in records
                 if record.WhichOneof("record") == "node"}
        children = {record.children.node: set(record.children.children) for record in records
                    if record.WhichOneof("record") == "children"}

        pack_decl = next(node for node in nodes.values()
                         if node.class_name == "ClassTemplateSpecializationDecl"
                         and decl_name(node.class_template_specialization_decl_data) == "PackedTypes")
        packed_arguments = pack_decl.class_template_specialization_decl_data.template_arguments
        self.assertTrue(packed_arguments)
        packed_types = [argument for root in packed_arguments
                        for argument in nested_template_arguments(root)
                         if argument.WhichOneof("template") == "template_type"]
        self.assertTrue(packed_types, "fixture did not produce type arguments inside a pack")
        for argument in packed_types:
            self.assertIn(argument.template_type.type, nodes)

        folds = [node for node in nodes.values() if node.class_name == "CXXFoldExpr"]
        self.assertGreaterEqual(len(folds), 5)
        self.assertTrue(all(node.WhichOneof("node") == "c_x_x_fold_expr_data" for node in folds))
        fold_data = [node.c_x_x_fold_expr_data for node in folds]
        self.assertTrue(any(data.is_left_fold for data in fold_data))
        self.assertTrue(any(data.is_right_fold for data in fold_data))
        self.assertTrue(any(data.HasField("init") for data in fold_data))
        self.assertTrue(any(data.HasField("callee") for data in fold_data),
                        "overloaded fold did not retain its callee link")

        expected_forms = {
            "values - ...": (True, None, "lhs"),
            "... - values": (False, None, "rhs"),
            "5 - ... - values": (False, "5", "rhs"),
            "values - ... - 5": (True, "5", "lhs"),
        }
        source_lines = SOURCE.read_text(encoding="utf-8").splitlines()
        observed_forms = {}

        for node, data in zip(folds, fold_data):
            self.assertTrue(data.HasField("op"))
            self.assertTrue(data.HasField("pattern"))
            self.assertTrue(data.HasField("is_left_fold"))
            self.assertTrue(data.HasField("is_right_fold"))
            self.assertTrue(data.HasField("ellipsis_loc"))
            self.assertTrue(data.base.base.base.source.expansion.HasField("line"))
            references = [data.pattern]
            references.extend(value for field in ("lhs", "rhs", "init", "callee")
                              if data.HasField(field) for value in [getattr(data, field)])
            self.assertTrue(all(reference in nodes for reference in references))
            self.assertTrue(set(references).issubset(children[node.id]))

            source_line_number = data.base.base.base.source.expansion.line
            source_line = source_lines[source_line_number - 1]
            return_start = source_line.index("return (") + len("return (")
            return_end = source_line.index(");", return_start)
            expression = source_line[return_start:return_end]
            if expression not in expected_forms:
                continue

            self.assertNotIn(expression, observed_forms)
            observed_forms[expression] = data
            is_right_fold, init_text, pattern_side = expected_forms[expression]
            self.assertEqual("BINARYOPERATORKIND_SUB",
                             data.DESCRIPTOR.fields_by_name["op"].enum_type
                             .values_by_number[data.op].name)
            self.assertEqual(is_right_fold, data.is_right_fold)
            self.assertEqual(not is_right_fold, data.is_left_fold)
            self.assertEqual(data.pattern, getattr(data, pattern_side))
            self.assertEqual(source_line_number, data.l_paren_loc.line)
            self.assertEqual(source_line_number, data.ellipsis_loc.line)
            self.assertEqual(source_line_number, data.r_paren_loc.line)
            self.assertEqual(source_line.index("(", source_line.index("return")) + 1,
                             data.l_paren_loc.column)
            self.assertEqual(source_line.index("...", return_start) + 1, data.ellipsis_loc.column)
            self.assertEqual(source_line.index(")", return_start) + 1, data.r_paren_loc.column)

            other_side = "lhs" if pattern_side == "rhs" else "rhs"
            if init_text is None:
                self.assertFalse(data.HasField("init"))
                self.assertFalse(data.HasField(other_side))
            else:
                self.assertTrue(data.HasField("init"))
                self.assertEqual(data.init, getattr(data, other_side))

        self.assertEqual(set(expected_forms), set(observed_forms))

        using = next(node for node in nodes.values()
                     if node.class_name == "UnresolvedUsingValueDecl")
        self.assertEqual(using.WhichOneof("node"), "unresolved_using_value_decl_data")
        using_data = using.unresolved_using_value_decl_data
        self.assertEqual(using_data.qualifier, "ValueBase<T>::")
        self.assertTrue(using_data.is_pack_expansion is False)
        self.assertTrue(using_data.HasField("qualifier_source_range"))
        self.assertTrue(using_data.HasField("using_location"))
        self.assertTrue(using_data.HasField("name_location"))
        self.assertTrue(using_data.base.type in nodes)
        self.assertTrue(using_data.base.base.decl_name == "value")
        self.assertTrue(using_data.base.base.base.base.source.expansion.HasField("line"))


parser = argparse.ArgumentParser()
parser.add_argument("--plugin", type=Path, required=True)
parser.add_argument("--clang", type=Path, required=True)
parser.add_argument("--verifier", type=Path, required=True)
parser.add_argument("--descriptor", type=Path, required=True)
ARGS = parser.parse_args()

if __name__ == "__main__":
    unittest.main(argv=[__file__])
