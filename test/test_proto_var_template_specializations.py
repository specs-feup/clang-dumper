#!/usr/bin/env python3
"""Check typed variable-template specialization data in native protobuf output."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
import unittest
from pathlib import Path

from google.protobuf import descriptor_pb2, descriptor_pool, message_factory

SOURCE = Path(__file__).resolve().parent / "fixtures" / "proto_var_template_specializations.cpp"
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


class VarTemplateSpecializationProtoTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.envelope_type = make_envelope_type(ARGS.descriptor)

    def test_specializations_keep_typed_data_references_and_source(self) -> None:
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

        with tempfile.TemporaryDirectory(prefix="clang-dumper-var-template-") as temp:
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
        by_name = {}
        for node in nodes.values():
            if node.class_name in ("VarTemplateDecl", "VarTemplatePartialSpecializationDecl",
                                   "VarTemplateSpecializationDecl"):
                by_name.setdefault(node.class_name, []).append(node)

        self.assertEqual(len(by_name.get("VarTemplateDecl", [])), 1)
        self.assertEqual(len(by_name.get("VarTemplatePartialSpecializationDecl", [])), 1)
        self.assertEqual(len(by_name.get("VarTemplateSpecializationDecl", [])), 2)

        primary = by_name["VarTemplateDecl"][0]
        partial = by_name["VarTemplatePartialSpecializationDecl"][0]
        specializations = by_name["VarTemplateSpecializationDecl"]
        self.assertEqual(len(specializations), 2)
        self.assertEqual(partial.WhichOneof("node"), "var_template_partial_specialization_decl_data")

        partial_data = partial.var_template_partial_specialization_decl_data
        specialization_field = specializations[0].var_template_specialization_decl_data.DESCRIPTOR.fields_by_name[
            "specialization_kind"]
        kind_values = specialization_field.enum_type.values_by_name
        explicit_kind = kind_values["TEMPLATESPECIALIZATIONKIND_EXPLICITSPECIALIZATION"].number
        implicit_kind = kind_values["TEMPLATESPECIALIZATIONKIND_IMPLICITINSTANTIATION"].number
        explicit = next(node for node in specializations
                        if node.var_template_specialization_decl_data.specialization_kind == explicit_kind)
        implicit = next(node for node in specializations
                        if node.var_template_specialization_decl_data.specialization_kind == implicit_kind)
        explicit_data = explicit.var_template_specialization_decl_data
        implicit_data = implicit.var_template_specialization_decl_data
        self.assertEqual(partial_data.base.specialized_template, primary.id)
        self.assertEqual(explicit_data.specialized_template, primary.id)
        self.assertEqual(implicit_data.specialized_template, primary.id)
        self.assertEqual(explicit_data.specialized_partial_specialization, -2)
        self.assertEqual(implicit_data.specialized_partial_specialization, partial.id)
        self.assertEqual(len(partial_data.base.template_arguments), 2)
        self.assertEqual(len(partial_data.base.template_instantiation_arguments), 2)
        self.assertEqual(len(partial_data.template_arguments_as_written), 2)
        self.assertEqual(len(explicit_data.template_arguments), 2)
        self.assertEqual(len(implicit_data.template_arguments), 2)
        self.assertEqual(len(explicit_data.template_instantiation_arguments), 2)
        # The partial pattern has one template parameter (T); its matched
        # instantiation argument list therefore contains the deduced type only.
        self.assertEqual(len(implicit_data.template_instantiation_arguments), 1)
        self.assertEqual(len(partial_data.template_parameters), 1)
        self.assertEqual(nodes[partial_data.template_parameters[0]].class_name, "TemplateTypeParmDecl")
        self.assertEqual(partial_data.base.specialized_partial_specialization, -2)
        self.assertFalse(partial_data.is_member_specialization)
        self.assertEqual(partial_data.instantiated_from_member, -2)
        self.assertEqual(explicit_data.specialization_kind, explicit_kind)
        self.assertEqual(implicit_data.specialization_kind, implicit_kind)
        self.assertTrue(implicit_data.HasField("point_of_instantiation"))
        self.assertEqual(implicit_data.point_of_instantiation.line, 10)
        self.assertFalse(explicit_data.HasField("point_of_instantiation"))

        partial_source = base_with_source(partial_data)
        explicit_source = base_with_source(explicit_data)
        implicit_source = base_with_source(implicit_data)
        self.assertEqual(partial_source.expansion.line, 4)
        self.assertEqual(explicit_source.expansion.line, 7)
        self.assertEqual(implicit_source.expansion.line, 4)

        child_records = {record.children.node: record.children.children for record in records
                         if record.WhichOneof("record") == "children"}
        partial_child_ids = child_records[partial.id]
        explicit_child_ids = child_records[explicit.id]
        self.assertTrue(any(nodes[child_id].class_name == "IntegerLiteral" for child_id in partial_child_ids))
        self.assertTrue(any(nodes[child_id].class_name == "IntegerLiteral" for child_id in explicit_child_ids))


parser = argparse.ArgumentParser()
parser.add_argument("--plugin", type=Path, required=True)
parser.add_argument("--clang", type=Path, required=True)
parser.add_argument("--verifier", type=Path, required=True)
parser.add_argument("--descriptor", type=Path, required=True)
ARGS = parser.parse_args()

if __name__ == "__main__":
    unittest.main(argv=[__file__])
