#!/usr/bin/env python3

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from run_tests import normalize_captured_output, normalize_static_output, normalize_system_source_blocks


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


if __name__ == "__main__":
    unittest.main()
