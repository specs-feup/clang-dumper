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

        builtin_templates = [node for node in nodes.values()
                            if node.class_name == "BuiltinTemplateDecl"]
        self.assertTrue(builtin_templates, "fixture did not emit builtin template declarations")
        builtin_kinds = {}
        for node in builtin_templates:
            self.assertEqual(node.WhichOneof("node"), "builtin_template_decl_data")
            data = node.builtin_template_decl_data
            name = decl_name(data)
            if name not in {"__make_integer_seq", "__type_pack_element"}:
                continue
            self.assertTrue(data.HasField("builtin_kind"))
            self.assertFalse(data.base.HasField("template_decl"),
                             "builtin template must not invent an underlying declaration")
            self.assertTrue(data.base.template_parameters)
            self.assertTrue(all(parameter in nodes for parameter in data.base.template_parameters))
            builtin_kinds[name] = data.DESCRIPTOR.fields_by_name["builtin_kind"].enum_type \
                .values_by_number[data.builtin_kind].name
        self.assertEqual(builtin_kinds, {
            "__make_integer_seq": "BUILTINTEMPLATEKIND_MAKE_INTEGER_SEQ",
            "__type_pack_element": "BUILTINTEMPLATEKIND_TYPE_PACK_ELEMENT",
        })

        hidden_emitted = subprocess.run(
            [str(ARGS.clang), "-fvisibility=hidden", *command[1:]],
            capture_output=True, check=False)
        self.assertEqual(hidden_emitted.returncode, 0,
                         f"plugin failed with hidden visibility: {hidden_emitted.stderr!r}")
        hidden_nodes = [record.node for envelope in read_envelopes(hidden_emitted.stdout, self.envelope_type)
                        if envelope.WhichOneof("payload") == "chunk"
                        for record in envelope.chunk.records if record.WhichOneof("record") == "node"]
        hidden_builtin = next(node.builtin_template_decl_data for node in hidden_nodes
                              if node.class_name == "BuiltinTemplateDecl"
                              and decl_name(node.builtin_template_decl_data) == "__make_integer_seq")
        self.assertTrue(hidden_builtin.base.base.HasField("visibility"))
        self.assertEqual("VISIBILITY_HIDDEN",
                         hidden_builtin.base.base.DESCRIPTOR.fields_by_name["visibility"].enum_type
                         .values_by_number[hidden_builtin.base.base.visibility].name)

        rewritten = [node for node in nodes.values()
                     if node.class_name == "CXXRewrittenBinaryOperator"]
        self.assertEqual(len(rewritten), 2, "fixture should emit both rewritten comparison forms")
        expected_operators = {"left < right": "BINARYOPERATORKIND_LT",
                              "left > right": "BINARYOPERATORKIND_GT"}
        observed_operators = set()
        for node in rewritten:
            self.assertEqual(node.WhichOneof("node"), "c_x_x_rewritten_binary_operator_data")
            data = node.c_x_x_rewritten_binary_operator_data
            for field in ("op", "lhs", "rhs", "semantic_form", "inner_bin_op",
                          "is_reversed", "operator_location"):
                self.assertTrue(data.HasField(field), f"missing rewritten-operator {field}")
            references = (data.lhs, data.rhs, data.semantic_form, data.inner_bin_op)
            self.assertTrue(all(reference in nodes for reference in references))
            self.assertNotEqual(data.lhs, data.rhs)
            self.assertNotEqual(data.semantic_form, data.inner_bin_op)

            line_number = data.base.base.base.source.expansion.line
            source_line = source_lines[line_number - 1]
            source_expression = source_line.split("return ", 1)[1].split(";", 1)[0]
            self.assertIn(source_expression, expected_operators)
            self.assertEqual(expected_operators[source_expression],
                             data.DESCRIPTOR.fields_by_name["op"].enum_type
                             .values_by_number[data.op].name)
            self.assertTrue(data.is_reversed,
                            "only the reversed <=> candidate accepts the non-commutative operand types")
            self.assertEqual(line_number, data.operator_location.line)
            self.assertEqual(source_line.index(source_expression.split()[1]) + 1,
                             data.operator_location.column)
            observed_operators.add(source_expression)
        self.assertEqual(observed_operators, set(expected_operators))

        lifetime_decls = [node for node in nodes.values()
                          if node.class_name == "LifetimeExtendedTemporaryDecl"]
        self.assertTrue(lifetime_decls, "fixture did not emit lifetime-extended temporaries")
        lifetime_ids = {node.id for node in lifetime_decls}
        for node in lifetime_decls:
            self.assertEqual(node.WhichOneof("node"), "lifetime_extended_temporary_decl_data")
            data = node.lifetime_extended_temporary_decl_data
            self.assertTrue(data.HasField("extending_decl"))
            self.assertTrue(data.HasField("temporary_expr"))
            self.assertTrue(data.HasField("storage_duration"))
            self.assertTrue(data.HasField("mangling_number"))
            self.assertIn(data.extending_decl, nodes)
            self.assertIn(data.temporary_expr, nodes)
            self.assertIn(data.temporary_expr, children[node.id])
            self.assertEqual(nodes[data.extending_decl].class_name, "VarDecl")

        materialized = [node for node in nodes.values()
                        if node.class_name == "MaterializeTemporaryExpr"
                        and node.materialize_temporary_expr_data.HasField(
                            "lifetime_extended_temporary_decl")]
        self.assertTrue(materialized, "materialization nodes lost their lifetime declaration link")
        for node in materialized:
            data = node.materialize_temporary_expr_data
            self.assertIn(data.lifetime_extended_temporary_decl, lifetime_ids)
            owner = nodes[data.lifetime_extended_temporary_decl].lifetime_extended_temporary_decl_data
            self.assertEqual(data.extending_decl, owner.extending_decl)


parser = argparse.ArgumentParser()
parser.add_argument("--plugin", type=Path, required=True)
parser.add_argument("--clang", type=Path, required=True)
parser.add_argument("--verifier", type=Path, required=True)
parser.add_argument("--descriptor", type=Path, required=True)
ARGS = parser.parse_args()

if __name__ == "__main__":
    unittest.main(argv=[__file__])
