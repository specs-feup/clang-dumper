#!/usr/bin/env python3
"""Test release manifest verification against the local native build artifacts."""

from __future__ import annotations

import argparse
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "scripts" / "write_release_manifest.py"
SPEC = importlib.util.spec_from_file_location("write_release_manifest", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
manifest = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(manifest)


class ReleaseManifestTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory(prefix="clang-dumper-manifest-test-")
        self.output = Path(self.temporary.name)

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def write_manifest(self) -> Path:
        manifest.write_local_build(
            output_dir=self.output,
            tool=ARGS.tool,
            descriptor=ARGS.descriptor,
            platform=ARGS.platform,
            arch=ARGS.arch,
            llvm_major=ARGS.llvm_major,
            protoc=ARGS.protoc,
            plugin=ARGS.plugin,
        )
        return self.output / manifest.MANIFEST_NAME

    def test_local_manifest_contract_and_asset_hashes(self) -> None:
        path = self.write_manifest()
        manifest.verify_manifest(path)
        value = json.loads(path.read_text(encoding="utf-8"))
        self.assertEqual(value["toolchain"], {"protobuf_version": "28.3", "protoc_version": "28.3"})
        self.assertEqual(
            value["compatibility"],
            {
                "minimum_java_protoc_version": "4.28.3",
                "minimum_java_runtime_version": "4.28.3",
            },
        )
        self.assertEqual(value["protocol"]["semantic_contract"], "clava-ast-wire-v1")
        self.assertEqual(value["protocol"]["llvm_major"], ARGS.llvm_major)
        names = {item["filename"] for item in value["assets"]}
        self.assertIn(manifest.SCHEMA_NAME, names)
        self.assertIn(manifest.DESCRIPTOR_NAME, names)
        self.assertIn(ARGS.tool.name, names)
        if ARGS.plugin is not None:
            self.assertIn(ARGS.plugin.name, names)
        self.assertEqual(
            value["generic_payload_contracts"]["attributes"]["payload"], "AttributeData"
        )
        self.assertEqual(value["generic_payload_contracts"]["openmp"]["payload"], "StmtData")

    def test_manifest_rejects_tampered_executable(self) -> None:
        path = self.write_manifest()
        executable = self.output / ARGS.tool.name
        with executable.open("ab") as target:
            target.write(b"tampered")
        with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
            manifest.verify_manifest(path)

    def test_manifest_rejects_schema_descriptor_mismatch(self) -> None:
        path = self.write_manifest()
        descriptor = self.output / manifest.DESCRIPTOR_NAME
        descriptor.write_bytes(descriptor.read_bytes() + b"\x00")
        with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
            manifest.verify_manifest(path)


parser = argparse.ArgumentParser()
parser.add_argument("--tool", type=Path, required=True)
parser.add_argument("--plugin", type=Path)
parser.add_argument("--descriptor", type=Path, required=True)
parser.add_argument("--protoc", type=Path, required=True)
parser.add_argument("--platform", choices=("linux", "macos", "windows"), required=True)
parser.add_argument("--arch", required=True)
parser.add_argument("--llvm-major", type=int, required=True)
ARGS = parser.parse_args()

if __name__ == "__main__":
    unittest.main(argv=[__file__])
