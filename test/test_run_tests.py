#!/usr/bin/env python3

import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from run_tests import (
    normalize_captured_output,
    normalize_static_output,
    normalize_system_source_blocks,
    strip_clang_diagnostics,
    unresolved_node_ids,
)


def clang_dumper_tool() -> Path:
    configured_path = os.environ.get("CLANG_DUMPER_TOOL")
    if configured_path:
        return Path(configured_path)
    return Path(__file__).resolve().parents[1] / "build" / "tool"


@unittest.skipUnless(
    clang_dumper_tool().is_file(),
    "build/tool is required for stream separation integration tests",
)
class DumpStreamIntegrationTest(unittest.TestCase):
    source = Path(__file__).parent / "inputs" / "simple_function.cpp"
    throwing_source = Path(__file__).parent / "inputs" / "throw.cpp"

    def run_tool(self, source: Path, output: Path | None = None) -> subprocess.CompletedProcess[str]:
        command = [
            str(clang_dumper_tool()),
            "-id=42",
            "-system-header-threshold=1",
        ]
        if output is not None:
            command.extend(["-c", "-o", str(output)])
        command.extend([str(source), "--"])
        return subprocess.run(command, capture_output=True, text=True, check=False)

    def test_file_output_is_byte_identical_to_legacy_protocol(self) -> None:
        legacy = self.run_tool(self.source)
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast.dump"
            separated = self.run_tool(self.source, output)
            self.assertEqual(separated.returncode, legacy.returncode)
            self.assertEqual(separated.stdout, legacy.stdout)
            legacy_dump, _ = normalize_captured_output(
                legacy.stderr, str(self.source.parent.resolve())
            )
            separated_dump, _ = normalize_captured_output(
                output.read_text(encoding="utf-8"), str(self.source.parent.resolve())
            )
            self.assertEqual(separated_dump, legacy_dump)
            self.assertNotIn("<Compiler Instance Data>", separated.stderr)

    @unittest.skipUnless(shutil.which("zstd"), "zstd is required to verify compressed output")
    def test_zstd_output_decompresses_to_equivalent_protocol(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            plain_output = Path(directory) / "ast.dump"
            compressed_output = Path(directory) / "ast.dump.zst"

            plain = self.run_tool(self.source, plain_output)
            command = [
                str(clang_dumper_tool()),
                "-id=42",
                "-system-header-threshold=1",
                "-c",
                "-o",
                str(compressed_output),
                "-ast-dump-compression=zstd",
                str(self.source),
                "--",
            ]
            compressed = subprocess.run(
                command, capture_output=True, text=True, check=False
            )
            decompressed = subprocess.run(
                ["zstd", "-q", "-d", "-c", str(compressed_output)],
                capture_output=True,
                check=False,
            )

            self.assertEqual(plain.returncode, 0, plain.stderr)
            self.assertEqual(compressed.returncode, 0, compressed.stderr)
            self.assertEqual(decompressed.returncode, 0, decompressed.stderr)
            source_root = str(self.source.parent.resolve())
            plain_dump, _ = normalize_captured_output(
                plain_output.read_text(encoding="utf-8"), source_root
            )
            compressed_dump, _ = normalize_captured_output(
                decompressed.stdout.decode("utf-8"), source_root
            )
            self.assertEqual(compressed_dump, plain_dump)

    def test_diagnostics_remain_on_stderr(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast dump with spaces.dump"
            result = self.run_tool(self.throwing_source, output)
            dump = output.read_text(encoding="utf-8")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("error:", result.stderr)
            self.assertNotIn("<Compiler Instance Data>", result.stderr)
            self.assertIn("<Compiler Instance Data>", dump)

    def test_file_output_truncates_existing_contents(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast.dump"
            output.write_text("stale data\n", encoding="utf-8")
            result = self.run_tool(self.source, output)
            self.assertEqual(result.returncode, 0)
            self.assertNotIn("stale data", output.read_text(encoding="utf-8"))

    def test_bad_file_output_path_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "missing" / "ast.dump"
            result = self.run_tool(self.source, output)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Cannot open AST dump output", result.stderr)

    def test_writes_make_dependencies_for_ccache_depend_mode(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast dump.output"
            dependencies = Path(directory) / "ast dump.d"
            command = [
                str(clang_dumper_tool()),
                "-c",
                str(Path(__file__).parent / "inputs" / "includes.cpp"),
                "-o",
                str(output),
                "-MD",
                "-MF",
                str(dependencies),
                "-id=42",
                "-system-header-threshold=1",
                "--",
            ]
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            depfile = dependencies.read_text(encoding="utf-8")
            self.assertIn(str(output).replace(" ", "\\ "), depfile)
            self.assertIn("includes.cpp", depfile)
            self.assertIn("includes.h", depfile)
            self.assertIn("includes2.h", depfile)
            self.assertIn("data1.dat", depfile)

    def test_accepts_ccache_canonicalized_argument_order(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast.dump"
            dependencies = Path(directory) / "ast.d"
            command = [
                str(clang_dumper_tool()),
                "-MD",
                "-MF",
                str(dependencies),
                "-id=42",
                "-system-header-threshold=1",
                "-std=c++17",
                "-fcolor-diagnostics",
                "-c",
                "-o",
                str(output),
                "--",
                str(self.source),
            ]
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(output.is_file())
            self.assertTrue(dependencies.is_file())

    @unittest.skipUnless(shutil.which("ccache"), "ccache is required for cache integration tests")
    def test_ccache_restores_compressed_dump_without_rerunning_tool(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = root / "ast.dump.zst"
            dependencies = root / "ast.d"
            environment = os.environ.copy()
            environment.update(
                CCACHE_DIR=str(root / "cache"),
                CCACHE_COMPILERTYPE="clang",
                CCACHE_DEPEND="true",
                CCACHE_NOHASHDIR="true",
                CCACHE_NOCOMPRESS="true",
            )
            command = [
                "ccache",
                str(clang_dumper_tool()),
                "-c",
                str(self.source),
                "-id=42",
                "-system-header-threshold=1",
                "-o",
                str(output),
                "-ast-dump-compression=zstd",
                "-MD",
                "-MF",
                str(dependencies),
                "--",
            ]

            first = subprocess.run(
                command, capture_output=True, text=True, check=False, env=environment
            )
            self.assertEqual(first.returncode, 0, first.stderr)
            first_dump = output.read_bytes()
            output.unlink()
            dependencies.unlink()
            second = subprocess.run(
                command, capture_output=True, text=True, check=False, env=environment
            )
            stats = subprocess.run(
                ["ccache", "--print-stats"],
                capture_output=True,
                text=True,
                check=True,
                env=environment,
            ).stdout

            self.assertEqual(second.returncode, 0, second.stderr)
            self.assertEqual(output.read_bytes(), first_dump)
            self.assertIn("direct_cache_hit\t1", stats)
            self.assertIn("cache_miss\t1", stats)


class ClangDiagnosticFilteringTest(unittest.TestCase):
    def test_removes_interleaved_diagnostics_without_touching_protocol(self) -> None:
        output = """protocol before
/tmp/input.c:3:4: warning: example warning
    3 | bad();
      | ^~~~~
protocol after
1 warning generated.
"""
        self.assertEqual(
            "protocol before\nprotocol after\n",
            strip_clang_diagnostics(output),
        )


def source_record(
    expansion_path: str,
    source: str,
    *,
    spelling_path: str | None = None,
    system_header: bool = False,
) -> str:
    lines = [
        "<IntegerLiteralData>",
        "ADDR_001",
        "IntegerLiteral",
        expansion_path,
        "4",
        "8",
        "<end>",
        "1" if spelling_path is not None else "0",
    ]
    if spelling_path is not None:
        lines.extend([spelling_path, "2", "3", "<end>"])
    lines.extend(
        [
            "1" if system_header else "0",
            "ADDR_002",
            "0",
            "0",
            "0",
            "%CLAVA_SOURCE_BEGIN%",
            source,
            "%CLAVA_SOURCE_END%",
            "42",
        ]
    )
    return "\n".join(lines)


class NormalizeSystemSourceBlocksTest(unittest.TestCase):
    def test_preserves_test_file_source(self) -> None:
        output = source_record("<TEST_DIR>/literal.c", "1.2")
        self.assertEqual(normalize_system_source_blocks(output), output)

    def test_normalizes_direct_system_header_source(self) -> None:
        output = source_record(
            "<SYSTEM_INCLUDE>/stdlib.h",
            "system\nheader\ntext",
            system_header=True,
        )
        self.assertIn(
            "%CLAVA_SYSTEM_SOURCE_BLOCK%",
            normalize_system_source_blocks(output),
        )
        self.assertNotIn(
            "system\nheader\ntext",
            normalize_system_source_blocks(output),
        )

    def test_normalizes_macro_spelled_in_compiler_header(self) -> None:
        output = source_record(
            "<TEST_DIR>/boolean.c",
            "1",
            spelling_path="<CLANG_INCLUDE>/stdbool.h",
        )
        self.assertIn(
            "%CLAVA_SYSTEM_SOURCE_BLOCK%",
            normalize_system_source_blocks(output),
        )

    def test_normalizes_internal_compiler_source(self) -> None:
        output = source_record("<built-in>", "__DBL_MAX__")
        self.assertIn(
            "%CLAVA_SYSTEM_SOURCE_BLOCK%",
            normalize_system_source_blocks(output),
        )

    def test_uses_system_header_flag_for_unrecognized_paths(self) -> None:
        output = source_record(
            "/vendor/sdk/header.h",
            "VENDOR_MACRO",
            system_header=True,
        )
        self.assertIn(
            "%CLAVA_SYSTEM_SOURCE_BLOCK%",
            normalize_system_source_blocks(output),
        )

    def test_preserves_macro_spelled_in_test_file(self) -> None:
        output = source_record(
            "<built-in>",
            "TEST_MACRO",
            spelling_path="<TEST_DIR>/macro.h",
        )
        self.assertEqual(normalize_system_source_blocks(output), output)

    def test_preserves_invalid_empty_source(self) -> None:
        output = "\n".join(
            [
                "<IntegerLiteralData>",
                "ADDR_001",
                "IntegerLiteral",
                "<invalid>",
                "0",
                "1",
                "ADDR_002",
                "0",
                "0",
                "0",
                "%CLAVA_SOURCE_BEGIN%",
                "",
                "%CLAVA_SOURCE_END%",
            ]
        )
        self.assertEqual(normalize_system_source_blocks(output), output)

    def test_does_not_apply_previous_record_provenance(self) -> None:
        system_record = source_record(
            "<SYSTEM_INCLUDE>/stdlib.h",
            "SYSTEM_TEXT",
            system_header=True,
        )
        test_record = source_record("<TEST_DIR>/literal.c", "LOCAL_TEXT")
        normalized = normalize_system_source_blocks(system_record + "\n" + test_record)
        self.assertNotIn("SYSTEM_TEXT", normalized)
        self.assertIn("LOCAL_TEXT", normalized)


class NormalizeSystemPathsTest(unittest.TestCase):
    def test_normalizes_entrypoint_windows_include_archive_paths(self) -> None:
        output = "\n".join(
            [
                r"C:\a\clang-dumper\clang-dumper\windows-includes\mingw\c++\v1\vector",
                r"C:\a\clang-dumper\clang-dumper\windows-includes\clang\stddef.h",
                r"C:\a\clang-dumper\clang-dumper\windows-includes\mingw\stdio.h",
            ]
        )

        normalized, _ = normalize_captured_output(output, "<TEST_DIR>")

        self.assertIn("<SYSTEM_INCLUDE>/c++", normalized)
        self.assertIn("<CLANG_INCLUDE>", normalized)
        self.assertIn("<SYSTEM_INCLUDE>", normalized)
        self.assertNotIn("windows-includes", normalized)


class NormalizeUnsignedLongLongTest(unittest.TestCase):
    def test_preserves_real_unsigned_long_long_builtin(self) -> None:
        output = "\n".join(
            [
                "<BuiltinTypeData>",
                "ADDR_001",
                "BuiltinType",
                "unsigned long long",
                "NONE",
                "0",
                "0",
                "0",
                "nullptr_type",
                "ULongLong",
                "unsigned long long",
                "<Id to Class Map>",
                "ADDR_001",
                "BuiltinType",
                "<Visited Children>",
                "ADDR_002",
                "0",
                "<VarDeclData>",
                "ADDR_002",
                "VarDecl",
                "<TEST_DIR>/builtin_types.cpp",
                "12",
                "2",
                "<TEST_DIR>/builtin_types.cpp",
                "12",
                "21",
                "0",
                "0",
                "0",
                "0",
                "0",
                "0",
                "0",
                "0",
                "",
                "unsignedLongLong",
                "0",
                "0",
                "0",
                "None",
                "Default",
                "ADDR_001",
            ]
        )

        self.assertEqual(normalize_static_output(output), output)

    def test_normalizes_external_unsigned_long_long_typedef_builtin(self) -> None:
        output = "\n".join(
            [
                "<BuiltinTypeData>",
                "ADDR_001",
                "BuiltinType",
                "unsigned long long",
                "NONE",
                "0",
                "0",
                "0",
                "nullptr_type",
                "ULongLong",
                "unsigned long long",
                "<Id to Class Map>",
                "ADDR_001",
                "BuiltinType",
                "<Visited Children>",
                "ADDR_002",
                "0",
                "<TypedefNameDeclData>",
                "ADDR_002",
                "TypedefDecl",
                "<SYSTEM_INCLUDE>/stdint.h",
                "27",
                "1",
                "<SYSTEM_INCLUDE>/stdint.h",
                "27",
                "20",
                "0",
                "1",
                "0",
                "0",
                "1",
                "0",
                "0",
                "0",
                "",
                "uint64_t",
                "0",
                "0",
                "0",
                "None",
                "Default",
                "ADDR_003",
                "ADDR_001",
            ]
        )

        normalized = normalize_static_output(output)

        self.assertIn("\nunsigned long\n", normalized)
        self.assertIn("\nULong\n", normalized)
        self.assertNotIn("\nunsigned long long\n", normalized)
        self.assertNotIn("\nULongLong\n", normalized)


class NodeClosureTest(unittest.TestCase):
    def test_accepts_resolved_node_ids(self) -> None:
        output = "\n".join(
            [
                "<TypedefNameDeclData>",
                "ADDR_001",
                "ADDR_002",
                "<Id to Class Map>",
                "ADDR_001",
                "TypedefDecl",
                "<Id to Class Map>",
                "ADDR_002",
                "BuiltinType",
            ]
        )

        self.assertEqual(unresolved_node_ids(output), [])

    def test_reports_unresolved_node_ids(self) -> None:
        output = "\n".join(
            [
                "<TypedefNameDeclData>",
                "ADDR_001",
                "ADDR_002",
                "<Top Level Attributes>",
                "ADDR_003",
                "<Id to Class Map>",
                "ADDR_001",
                "TypedefDecl",
            ]
        )

        self.assertEqual(unresolved_node_ids(output), ["ADDR_002", "ADDR_003"])


if __name__ == "__main__":
    unittest.main()
