#!/usr/bin/env python3
"""Check typed C++ deduction-guide data, identities, links, and source."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
import unittest
from pathlib import Path

from google.protobuf import descriptor_pb2, descriptor_pool, message_factory

SOURCE = Path(__file__).resolve().parent / "fixtures" / "proto_cxx_deduction_guides.cpp"
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


def base_with_source(payload):
    while "base" in payload.DESCRIPTOR.fields_by_name and payload.HasField("base"):
        payload = payload.base
    return payload.source


class CXXDeductionGuideProtoTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.envelope_type = make_envelope_type(ARGS.descriptor)

    def test_guides_keep_template_constructor_candidate_explicitness_and_source(self) -> None:
        command = [
            str(ARGS.clang),
            f"-fplugin={ARGS.plugin}",
            "-Xclang", "-plugin", "-Xclang", "DumpAst",
            "-Xclang", "-plugin-arg-DumpAst", "-Xclang", "-file-id=42",
            "-std=c++20", "-fsyntax-only", str(SOURCE),
        ]
        emitted = subprocess.run(command, capture_output=True, check=False)
        self.assertEqual(emitted.returncode, 0,
                         f"plugin failed: stdout={emitted.stdout!r}, stderr={emitted.stderr!r}")

        with tempfile.TemporaryDirectory(prefix="clang-dumper-deduction-guide-") as temp:
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
        child_records = {record.children.node: record.children.children for record in records
                         if record.WhichOneof("record") == "children"}
        guides = [node for node in nodes.values()
                  if node.class_name == "CXXDeductionGuideDecl"]
        self.assertGreaterEqual(len(guides), 3)
        for guide in guides:
            self.assertEqual(guide.WhichOneof("node"), "c_x_x_deduction_guide_decl_data")

        guide_field = guides[0].DESCRIPTOR.fields_by_name["c_x_x_deduction_guide_decl_data"]
        candidate_field = guide_field.message_type.fields_by_name["deduction_candidate_kind"]
        candidate_names = candidate_field.enum_type.values_by_number
        candidate_kinds = {
            candidate_names[guide.c_x_x_deduction_guide_decl_data.deduction_candidate_kind].name
            for guide in guides
        }
        self.assertEqual(candidate_kinds, {"DEDUCTIONCANDIDATEKIND_NORMAL"})

        guide_box_template = next(node for node in nodes.values()
                                  if node.class_name == "ClassTemplateDecl"
                                  and node.template_decl_data.base.decl_name == "GuideBox")
        explicit_line = next(index for index, line in enumerate(SOURCE.read_text().splitlines(), 1)
                             if "explicit(true)" in line)
        explicit_guide = next(
            node for node in guides
            if node.c_x_x_deduction_guide_decl_data.is_explicit
            and base_with_source(node.c_x_x_deduction_guide_decl_data).expansion.line == explicit_line)
        explicit_data = explicit_guide.c_x_x_deduction_guide_decl_data
        self.assertEqual(explicit_data.deduced_template, guide_box_template.id)
        self.assertEqual(nodes[explicit_data.deduced_template].class_name, "ClassTemplateDecl")
        self.assertTrue(explicit_data.HasField("corresponding_constructor"))
        self.assertEqual(explicit_data.corresponding_constructor, -2)
        self.assertEqual(explicit_data.template_requires_clause, -3)
        self.assertTrue(explicit_data.explicit_specifier.is_specified)
        self.assertTrue(explicit_data.explicit_specifier.HasField("expr"))
        self.assertEqual(nodes[explicit_data.explicit_specifier.expr].class_name, "ConstantExpr")
        self.assertEqual(base_with_source(explicit_data).expansion.line, explicit_line)
        self.assertTrue(any(nodes[child].class_name == "ParmVarDecl"
                            for child in child_records[explicit_guide.id]))

        false_line = next(index for index, line in enumerate(SOURCE.read_text().splitlines(), 1)
                          if "explicit(false)" in line)
        false_guide = next(
            node for node in guides
            if not node.c_x_x_deduction_guide_decl_data.is_explicit
            and node.c_x_x_deduction_guide_decl_data.explicit_specifier.is_specified
            and base_with_source(node.c_x_x_deduction_guide_decl_data).expansion.line == false_line)
        self.assertTrue(false_guide.c_x_x_deduction_guide_decl_data.explicit_specifier.HasField("expr"))
        self.assertEqual(nodes[false_guide.c_x_x_deduction_guide_decl_data.explicit_specifier.expr].class_name,
                         "ConstantExpr")

        constrained_guide = next(node for node in guides
                                 if node.c_x_x_deduction_guide_decl_data.template_requires_clause > 0)
        requires_clause_id = constrained_guide.c_x_x_deduction_guide_decl_data.template_requires_clause
        self.assertEqual(nodes[requires_clause_id].class_name, "ParenExpr")
        self.assertTrue(any(nodes[child].class_name == "BinaryOperator"
                            for child in child_records[requires_clause_id]))

        function_type_id = explicit_data.base.base.base.type
        function_type = nodes[function_type_id]
        self.assertEqual(function_type.class_name, "FunctionProtoType")
        return_type_id = function_type.function_proto_type_data.base.return_type
        return_type = nodes[return_type_id]
        if return_type.class_name == "ElaboratedType":
            return_type = nodes[return_type.elaborated_type_data.named_type]
        self.assertEqual(return_type.class_name, "TemplateSpecializationType")
        self.assertEqual(return_type.template_specialization_type_data.template_decl,
                         explicit_data.deduced_template)

        guide_templates = [node for node in nodes.values()
                           if node.class_name == "FunctionTemplateDecl"
                           and node.template_decl_data.template_decl == explicit_guide.id]
        self.assertEqual(len(guide_templates), 1)
        self.assertIn(explicit_guide.id, child_records[guide_templates[0].id])


parser = argparse.ArgumentParser()
parser.add_argument("--plugin", type=Path, required=True)
parser.add_argument("--clang", type=Path, required=True)
parser.add_argument("--verifier", type=Path, required=True)
parser.add_argument("--descriptor", type=Path, required=True)
ARGS = parser.parse_args()

if __name__ == "__main__":
    unittest.main(argv=[__file__])
