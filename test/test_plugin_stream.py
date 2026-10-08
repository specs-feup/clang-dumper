#!/usr/bin/env python3
"""Verify that the Clang plugin emits one valid Protobuf AST stream."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
import unittest
from pathlib import Path

SOURCE = Path(__file__).resolve().parent / "inputs" / "simple_function.cpp"
WARNING_SOURCE = Path(__file__).resolve().parent / "fixtures" / "plugin_warning.cpp"
ERROR_SOURCE = Path(__file__).resolve().parent / "fixtures" / "plugin_error.cpp"
STREAM_MAGIC = b"CLAVAPB1"


class ProtoPluginStreamTest(unittest.TestCase):
    def run_plugin(
        self, source: Path, *compiler_flags: str
    ) -> subprocess.CompletedProcess[bytes]:
        command = [
            str(ARGS.clang),
            f"-fplugin={ARGS.plugin}",
            "-Xclang",
            "-plugin",
            "-Xclang",
            "DumpAst",
            "-Xclang",
            "-plugin-arg-DumpAst",
            "-Xclang",
            "-file-id=42",
            *compiler_flags,
            "-fsyntax-only",
            str(source),
        ]
        return subprocess.run(command, capture_output=True, check=False)

    def assert_valid_stream(self, stream_bytes: bytes) -> None:
        self.assertTrue(
            stream_bytes.startswith(STREAM_MAGIC),
            "missing Protobuf AST stream magic",
        )
        with tempfile.TemporaryDirectory(prefix="clang-dumper-plugin-stream-") as temp:
            stream = Path(temp) / "plugin.pb"
            stream.write_bytes(stream_bytes)
            checked = subprocess.run(
                [str(ARGS.verifier), str(stream)],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(checked.returncode, 0, checked.stderr)

    def test_plugin_emits_a_valid_stream_on_stdout(self) -> None:
        result = self.run_plugin(SOURCE)
        self.assertEqual(
            result.returncode,
            0,
            f"clang plugin failed: stdout={result.stdout!r}, stderr={result.stderr!r}",
        )
        self.assert_valid_stream(result.stdout)
        self.assertNotIn(STREAM_MAGIC, result.stderr)

    def test_warning_diagnostic_stays_on_stderr(self) -> None:
        result = self.run_plugin(WARNING_SOURCE, "-Wall", "-Wextra")
        self.assertEqual(
            result.returncode,
            0,
            f"clang plugin failed: stdout={result.stdout!r}, stderr={result.stderr!r}",
        )
        self.assertIn(b"warning:", result.stderr)
        self.assert_valid_stream(result.stdout)

    def test_error_diagnostic_stays_on_stderr_and_exit_status_is_nonzero(self) -> None:
        result = self.run_plugin(ERROR_SOURCE)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"error:", result.stderr)
        if result.stdout:
            self.assert_valid_stream(result.stdout)


parser = argparse.ArgumentParser()
parser.add_argument("--plugin", type=Path, required=True)
parser.add_argument("--clang", type=Path, required=True)
parser.add_argument("--verifier", type=Path, required=True)
ARGS = parser.parse_args()

if __name__ == "__main__":
    unittest.main(argv=[__file__])
