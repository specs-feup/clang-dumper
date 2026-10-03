#!/usr/bin/env python3

import os
import re
import subprocess
import tempfile
import unittest
from pathlib import Path



def clang_dumper_tool() -> Path:
    configured_path = os.environ.get("CLANG_DUMPER_TOOL")
    if configured_path:
        return Path(configured_path)
    return Path(__file__).resolve().parents[1] / "build" / "tool"


@unittest.skipUnless(
    clang_dumper_tool().is_file(),
    "build/tool is required for FlatBuffers integration tests",
)
class FlatBuffersIntegrationTest(unittest.TestCase):
    source = Path(__file__).parent / "inputs" / "simple_function.cpp"

    @staticmethod
    def verifier() -> Path:
        configured_path = os.environ.get("CLANG_DUMPER_VERIFIER")
        if configured_path:
            return Path(configured_path)
        return Path(__file__).resolve().parents[1] / "build" / "verify_flatbuffers"

    def run_tool(self, *arguments: str) -> subprocess.CompletedProcess[bytes]:
        return subprocess.run(
            [str(clang_dumper_tool()), *arguments],
            capture_output=True,
            check=False,
        )

    def write_dump(self, directory: str) -> Path:
        output = Path(directory) / "ast.clv2"
        result = self.run_tool(
            "-id=42", "-system-header-threshold=1", "-c", str(self.source),
            "-o", str(output), "--", "-std=c++17",
        )
        self.assertEqual(0, result.returncode, result.stderr.decode(errors="replace"))
        self.assertTrue(output.is_file())
        self.assertEqual(b"", result.stdout)
        return output

    def test_file_output_is_a_complete_verified_stream(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = self.write_dump(directory)
            verified = subprocess.run(
                [str(self.verifier()), str(output)],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(0, verified.returncode, verified.stderr)
        self.assertIn("blocks=", verified.stdout)
        self.assertIn(" records=", verified.stdout)
        self.assertIn(" nodes=", verified.stdout)

    def test_requires_output_path_and_rejects_legacy_selectors(self) -> None:
        no_output = self.run_tool(str(self.source), "--", "-std=c++17")
        self.assertNotEqual(0, no_output.returncode)
        self.assertIn(b"-o <path> is required", no_output.stderr)
        for legacy in ("-ast-dump-format=text", "-ast-dump-format=flatbuffers-v2",
                       "-ast-dump-compression=zstd"):
            with self.subTest(legacy=legacy), tempfile.TemporaryDirectory() as directory:
                result = self.run_tool(
                    "-c", str(self.source), "-o", str(Path(directory) / "out.clv2"),
                    legacy, "--", "-std=c++17",
                )
                self.assertNotEqual(0, result.returncode)

    def test_verifier_rejects_truncation_and_schema_mismatch(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            original = self.write_dump(directory).read_bytes()
            truncated = Path(directory) / "truncated.clv2"
            truncated.write_bytes(original[:-1])
            rejected_truncated = subprocess.run(
                [str(self.verifier()), str(truncated)], capture_output=True, text=True
            )
            self.assertNotEqual(0, rejected_truncated.returncode)

            changed = bytearray(original)
            match = re.search(rb"[0-9a-f]{64}", changed)
            self.assertIsNotNone(match, "header must contain its schema digest")
            assert match is not None
            changed[match.start()] = ord("0") if changed[match.start()] != ord("0") else ord("1")
            mismatch = Path(directory) / "wrong-schema.clv2"
            mismatch.write_bytes(changed)
            rejected_schema = subprocess.run(
                [str(self.verifier()), str(mismatch)], capture_output=True, text=True
            )
        self.assertNotEqual(0, rejected_schema.returncode)
        self.assertIn("schema hash mismatch", rejected_schema.stderr)

    def test_verifier_rejects_a_missing_terminal_end_record(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            changed = bytearray(self.write_dump(directory).read_bytes())

            def u16(offset: int) -> int:
                return int.from_bytes(changed[offset : offset + 2], "little")

            def u32(offset: int) -> int:
                return int.from_bytes(changed[offset : offset + 4], "little")

            def table_vtable(table: int) -> int:
                return table - int.from_bytes(changed[table : table + 4], "little", signed=True)

            block = 4 + u32(4)
            block_vtable = table_vtable(block)
            records_field = u16(block_vtable + 4)
            records_vector = block + records_field + u32(block + records_field)
            record_count = u32(records_vector)
            last_record_offset = records_vector + 4 + (record_count - 1) * 4
            last_record = last_record_offset + u32(last_record_offset)
            record_vtable = table_vtable(last_record)
            payload_type_field = u16(record_vtable + 4)
            payload_type_offset = last_record + payload_type_field
            self.assertEqual(12, changed[payload_type_offset])
            changed[payload_type_offset] = 10  # End -> Counter; valid union, missing terminator.

            malformed = Path(directory) / "missing-end.clv2"
            malformed.write_bytes(changed)
            result = subprocess.run(
                [str(self.verifier()), str(malformed)], capture_output=True, text=True
            )
        self.assertNotEqual(0, result.returncode)
        self.assertIn("missing its header or end record", result.stderr)

    def test_output_truncates_existing_contents_and_path_errors_are_reported(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast.clv2"
            output.write_bytes(b"stale data")
            result = self.run_tool(
                "-c", str(self.source), "-o", str(output), "--", "-std=c++17"
            )
            self.assertEqual(0, result.returncode, result.stderr.decode(errors="replace"))
            self.assertFalse(output.read_bytes().startswith(b"stale"))

            missing_parent = Path(directory) / "missing" / "ast.clv2"
            failed = self.run_tool(
                "-c", str(self.source), "-o", str(missing_parent), "--", "-std=c++17"
            )
            self.assertNotEqual(0, failed.returncode)
            self.assertIn(b"Cannot open AST dump output", failed.stderr)


if __name__ == "__main__":
    unittest.main()
