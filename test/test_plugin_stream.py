#!/usr/bin/env python3
"""Verify that the Clang plugin emits one valid Protobuf AST stream."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
import unittest
from pathlib import Path

SOURCE = Path(__file__).resolve().parent / "inputs" / "simple_function.cpp"


class ProtoPluginStreamTest(unittest.TestCase):
    def test_plugin_emits_a_valid_stream_on_stderr(self) -> None:
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
            "-fsyntax-only",
            str(SOURCE),
        ]
        result = subprocess.run(command, capture_output=True, check=False)
        self.assertEqual(
            result.returncode,
            0,
            f"clang plugin failed: stdout={result.stdout!r}, stderr={result.stderr!r}",
        )
        self.assertEqual(result.stdout, b"")
        self.assertTrue(result.stderr.startswith(b"CLAVAPB1"))

        with tempfile.TemporaryDirectory(prefix="clang-dumper-plugin-stream-") as temp:
            stream = Path(temp) / "plugin.pb"
            stream.write_bytes(result.stderr)
            checked = subprocess.run(
                [str(ARGS.verifier), str(stream)],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(checked.returncode, 0, checked.stderr)


parser = argparse.ArgumentParser()
parser.add_argument("--plugin", type=Path, required=True)
parser.add_argument("--clang", type=Path, required=True)
parser.add_argument("--verifier", type=Path, required=True)
ARGS = parser.parse_args()

if __name__ == "__main__":
    unittest.main(argv=[__file__])
