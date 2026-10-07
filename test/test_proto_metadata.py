#!/usr/bin/env python3
"""Verify the new native metadata is present in an actual Protobuf dump."""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import tempfile
import unittest
from pathlib import Path

from google.protobuf import descriptor_pb2, message_factory

SOURCE = Path(__file__).resolve().parent / "fixtures" / "proto_metadata.cpp"


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


def read_envelopes(path: Path, envelope_type: type) -> list:
    data = path.read_bytes()
    if not data.startswith(b"CLAVAPB1"):
        raise ValueError("missing Protobuf AST stream magic")
    offset = len(b"CLAVAPB1")
    envelopes = []
    while offset < len(data):
        length, offset = read_varint(data, offset)
        end = offset + length
        if length == 0 or end > len(data):
            raise ValueError("invalid Protobuf frame length")
        envelopes.append(envelope_type.FromString(data[offset:end]))
        offset = end
    return envelopes


class ProtoMetadataTest(unittest.TestCase):
    def test_native_emission_contains_structured_metadata(self) -> None:
        descriptor_set = descriptor_pb2.FileDescriptorSet.FromString(
            ARGS.descriptor.read_bytes()
        )
        messages = message_factory.GetMessages(descriptor_set.file)
        envelope_type = messages["astwire.v1.Envelope"]
        with tempfile.TemporaryDirectory(prefix="clang-dumper-proto-metadata-") as temp:
            dump = Path(temp) / "metadata.pb"
            result = subprocess.run(
                [
                    str(ARGS.tool),
                    "-c",
                    str(SOURCE),
                    "-o",
                    str(dump),
                    "--",
                    "-std=gnu++20",
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            checked = subprocess.run(
                [str(ARGS.verifier), str(dump)],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(checked.returncode, 0, checked.stderr)
            envelopes = read_envelopes(dump, envelope_type)

        self.assertEqual(envelopes[0].header.protocol_minor, 1)
        self.assertEqual(
            envelopes[0].header.schema_sha256,
            hashlib.sha256(ARGS.schema.read_bytes()).hexdigest().encode("ascii"),
        )
        nodes = [
            record.node
            for envelope in envelopes
            if envelope.WhichOneof("payload") == "chunk"
            for record in envelope.chunk.records
            if record.WhichOneof("record") == "node"
        ]
        by_class: dict[str, list] = {}
        for node in nodes:
            by_class.setdefault(node.class_name, []).append(node)

        implicit_casts = by_class.get("ImplicitCastExpr", [])
        self.assertTrue(implicit_casts, "implicit cast nodes missing")
        self.assertTrue(
            all(node.WhichOneof("node") == "cast_expr_data" for node in implicit_casts),
            "ImplicitCastExpr nodes must use the checked CastExprData payload",
        )
        c_style_casts = by_class.get("CStyleCastExpr", [])
        self.assertTrue(c_style_casts, "C-style cast nodes missing")
        self.assertTrue(
            all(
                node.WhichOneof("node") == "explicit_cast_expr_data"
                for node in c_style_casts
            ),
            "CStyleCastExpr nodes must retain ExplicitCastExprData metadata",
        )

        function_data = [
            node.function_decl_data
            for node in by_class.get("FunctionDecl", [])
        ]
        function_data.extend(
            node.c_x_x_method_decl_data.base
            for node in by_class.get("CXXMethodDecl", [])
        )
        function_data = [data for data in function_data if data.template_parameters]
        self.assertTrue(function_data, "out-of-line function template params missing")
        self.assertTrue(
            any(
                sum(data.template_parameter_list_sizes)
                == len(data.template_parameters)
                and any(data.template_parameter_list_sizes)
                for data in function_data
            ),
            "function template parameter-list sizes missing or inconsistent",
        )

        partials = by_class.get("ClassTemplatePartialSpecializationDecl", [])
        self.assertTrue(partials, "partial specialization node missing")
        self.assertTrue(
            any(node.class_template_partial_specialization_decl_data.template_parameters for node in partials),
            "partial specialization parameters missing",
        )
        partial_ids = {node.id for node in partials}
        top_level_ids = {
            record.top_level.node
            for envelope in envelopes
            if envelope.WhichOneof("payload") == "chunk"
            for record in envelope.chunk.records
            if record.WhichOneof("record") == "top_level"
        }
        self.assertTrue(
            partial_ids.intersection(top_level_ids),
            "partial specialization is missing from the top-level declaration roots",
        )

        lambdas = [node.lambda_expr_data for node in by_class.get("LambdaExpr", [])]
        init_lambdas = [value for value in lambdas if value.init_capture_names]
        self.assertTrue(init_lambdas, "lambda init-capture names missing")
        for value in init_lambdas:
            count = len(value.capture_kinds)
            self.assertEqual(len(value.init_capture_names), count)
            self.assertEqual(len(value.capture_init_styles), count)
            self.assertEqual(len(value.capture_pack_expansions), count)
            self.assertEqual(len(value.capture_is_implicit), count)
        self.assertTrue(
            any(any(value.capture_pack_expansions) for value in init_lambdas),
            "lambda init-capture pack expansion flag missing",
        )

        assembly = [node.g_c_c_asm_stmt_data for node in by_class.get("GCCAsmStmt", [])]
        self.assertTrue(any(value.is_inline for value in assembly), "asm inline flag missing")
        self.assertTrue(
            any(value.is_goto and "target" in value.labels for value in assembly),
            "asm goto labels missing",
        )
        self.assertTrue(
            any(
                "%0" in value.asm_string and "%l[target]" in value.asm_string
                for value in assembly
            ),
            "source-level asm operand references were rewritten for LLVM IR",
        )

        member_pointer = [
            node.member_pointer_type_data
            for node in by_class.get("MemberPointerType", [])
        ]
        self.assertTrue(member_pointer, "member pointer type node missing")
        self.assertTrue(
            all(
                value.HasField("class_type") and value.HasField("pointee_type")
                for value in member_pointer
            ),
            "member pointer class or pointee type reference missing",
        )

        constructions = [
            node.c_x_x_unresolved_construct_expr_data
            for node in by_class.get("CXXUnresolvedConstructExpr", [])
        ]
        self.assertTrue(constructions, "dependent construction node missing")
        self.assertIn(False, [value.is_list_initialization for value in constructions])
        self.assertIn(True, [value.is_list_initialization for value in constructions])

        friends = [node.friend_decl_data for node in by_class.get("FriendDecl", [])]
        self.assertTrue(friends, "FriendDecl node missing")
        self.assertTrue(all(value.HasField("owner_record") for value in friends))
        self.assertTrue(
            all(
                value.HasField("friend_decl") and value.HasField("friend_type")
                for value in friends
            ),
            "typed-null friend target must retain presence",
        )


parser = argparse.ArgumentParser()
parser.add_argument("--tool", type=Path, required=True)
parser.add_argument("--verifier", type=Path, required=True)
parser.add_argument("--descriptor", type=Path, required=True)
parser.add_argument("--schema", type=Path, required=True)
ARGS = parser.parse_args()

if __name__ == "__main__":
    unittest.main(argv=[__file__])
