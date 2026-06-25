#!/usr/bin/env python3
"""
Test runner for clang-dumper tool and plugin.

Runs the tool or plugin on sample C/C++ source files, normalizes memory addresses
in the output, and compares against expected baseline files.

Usage:
    # Tool mode (default)
    python run_tests.py --mode tool --path /path/to/tool
    python run_tests.py --mode tool --path /path/to/tool --generate

    # Plugin mode
    python run_tests.py --mode plugin --path /path/to/plugin.so --clang-path /path/to/clang
    python run_tests.py --mode plugin --path /path/to/plugin.so --clang-path /path/to/clang --generate
"""

import argparse
import json
import os
import platform
import re
import shlex
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Literal, Optional, get_args

# Type alias for mode
Mode = Literal["tool", "plugin"]


@dataclass
class TestConfig:
    """Configuration for a single test file."""

    id: int = 0
    flags: list[str] = field(default_factory=list)
    requires: set[str] = field(default_factory=set)


# Helper to create simple test configs
def T(
    id: int = 0,
    flags: Optional[list[str]] = None,
    requires: Optional[set[str]] = None,
) -> TestConfig:
    """Shorthand for creating TestConfig instances."""
    return TestConfig(
        id=id,
        flags=flags or [],
        requires=requires or set(),
    )


# Test registry with per-test configuration
# Every test file MUST have an entry here - no default fallback to catch typos
# Use T() helper: T(id, flags=[...], requires={...})
TEST_REGISTRY: dict[str, TestConfig] = {
    "simple_function.cpp": T(42),
    "class_decl.cpp": T(17),
    "expressions.cpp": T(73),
    "2mm.c": T(requires={"posix"}),
    "2mm.h": T(),
    "ArrayInitLoopExpr.cpp": T(),
    "ComplexType.cpp": T(),
    "OMPParallelForDirective.cpp": T(),
    "Routing.cpp": T(),
    "ShortcutPosition.h": T(flags=["-x", "c++"]),
    "TemplateTemplateParmDecl.cpp": T(),
    "VectorType.cpp": T(),
    "array_filler.c": T(),
    "ast-dump-c-attr.c": T(),
    "ast-dump-expr.c": T(requires={"x86"}),
    "ast-dump-records.c": T(),
    "ast-dump-stmt.c": T(),
    "ast-print-bool.c": T(flags=["-DDEF_BOOL_CBOOL"]), # ["-DDEF_BOOL_INT"]
    "ast-print-bool.cpp": T(),
    "ast-print-enum-decl.c": T(),
    "ast-print-record-decl.c": T(flags=["-DKW=struct", "-DBASES="]), # ["-DKW=union", "-DBASES="]
    "ast-print-record-decl.cpp": T(flags=["-DKW=struct", "-DBASES="]),
    "atom.cpp": T(),
    "atomicAdd.cu": T(requires={"cuda"}),
    "attr-target-ast.c": T(),
    "attribute.cpp": T(),
    "blocked_mm.cpp": T(),
    "boolean.c": T(),
    "boolean.cpp": T(),
    "boolean2.c": T(),
    "builtin_types.cl": T(requires={"opencl"}),
    "builtin_types.cpp": T(),
    "c-casts.c": T(),
    "c89.c": T(),
    "c99.c": T(),
    "character.cpp": T(),
    "cl_attribute.cl": T(requires={"opencl"}),
    "class_template.cpp": T(),
    "class_template.h": T(flags=["-x", "c++"]),
    "classes.cpp": T(),
    "clava_issue09.h": T(flags=["-x", "c++"]),
    "clava_issue10.cpp": T(),
    "clava_issue11.cpp": T(),
    "clava_issue13.cpp": T(),
    "clava_issue14.h": T(flags=["-x", "c++"]),
    "clava_issue15.cpp": T(requires={"x86"}),
    "clava_issue17.cpp": T(),
    "clava_issue18.cpp": T(),
    "clava_issue19.cpp": T(),
    "clava_issue20.cpp": T(),
    "clava_issue21.cpp": T(),
    "clava_issue24.cpp": T(),
    "clava_issue25.cpp": T(),
    "clava_issue26.cpp": T(),
    "clava_issue27.cpp": T(flags=["-std=c++14"]),
    "clava_issue28.cpp": T(),
    "clava_issue28.h": T(flags=["-x", "c++"]),
    "clava_issue29.cpp": T(),
    "clava_issue39.cpp": T(),
    "clava_issue40.cpp": T(),
    "comment.cpp": T(),
    "comment_include.cpp": T(),
    "compound_literal.c": T(),
    "constructor.cpp": T(),
    "constructor.h": T(flags=["-x", "c++"]),
    "convolution_cache.cu": T(requires={"cuda"}),
    "dbl_max.cpp": T(),
    "decl.c": T(),
    "decl.cpp": T(),
    "default.h": T(flags=["-x", "c++"]),
    "dependent_scope_decl_ref_expr.cpp": T(),
    "destructor.cpp": T(),
    "dummy.cpp": T(),
    "enum.c": T(),
    "enum.cpp": T(),
    "enum.h": T(),
    "enum.hpp": T(),
    "exceptions.cpp": T(),
    "fast_stack.cpp": T(),
    "fixed_point.c": T(flags=["-ffixed-point"]),
    "fixed_point_to_string.c": T(flags=["-ffixed-point"]),
    "for.cpp": T(),
    "friend.cpp": T(),
    "functions.cpp": T(),
    "gnu_extensions.cpp": T(),
    "gnu_stmt_expr.c": T(),
    "goto.c": T(),
    "if.cpp": T(),
    "implicit-cast-dump.c": T(),
    "implicit_reference.cpp": T(),
    "includes.cpp": T(),
    "includes.h": T(flags=["-x", "c++"]),
    "includes2.cpp": T(),
    "includes2.h": T(),
    "labels.c": T(),
    "lambda.cpp": T(),
    "literals.cpp": T(),
    "macro.c": T(),
    "macro.h": T(),
    "member_calls.cpp": T(),
    "mini_logger.hpp": T(),
    "mult_matrix.cu": T(requires={"cuda"}),
    "multiple_clauses_omp_pragmas.cpp": T(),
    "multistep-explicit-cast.c": T(),
    "naked_loops.c": T(),
    "namespace.cpp": T(),
    "namespace.h": T(flags=["-x", "c++"]),
    "namespacealias.cpp": T(),
    "nas_bt.c": T(requires={"posix"}),
    "nas_ft.c": T(requires={"posix"}),
    "nas_lu.c": T(requires={"posix"}),
    "nas_ua.c": T(requires={"posix"}),
    "new.cpp": T(),
    "noexcept.cpp": T(),
    "offset.c": T(),
    "offset.cpp": T(),
    "omp_pragmas.cpp": T(),
    "operator.cpp": T(),
    "pair_hash.cpp": T(),
    "pair_hash.h": T(flags=["-x", "c++"]),
    "pointer_to_member_operators.cpp": T(),
    "polybench.h": T(),
    "pragmas.cpp": T(),
    "predefined.c": T(),
    "problematic_operator.cpp": T(),
    "pseudo_destructor.cpp": T(),
    "qualifiers.cpp": T(),
    "rdr6094103-unordered-compare-promote.c": T(),
    "scope.cpp": T(),
    "sizeof.c": T(),
    "sizeof.cpp": T(),
    "sorted_id.cpp": T(),
    "sorted_id.h": T(flags=["-x", "c++"]),
    "streamAdd.cu": T(requires={"cuda"}),
    "strings.cpp": T(),
    "struct.c": T(),
    "struct.cpp": T(),
    "struct2.c": T(),
    "sumArrays.cu": T(requires={"cuda"}),
    "switch.c": T(),
    "template_auto.cpp": T(),
    "template_expansion_pack.cpp": T(),
    "templates.cpp": T(),
    "templates.h": T(flags=["-x", "c++"]),
    "test_includes.c": T(),
    "test_includes.cpp": T(),
    "throw.cpp": T(flags=["-std=c++14"]),
    "types.c": T(),
    "types.cpp": T(),
    "using.cpp": T(),
    "variadic-promotion.c": T(),
    "variadic.c": T(requires={"x86"}),
    "while.cpp": T(),
}

# Placeholders for normalized paths
PATH_PLACEHOLDER = "<TEST_DIR>"
SYSTEM_INCLUDE_PLACEHOLDER = "<SYSTEM_INCLUDE>"
CLANG_INCLUDE_PLACEHOLDER = "<CLANG_INCLUDE>"
GCC_INCLUDE_PLACEHOLDER = "<GCC_INCLUDE>"
CUDA_TEST_FLAGS = ["--no-cuda-version-check", "--cuda-host-only"]

# System header path normalization patterns
# These patterns replace platform-specific paths with portable placeholders
# Organized as (pattern, replacement) tuples - order matters for specificity
_SYSTEM_PATH_PATTERNS: list[tuple[str, str]] = [
    # ==================== CLANG BUILTIN HEADERS ====================
    # Linux: Various Clang installation layouts
    (r"/usr/lib/llvm-\d+/lib/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),
    (r"/usr/include/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),
    (r"/usr/lib/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),

    # macOS: Homebrew and Xcode Clang installations
    (r"/usr/local/opt/llvm@?\d*/lib/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),
    (r"/usr/local/Cellar/llvm@?\d*/[\d.]+/lib/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),
    (r"/opt/homebrew/opt/llvm@?\d*/lib/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),
    (r"/opt/homebrew/Cellar/llvm@?\d*/[\d.]+/lib/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),
    (r"/Applications/Xcode[^/]*\.app/.+/lib/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),

    # Windows: MSYS2/MinGW and LLVM installations
    (r"[A-Za-z]:[/\\]msys64[/\\]mingw\d+[/\\]lib[/\\]clang[/\\][\d.]+[/\\]include", CLANG_INCLUDE_PLACEHOLDER),
    (r"[A-Za-z]:[/\\]Program Files[/\\]LLVM[/\\]lib[/\\]clang[/\\][\d.]+[/\\]include", CLANG_INCLUDE_PLACEHOLDER),
    (r"[A-Za-z]:[/\\]mingw64-clang-\d+[/\\]lib[/\\]clang[/\\][\d.]+[/\\]include", CLANG_INCLUDE_PLACEHOLDER),
    # Windows: Custom LLVM installation paths (e.g., CI environments)
    (r"[A-Za-z]:[/\\]llvm[/\\]lib[/\\]clang[/\\][\d.]+[/\\]include", CLANG_INCLUDE_PLACEHOLDER),

    # Windows packaged include bundles used by CI and releases.
    (r"[A-Za-z]:[/\\][^\r\n]*[/\\]windows-includes[/\\]01-libcxx", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"[A-Za-z]:[/\\][^\r\n]*[/\\]windows-includes[/\\]02-clang", CLANG_INCLUDE_PLACEHOLDER),
    (r"[A-Za-z]:[/\\][^\r\n]*[/\\]windows-includes[/\\]03-mingw", SYSTEM_INCLUDE_PLACEHOLDER),

    # ==================== GCC HEADERS ====================
    # Linux: GCC's libstdc++ headers are often reported through a target-triple
    # relative path rooted under /usr/lib/gcc.
    (r"/usr/bin/\.\./lib/gcc/[^/]+/[\d.]+/\.\./\.\./\.\./\.\./include/[^/]+-linux-gnu/c\+\+/[\d.]+", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/usr/bin/\.\./lib/gcc/[^/]+/[\d.]+/\.\./\.\./\.\./\.\./include/c\+\+/[\d.]+", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/usr/bin/\.\./lib/gcc/[^/]+/[\d.]+/\.\./\.\./\.\./\.\./include/[^/]+-linux-gnu", SYSTEM_INCLUDE_PLACEHOLDER),
    (r"/usr/bin/\.\./lib/gcc/[^/]+/[\d.]+/\.\./\.\./\.\./\.\./include", SYSTEM_INCLUDE_PLACEHOLDER),
    (r"/usr/lib/gcc/[^/]+/[\d.]+/\.\./\.\./\.\./\.\./include/[^/]+-linux-gnu/c\+\+/[\d.]+", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/usr/lib/gcc/[^/]+/[\d.]+/\.\./\.\./\.\./\.\./include/c\+\+/[\d.]+", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/usr/lib/gcc/[^/]+/[\d.]+/\.\./\.\./\.\./\.\./include/[^/]+-linux-gnu", SYSTEM_INCLUDE_PLACEHOLDER),
    (r"/usr/lib/gcc/[^/]+/[\d.]+/\.\./\.\./\.\./\.\./include", SYSTEM_INCLUDE_PLACEHOLDER),
    # Linux: Canonicalize remaining GCC lib paths that are not system includes.
    (r"/usr/bin/\.\./lib/gcc/", "/usr/lib/gcc/"),

    # macOS: GCC from Homebrew
    (r"/usr/local/Cellar/gcc/[\d.]+/lib/gcc/.+/include", GCC_INCLUDE_PLACEHOLDER),
    (r"/opt/homebrew/Cellar/gcc/[\d.]+/lib/gcc/.+/include", GCC_INCLUDE_PLACEHOLDER),

    # Windows: MinGW GCC
    (r"[A-Za-z]:[/\\]msys64[/\\]mingw\d+[/\\]lib[/\\]gcc[/\\][^/\\]+[/\\][\d.]+[/\\]include", GCC_INCLUDE_PLACEHOLDER),
    (r"[A-Za-z]:[/\\]mingw64-clang-\d+[/\\]lib[/\\]gcc[/\\][^/\\]+[/\\][\d.]+[/\\]include", GCC_INCLUDE_PLACEHOLDER),

    # ==================== SYSTEM C/C++ HEADERS ====================
    # Linux: Standard system includes
    (r"/usr/include/c\+\+/[\d.]+", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/usr/include/[^/]+-linux-gnu/c\+\+/[\d.]+", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/usr/include/[^/]+-linux-gnu", SYSTEM_INCLUDE_PLACEHOLDER),

    # macOS: SDK and system headers
    (r"/usr/local/opt/llvm@?\d*/include/c\+\+/v1", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/usr/local/Cellar/llvm@?\d*/[\d.]+/include/c\+\+/v1", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/opt/homebrew/opt/llvm@?\d*/include/c\+\+/v1", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/opt/homebrew/Cellar/llvm@?\d*/[\d.]+/include/c\+\+/v1", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/Library/Developer/CommandLineTools/SDKs/MacOSX[\d.]*\.sdk/usr/include/(?:arm|i386)", SYSTEM_INCLUDE_PLACEHOLDER + "/arch"),
    (r"/Library/Developer/CommandLineTools/SDKs/MacOSX[\d.]*\.sdk/usr/include/c\+\+/v1", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/Library/Developer/CommandLineTools/SDKs/MacOSX[\d.]*\.sdk/usr/include", SYSTEM_INCLUDE_PLACEHOLDER),
    (r"/Applications/Xcode[^/]*\.app/.+/SDKs/MacOSX[\d.]*\.sdk/usr/include/(?:arm|i386)", SYSTEM_INCLUDE_PLACEHOLDER + "/arch"),
    (r"/Applications/Xcode[^/]*\.app/.+/SDKs/MacOSX[\d.]*\.sdk/usr/include/c\+\+/v1", SYSTEM_INCLUDE_PLACEHOLDER + "/c++"),
    (r"/Applications/Xcode[^/]*\.app/.+/SDKs/MacOSX[\d.]*\.sdk/usr/include", SYSTEM_INCLUDE_PLACEHOLDER),

    # Windows: MSVC and Windows SDK headers
    (r"[A-Za-z]:[/\\]Program Files[/\\]Microsoft Visual Studio[/\\][^/\\]+[/\\][^/\\]+[/\\]VC[/\\]Tools[/\\]MSVC[/\\][\d.]+[/\\]include", SYSTEM_INCLUDE_PLACEHOLDER),
    (r"[A-Za-z]:[/\\]Program Files \(x86\)[/\\]Windows Kits[/\\]\d+[/\\]Include[/\\][\d.]+[/\\]\w+", SYSTEM_INCLUDE_PLACEHOLDER),

    # Windows: MinGW system includes
    (r"[A-Za-z]:[/\\]msys64[/\\]mingw\d+[/\\]include", SYSTEM_INCLUDE_PLACEHOLDER),
    (r"[A-Za-z]:[/\\]msys64[/\\]mingw\d+[/\\][^/\\]+-w64-mingw32[/\\]include", SYSTEM_INCLUDE_PLACEHOLDER),
    (r"[A-Za-z]:[/\\]mingw64-clang-\d+[/\\]include", SYSTEM_INCLUDE_PLACEHOLDER),

    # Generic /usr/include (should be last for Linux paths)
    (r"/usr/include(?=/[^/])", SYSTEM_INCLUDE_PLACEHOLDER),
]

# Build a single combined regex pattern at module load time for performance
# Each pattern becomes a named group, and we use a lookup table for replacements
def _build_combined_pattern() -> tuple[re.Pattern[str], dict[str, str]]:
    """
    Build a single combined regex from all system path patterns AND address pattern.
    This enables single-pass normalization for better performance on large outputs.
    
    Returns:
        tuple: (compiled_pattern, group_to_replacement_map)
    """
    groups = []
    group_map = {}
    
    # Add system path patterns first (more specific, should match before generic patterns)
    for i, (pattern, replacement) in enumerate(_SYSTEM_PATH_PATTERNS):
        group_name = f"syspath{i}"
        groups.append(f"(?P<{group_name}>{pattern})")
        group_map[group_name] = replacement
    
    # Add address pattern - replacement is dynamic, so we use a sentinel
    # Matches both formats:
    #   - Linux/macOS: 0x7f1234abcd_0 (with 0x prefix, variable length)
    #   - Windows: 0000022070407AD0_0 (no prefix, exactly 16 hex digits for 64-bit pointers)
    # The Windows pattern requires exactly 16 hex digits to avoid false positives like "x86_64"
    groups.append(r"(?P<addr>(?:0x[0-9a-fA-F]+|[0-9a-fA-F]{16})_\d+)")
    group_map["addr"] = None  # Sentinel: handled specially in replacement function
    
    combined = "|".join(groups)
    return re.compile(combined), group_map

# Pre-compiled at module load time
_UNIFIED_REGEX, _UNIFIED_REPLACEMENTS = _build_combined_pattern()

# Line offsets (0-indexed from <Compiler Instance Data>) for platform-specific type widths
# These widths can differ between platforms and need normalization
_TYPE_WIDTH_LINE_OFFSETS = {
    26: "<LONG_DOUBLE_WIDTH>",  # LongDoubleWidth: 128 (Linux x86_64) vs 64 (Windows MSVC)
    30: "<LONG_WIDTH>",         # LongWidth: 64 (Linux LP64) vs 32 (Windows LLP64)
}

_ADDR_PLACEHOLDER_RE = re.compile(r"^ADDR_\d+$")

_TARGET_ATTR_WARNING_RE = re.compile(
    r"warning: (?:unknown CPU 'hiss'|duplicate 'arch=') in the 'target' "
    r"attribute string; 'target' attribute ignored \[-Wignored-attributes\]"
)
_CLANG_DIAGNOSTIC_PREFIX_RE = re.compile(
    r"^clang(?:\+\+)?-\d+:\s+(?=(?:error|warning|note):)"
)

# AArch64 treats plain char as unsigned by default, while x86_64 treats it as
# signed. The tests exercise AST shape, not the host default-char ABI.
_PLAIN_CHAR_KIND_RE = re.compile(r"^Char_[SU]$", re.MULTILINE)
_WIDE_CHAR_KIND_RE = re.compile(r"^WChar_[SU]$", re.MULTILINE)
_UNSIGNED_INT_ARRAY_RE = re.compile(r"^unsigned int\[(\d+)\]$")
_DRIVE_PREFIXED_PLACEHOLDER_RE = re.compile(
    rf"\b[A-Za-z]:(?={re.escape(PATH_PLACEHOLDER)}|"
    rf"{re.escape(SYSTEM_INCLUDE_PLACEHOLDER)}|"
    rf"{re.escape(CLANG_INCLUDE_PLACEHOLDER)}|"
    rf"{re.escape(GCC_INCLUDE_PLACEHOLDER)})"
)
_DIAGNOSTIC_RE = re.compile(
    r"^<TEST_DIR>/[^\n]+: warning: [^\n]+\n"
    r"(?:[ \t]*\d+ \|[^\n]*\n)?"
    r"(?:[ \t]*\|[^\n]*\n)*",
    re.MULTILINE,
)
_SOURCE_BLOCK_RE = re.compile(
    r"^%CLAVA_SOURCE_BEGIN%\n"
    r"(?:.*\n)*?"
    r"^%CLAVA_SOURCE_END%\n?",
    re.MULTILINE,
)
_UNSIGNED_LONG_LONG_LINE_RE = re.compile(r"^unsigned long long$", re.MULTILINE)
_ULONG_LONG_KIND_RE = re.compile(r"^ULongLong$", re.MULTILINE)
_INTERNAL_BUFFER_LINE_RE = re.compile(
    r"^(<(?:built-in|command line|scratch space)>)\n\d+\n(\d+)$",
    re.MULTILINE,
)
_ANON_DECL_NAME_RE = re.compile(r"^(\n)(\d+)(\n12\n)", re.MULTILINE)
_WINDOWS_ADDR_CANDIDATE_RE = re.compile(r"\b[0-9a-fA-F]{16}_\d+\b")


def canonical_raw_address(raw_address: str) -> str:
    """Return a stable key for raw address tokens across platform spellings."""
    pointer, suffix = raw_address.rsplit("_", 1)
    pointer = pointer.lower()
    if pointer.startswith("0x"):
        pointer = pointer[2:]
    pointer = pointer.lstrip("0") or "0"
    return f"{pointer}_{suffix}"


def normalize_wide_string_literals(output: str) -> str:
    """
    Normalize target-dependent WIDE string literal byte payloads.

    Clang reports wide character byte width and bytes according to the target
    ABI. The tests care about AST shape and string kind/length, not whether the
    target uses 2-byte or 4-byte wide characters.
    """
    lines = output.split("\n")
    normalized: list[str] = []
    i = 0
    while i < len(lines):
        normalized.append(lines[i])

        if (
            lines[i] == "WIDE"
            and i + 3 < len(lines)
            and lines[i + 1].isdigit()
            and lines[i + 2].isdigit()
            and lines[i + 3].isdigit()
        ):
            byte_count = int(lines[i + 3])
            normalized.append(lines[i + 1])
            normalized.append("<WIDE_CHAR_WIDTH>")
            normalized.append("<WIDE_STRING_BYTES>")
            i += 4 + byte_count
            continue

        i += 1

    return "\n".join(normalized)

def normalize_type_widths(output: str) -> str:
    """
    Normalize platform-specific type width values in the output.
    
    Different platforms have different type sizes:
    - LongDoubleWidth (line 27): 128 bits on Linux x86_64, 64 bits on Windows MSVC
    - LongWidth (line 31): 64 bits on Linux (LP64), 32 bits on Windows (LLP64)
    
    The output format has these values as bare numbers at specific line offsets
    from the <Compiler Instance Data> marker. This function replaces those
    values with placeholders to ensure cross-platform test compatibility.
    """
    marker = "<Compiler Instance Data>\n"
    marker_idx = output.find(marker)
    if marker_idx < 0:
        return output

    rest_start = marker_idx + len(marker)
    max_offset = max(_TYPE_WIDTH_LINE_OFFSETS)
    # Offsets are relative to the marker line. Split only the small prefix that
    # contains the fields we patch, not the whole AST dump.
    lines = output[rest_start:].split("\n", max_offset + 1)
    changed = False
    for offset, placeholder in _TYPE_WIDTH_LINE_OFFSETS.items():
        target_idx = offset - 1
        if target_idx < len(lines) and lines[target_idx].strip().isdigit():
            lines[target_idx] = placeholder
            changed = True

    if not changed:
        return output
    return output[:rest_start] + "\n".join(lines)


def normalize_plain_char_arrays(output: str) -> str:
    """
    Normalize host-dependent plain-char array spellings without rewriting real
    unsigned-int arrays. A ConstantArrayType spelling is only treated as a
    plain-char artifact when its element type points at a BuiltinType for char.
    """
    lines = output.split("\n")
    plain_char_type_ids: set[str] = set()

    for i, line in enumerate(lines):
        if (
            line == "<BuiltinTypeData>"
            and i + 11 < len(lines)
            and lines[i + 2] == "BuiltinType"
            and lines[i + 3] == "char"
            and lines[i + 10] == "char"
        ):
            plain_char_type_ids.add(lines[i + 1])

    if not plain_char_type_ids:
        return output

    for i, line in enumerate(lines):
        match = _UNSIGNED_INT_ARRAY_RE.fullmatch(line)
        if (
            match
            and i >= 3
            and lines[i - 3] == "<ConstantArrayTypeData>"
            and i + 8 < len(lines)
            and lines[i + 8] in plain_char_type_ids
        ):
            lines[i] = f"char[{match.group(1)}]"

    return "\n".join(lines)


def normalize_static_output(output: str) -> str:
    """
    Normalize architecture- and installation-dependent text that can appear in
    freshly generated output.
    """
    output = output.replace("\r\n", "\n").replace("\r", "\n")
    if "Char_U" in output:
        output = _PLAIN_CHAR_KIND_RE.sub("Char_S", output)
    if "WChar_U" in output:
        output = _WIDE_CHAR_KIND_RE.sub("WChar_S", output)
    if "unsigned int[" in output and "<BuiltinTypeData>" in output:
        output = normalize_plain_char_arrays(output)
    if "\nWIDE\n" in output or output.startswith("WIDE\n"):
        output = normalize_wide_string_literals(output)
    if "unsigned long long" in output:
        output = _UNSIGNED_LONG_LONG_LINE_RE.sub("unsigned long", output)
    if "ULongLong" in output:
        output = _ULONG_LONG_KIND_RE.sub("ULong", output)
    if "basic_string<char>" in output:
        output = output.replace("std::basic_string<char>", "std::string")
        output = output.replace("basic_string<char>", "string")
    if "basic_ostream<char>" in output:
        output = output.replace("std::basic_ostream<char>", "std::ostream")
        output = output.replace("basic_ostream<char>", "ostream")
    if "basic_istream<char>" in output:
        output = output.replace("std::basic_istream<char>", "std::istream")
        output = output.replace("basic_istream<char>", "istream")
    if (
        "<built-in>" in output
        or "<command line>" in output
        or "<scratch space>" in output
    ):
        output = _INTERNAL_BUFFER_LINE_RE.sub(
            r"\1\n<INTERNAL_BUFFER_LINE>\n\2", output
        )
    if "\n12\n" in output:
        output = _ANON_DECL_NAME_RE.sub(r"\1<ANON_DECL_NAME>\3", output)
    if ":<" in output:
        output = _DRIVE_PREFIXED_PLACEHOLDER_RE.sub("", output)
    if "target attribute ignored [-Wignored-attributes]" in output:
        output = _TARGET_ATTR_WARNING_RE.sub(
            "warning: target attribute diagnostic normalized; "
            "target attribute ignored [-Wignored-attributes]",
            output,
        )
    if " warning: " in output and PATH_PLACEHOLDER in output:
        output = _DIAGNOSTIC_RE.sub("", output)
    if "%CLAVA_SOURCE_BEGIN%" in output:
        output = _SOURCE_BLOCK_RE.sub("%CLAVA_SOURCE_BLOCK%\n", output)
    if "<Compiler Instance Data>" in output:
        output = normalize_type_widths(output)
    return output


def line_needs_unified_regex(line: str) -> bool:
    """Return true when a raw line can contain a path or address token."""
    if "0x" in line or "/" in line or "\\" in line:
        return True
    return "_" in line and _WINDOWS_ADDR_CANDIDATE_RE.search(line) is not None


def normalize_captured_lines(
    raw_lines: Iterable[str],
    inputs_dir_str: str,
) -> tuple[str, dict[str, list[str]]]:
    """Normalize captured raw stderr lines from a dumper invocation."""
    address_map: dict[str, str] = {}
    placeholder_to_raw: dict[str, list[str]] = {}
    counter = [1]

    def unified_replacer(match: re.Match[str]) -> str:
        """Single-pass replacement function for both paths and addresses."""
        group_name = match.lastgroup
        if group_name is None:
            return match.group(0)

        if group_name == "addr":
            raw_address = match.group(0)
            address_key = canonical_raw_address(raw_address)
            if address_key not in address_map:
                placeholder = f"ADDR_{counter[0]:03d}"
                address_map[address_key] = placeholder
                placeholder_to_raw[placeholder] = [raw_address]
                counter[0] += 1
            else:
                placeholder = address_map[address_key]
                if raw_address not in placeholder_to_raw[placeholder]:
                    placeholder_to_raw[placeholder].append(raw_address)
            return placeholder

        replacement = _UNIFIED_REPLACEMENTS.get(group_name)
        if replacement is not None:
            return replacement
        return match.group(0)

    inputs_dir_str_bwd = inputs_dir_str.replace("/", "\\")
    normalized_lines: list[str] = []
    for line in raw_lines:
        line = _CLANG_DIAGNOSTIC_PREFIX_RE.sub("", line)
        line = line.replace(inputs_dir_str, PATH_PLACEHOLDER)
        line = line.replace(inputs_dir_str_bwd, PATH_PLACEHOLDER)
        line = line.replace(PATH_PLACEHOLDER + "\\", PATH_PLACEHOLDER + "/")
        if line_needs_unified_regex(line):
            line = _UNIFIED_REGEX.sub(unified_replacer, line)
        normalized_lines.append(line)

    return (
        normalize_static_output("".join(normalized_lines)),
        placeholder_to_raw,
    )


def normalize_captured_output(
    raw_output: str,
    inputs_dir_str: str,
) -> tuple[str, dict[str, list[str]]]:
    """Normalize one captured raw stderr stream from a dumper invocation."""
    return normalize_captured_lines(raw_output.splitlines(keepends=True), inputs_dir_str)


def lines_equivalent(
    expected_line: str,
    actual_line: str,
    expected_to_actual_addr: dict[str, str],
    actual_to_expected_addr: dict[str, str],
) -> bool:
    """Return True when two normalized lines are equivalent across hosts."""
    expected = expected_line.rstrip("\r\n")
    actual = actual_line.rstrip("\r\n")

    if expected == actual:
        return True

    if _ADDR_PLACEHOLDER_RE.fullmatch(expected) and _ADDR_PLACEHOLDER_RE.fullmatch(
        actual
    ):
        mapped_actual = expected_to_actual_addr.get(expected)
        mapped_expected = actual_to_expected_addr.get(actual)
        if mapped_actual is not None:
            return mapped_actual == actual
        if mapped_expected is not None:
            return mapped_expected == expected

        expected_to_actual_addr[expected] = actual
        actual_to_expected_addr[actual] = expected
        return True

    return False


def compare_normalized_outputs(
    test_name: str,
    expected_output: str,
    normalized_output: str,
) -> Optional[str]:
    """Return a failure message when two already-normalized outputs differ."""
    if normalized_output == expected_output:
        return None

    normalized_lines = normalized_output.splitlines(keepends=True)
    expected_lines = expected_output.splitlines(keepends=True)
    expected_to_actual_addr: dict[str, str] = {}
    actual_to_expected_addr: dict[str, str] = {}

    for i, (norm_line, exp_line) in enumerate(zip(normalized_lines, expected_lines), 1):
        if not lines_equivalent(
            exp_line,
            norm_line,
            expected_to_actual_addr,
            actual_to_expected_addr,
        ):
            return (
                f"Mismatch at line {i}:\n"
                f"  Expected: {exp_line.rstrip()!r}\n"
                f"  Got:      {norm_line.rstrip()!r}"
            )

    if len(normalized_lines) != len(expected_lines):
        return (
            f"Line count mismatch: expected {len(expected_lines)}, "
            f"got {len(normalized_lines)}"
        )

    return "Unknown difference"


def platform_expected_dir(test_dir: Path, baseline_platform: Optional[str]) -> Optional[Path]:
    """Return the platform baseline directory for a requested CI target."""
    if not baseline_platform:
        return None
    return test_dir / "expected-platforms" / baseline_platform


def platform_expected_dirs(test_dir: Path, baseline_platform: Optional[str]) -> list[Path]:
    """Return exact and OS-family platform baseline directories in lookup order."""
    exact_dir = platform_expected_dir(test_dir, baseline_platform)
    if exact_dir is None:
        return []

    dirs = [exact_dir]
    os_family = baseline_platform.split("-", 1)[0]
    if os_family != baseline_platform:
        dirs.append(test_dir / "expected-platforms" / os_family)
    return dirs


def resolve_expected_file(
    expected_dir: Path,
    platform_dirs: Iterable[Path],
    test_name: str,
) -> tuple[Path, str]:
    """Prefer a platform baseline when present, otherwise use the shared one."""
    for platform_dir in platform_dirs:
        platform_file = platform_dir / f"{test_name}.expected"
        if platform_file.exists():
            return platform_file, platform_dir.name
    return expected_dir / f"{test_name}.expected", "shared"


def get_test_config(test_name: str) -> TestConfig:
    """
    Get the configuration for a test file.

    Raises:
        KeyError: If the test file is not registered in TEST_REGISTRY (prevents typos)
    """
    if test_name not in TEST_REGISTRY:
        raise KeyError(
            f"Test file '{test_name}' not found in TEST_REGISTRY. "
            f"Add an entry for it in run_tests.py to register this test."
        )
    return TEST_REGISTRY[test_name]


def check_address_consistency(placeholder_to_raw: dict[str, list[str]]) -> list[str]:
    """
    Verify that each placeholder maps to exactly one raw address.

    Returns:
        List of error messages (empty if consistent)
    """
    errors = []
    for placeholder, raw_addresses in placeholder_to_raw.items():
        unique_addresses = {canonical_raw_address(address) for address in raw_addresses}
        if len(unique_addresses) > 1:
            errors.append(
                f"Inconsistent address for {placeholder}: "
                f"found {len(unique_addresses)} different addresses: {unique_addresses}"
            )
    return errors


def run_tool_and_normalize(
    mode: Mode,
    path: str,
    input_file: str,
    test_id: int,
    inputs_dir_str: str,
    clang_path: Optional[str] = None,
    extra_flags: Optional[list[str]] = None,
    system_header_threshold: Optional[int] = 0,
) -> tuple[int, str, str, str, dict[str, list[str]]]:
    """
    Run the clang-dumper tool or plugin and normalize captured stderr.

    Args:
        mode: Either "tool" or "plugin"
        path: Path to the tool executable or plugin shared library
        input_file: Path to the input source file
        test_id: The test ID for address disambiguation
        inputs_dir_str: Pre-resolved inputs directory path for normalization
        clang_path: Path to clang executable (required for plugin mode)
        extra_flags: Additional compiler flags to pass

    Returns:
        tuple: (return_code, stdout, raw_stderr, normalized_stderr, address_mapping)
    """
    flags = extra_flags or []

    if mode == "tool":
        cmd = [path, f"-id={test_id}"]
        if system_header_threshold is not None:
            cmd.append(f"-system-header-threshold={system_header_threshold}")
        cmd += [input_file, "--"] + flags
    else:
        # Plugin mode - invoke clang with the plugin loaded
        assert clang_path is not None, "clang_path required for plugin mode"
        cmd = [
            clang_path,
            f"-fplugin={path}",
            "-Xclang",
            "-plugin",
            "-Xclang",
            "DumpAst",
            "-Xclang",
            "-plugin-arg-DumpAst",
            "-Xclang",
            f"-file-id={test_id}",
        ]
        if system_header_threshold is not None:
            cmd += [
                "-Xclang",
                "-plugin-arg-DumpAst",
                "-Xclang",
                f"-system-header-threshold={system_header_threshold}",
            ]
        cmd += flags + [
            "-fsyntax-only",
            input_file,
        ]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    stdout, raw_stderr = proc.communicate()
    normalized_stderr, placeholder_to_raw = normalize_captured_output(
        raw_stderr,
        inputs_dir_str,
    )
    return proc.returncode, stdout, raw_stderr, normalized_stderr, placeholder_to_raw


def discover_tests(inputs_dir: Path) -> list[Path]:
    """Discover test input files in the inputs directory."""
    extensions = {
        ".c",
        ".cpp",
        ".cc",
        ".cxx",
        ".h",
        ".hpp",
        ".hh",
        ".hxx",
        ".cl",
        ".cu",
    }
    tests = []

    for file in sorted(inputs_dir.iterdir()):
        if file.is_file() and file.suffix in extensions:
            tests.append(file)

    return tests


# Result status for test execution
class TestStatus:
    PASS = "PASS"
    FAIL = "FAIL"
    SKIP = "SKIP"
    GENERATED = "GENERATED"


def run_single_test(
    mode: Mode,
    path: str,
    input_file: Path,
    expected_dir: Path,
    platform_expected_dirs: list[Path],
    failure_output_dir: Optional[Path],
    raw_output_dir: Optional[Path],
    inputs_dir_str: str,
    generate: bool,
    enabled_features: set[str],
    clang_path: Optional[str] = None,
    global_flags: Optional[list[str]] = None,
    system_header_threshold: Optional[int] = 0,
) -> tuple[str, str]:
    """
    Run a single test case.

    Returns:
        tuple: (status, message) where status is one of TestStatus values
    """
    test_name = input_file.name

    # Verify test is registered in TEST_REGISTRY
    try:
        config = get_test_config(test_name)
    except KeyError as e:
        return TestStatus.FAIL, str(e)

    # Check if test requirements are met
    missing_features = config.requires - enabled_features
    if missing_features:
        return (
            TestStatus.SKIP,
            f"Missing features: {', '.join(sorted(missing_features))}",
        )

    expected_file_name = f"{test_name}.expected"
    expected_file = expected_dir / expected_file_name
    expected_source = "shared"
    if not generate:
        expected_file, expected_source = resolve_expected_file(
            expected_dir,
            platform_expected_dirs,
            test_name,
        )

    missing_expected = not generate and not expected_file.exists()

    # Run the tool/plugin with streaming normalization
    flags = list(global_flags or []) + config.flags
    if input_file.suffix == ".cu":
        flags.extend(flag for flag in CUDA_TEST_FLAGS if flag not in flags)
    (
        return_code,
        stdout,
        raw_output,
        normalized_output,
        placeholder_to_raw,
    ) = run_tool_and_normalize(
        mode,
        path,
        str(input_file),
        config.id,
        inputs_dir_str,
        clang_path,
        flags,
        system_header_threshold,
    )

    if raw_output_dir is not None:
        raw_output_dir.mkdir(parents=True, exist_ok=True)
        raw_output_file = raw_output_dir / f"{test_name}.stderr"
        raw_output_file.write_text(raw_output, encoding="utf-8")

    if return_code != 0:
        if failure_output_dir is not None:
            failure_output_dir.mkdir(parents=True, exist_ok=True)
            failure_output_file = failure_output_dir / expected_file_name
            failure_output_file.write_text(normalized_output, encoding="utf-8")
        # Include stderr excerpt for debugging
        stderr_lines = normalized_output.splitlines()
        if stderr_lines:
            head_excerpt = "\n".join(stderr_lines[:50])
            tail_excerpt = "\n".join(stderr_lines[-50:])
        else:
            head_excerpt = "(empty)"
            tail_excerpt = "(empty)"
        return TestStatus.FAIL, (
            f"Tool exited with code {return_code}\n"
            f"Stderr (first 50 lines):\n{head_excerpt}\n"
            f"Stderr (last 50 lines):\n{tail_excerpt}"
        )

    # Check address consistency
    consistency_errors = check_address_consistency(placeholder_to_raw)
    if consistency_errors:
        return TestStatus.FAIL, "Address consistency errors:\n" + "\n".join(
            consistency_errors
        )

    if missing_expected:
        return TestStatus.FAIL, (
            f"Expected file not found: {expected_file}\n"
            f"Run with --generate to create it, or check if the test is properly registered."
        )

    if generate:
        # Generate mode: save normalized output as expected
        expected_file.parent.mkdir(parents=True, exist_ok=True)
        expected_file.write_text(normalized_output, encoding="utf-8")
        return TestStatus.GENERATED, f"Generated {expected_file}"

    mismatch = compare_normalized_outputs(
        test_name,
        expected_file.read_text(encoding="utf-8"),
        normalized_output,
    )
    if mismatch is None:
        return TestStatus.PASS, "PASSED"

    if failure_output_dir is not None:
        failure_output_dir.mkdir(parents=True, exist_ok=True)
        failure_output_file = failure_output_dir / expected_file_name
        failure_output_file.write_text(normalized_output, encoding="utf-8")

    return TestStatus.FAIL, (
        f"{mismatch}\n"
        f"  Expected file: {expected_file} ({expected_source} baseline)"
    )


def main():
    parser = argparse.ArgumentParser(
        description="Test runner for clang-dumper tool and plugin"
    )
    mode_choices = list(get_args(Mode))
    parser.add_argument(
        "--mode",
        choices=mode_choices,
        default=mode_choices[0],
        help="Test mode: 'tool' for standalone executable, 'plugin' for clang plugin (default: tool)",
    )
    parser.add_argument(
        "--path",
        required=True,
        help="Path to the clang-dumper tool executable or plugin shared library",
    )
    parser.add_argument(
        "--clang-path",
        default=None,
        help="Path to clang executable (required for plugin mode)",
    )
    parser.add_argument(
        "--generate",
        action="store_true",
        help="Generate expected output files instead of comparing",
    )
    parser.add_argument(
        "--test-dir",
        default=None,
        help="Path to test directory (default: directory containing this script)",
    )
    parser.add_argument(
        "--enable-cuda",
        action="store_true",
        help="Enable CUDA tests (requires CUDA support in clang)",
    )
    parser.add_argument(
        "--enable-opencl",
        action="store_true",
        help="Enable OpenCL tests (requires target support for the tested OpenCL features)",
    )
    parser.add_argument(
        "--extra-clang-arg",
        action="append",
        default=[],
        help="Extra compiler argument to pass to every test. May be repeated.",
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=None,
        help="Number of parallel test workers (default: CPU count).",
    )
    parser.add_argument(
        "--system-header-threshold",
        type=int,
        default=0,
        help=(
            "Maximum system-header traversal depth for test output. "
            "Use -1 for unlimited traversal."
        ),
    )
    parser.add_argument(
        "--failure-output-dir",
        default=None,
        help=(
            "Write normalized outputs for failed comparisons to this directory. "
            "Useful for reviewing CI differences and replaying normalization changes."
        ),
    )
    parser.add_argument(
        "--raw-output-dir",
        default=None,
        help=(
            "Write raw, non-normalized stderr for every executed test to this "
            "directory, plus a _manifest.json file for offline replay."
        ),
    )
    parser.add_argument(
        "--baseline-platform",
        default=None,
        help=(
            "Use test/expected-platforms/<platform>/<test>.expected when it "
            "exists, falling back to test/expected/<test>.expected."
        ),
    )

    args = parser.parse_args()

    # Validate plugin mode requirements
    if args.mode == "plugin" and args.clang_path is None:
        parser.error("--clang-path is required when using --mode plugin")

    # Resolve paths
    if args.test_dir:
        test_dir = Path(args.test_dir)
    else:
        test_dir = Path(__file__).parent

    inputs_dir = test_dir / "inputs"
    expected_dir = test_dir / "expected"
    target_platform_expected_dirs = platform_expected_dirs(
        test_dir,
        args.baseline_platform,
    )

    if not inputs_dir.exists():
        print(f"ERROR: Inputs directory not found: {inputs_dir}", file=sys.stderr)
        sys.exit(1)

    if not args.generate and not expected_dir.exists():
        print(f"ERROR: Expected directory not found: {expected_dir}", file=sys.stderr)
        sys.exit(1)

    failure_output_dir: Optional[Path] = None
    if args.failure_output_dir:
        failure_output_dir = Path(args.failure_output_dir)

    raw_output_dir: Optional[Path] = None
    if args.raw_output_dir:
        raw_output_dir = Path(args.raw_output_dir)

    # Verify target path exists (tool executable or plugin library)
    target_path = Path(args.path)
    if not target_path.exists():
        print(
            f"ERROR: {'Plugin' if args.mode == 'plugin' else 'Tool'} not found: {target_path}",
            file=sys.stderr,
        )
        sys.exit(1)

    # Verify clang path for plugin mode
    clang_path: Optional[str] = None
    if args.mode == "plugin":
        clang_path_obj = Path(args.clang_path)
        # On Windows, also check with .exe extension
        if not clang_path_obj.exists() and platform.system() == "Windows":
            clang_path_obj = Path(args.clang_path + ".exe")
        if not clang_path_obj.exists():
            # Try to find it in PATH
            import shutil

            found_clang = shutil.which(args.clang_path)
            if found_clang:
                clang_path = found_clang
            else:
                print(f"ERROR: Clang not found: {args.clang_path}", file=sys.stderr)
                sys.exit(1)
        else:
            clang_path = str(clang_path_obj)

    # Build set of enabled features
    # Auto-enable platform-specific features
    enabled_features: set[str] = set()
    if platform.system() != "Windows":
        enabled_features.add("posix")  # POSIX headers like unistd.h
    if args.enable_cuda:
        enabled_features.add("cuda")
    if args.enable_opencl:
        enabled_features.add("opencl")
    if platform.machine().lower() in {"amd64", "x86_64"}:
        enabled_features.add("x86")

    global_flags = shlex.split(os.environ.get("CLANG_DUMPER_TEST_CLANG_ARGS", ""))
    global_flags.extend(args.extra_clang_arg)

    # Discover and run tests
    tests = discover_tests(inputs_dir)

    if not tests:
        print(f"WARNING: No test files found in {inputs_dir}", file=sys.stderr)
        sys.exit(0)

    # Verify all registered tests have corresponding input files
    discovered_names = {t.name for t in tests}
    missing_inputs = []
    for registered_test in TEST_REGISTRY.keys():
        if registered_test not in discovered_names:
            missing_inputs.append(registered_test)

    if missing_inputs:
        print("ERROR: Registered tests without input files:", file=sys.stderr)
        for name in missing_inputs:
            print(f"  - {name}", file=sys.stderr)
        print(
            f"Either create these files in {inputs_dir} or remove them from TEST_REGISTRY.",
            file=sys.stderr,
        )
        sys.exit(1)

    # Pre-resolve inputs_dir to string once (avoid repeated Path.resolve() calls)
    # Use forward slashes for cross-platform consistency (matches normalization in run_tool_and_normalize)
    inputs_dir_str = str(inputs_dir.resolve()).replace("\\", "/")

    if raw_output_dir is not None:
        raw_output_dir.mkdir(parents=True, exist_ok=True)
        manifest = {
            "format": 1,
            "mode": args.mode,
            "inputs_dir": inputs_dir_str,
            "enabled_features": sorted(enabled_features),
            "extra_clang_args": global_flags,
            "system_header_threshold": args.system_header_threshold,
            "baseline_platform": args.baseline_platform,
        }
        (raw_output_dir / "_manifest.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    num_workers = args.jobs if args.jobs is not None else os.cpu_count() or 1
    if num_workers < 1:
        parser.error("--jobs must be at least 1")
    print(
        f"{'Generating' if args.generate else 'Running'} {len(tests)} test(s) "
        f"in {args.mode} mode using {num_workers} parallel workers..."
    )
    if enabled_features:
        print(f"Enabled features: {', '.join(sorted(enabled_features))}")
    if global_flags:
        print(f"Extra compiler args: {shlex.join(global_flags)}")
    if args.baseline_platform:
        print(f"Baseline platform: {args.baseline_platform}")
    print()

    passed = 0
    failed = 0
    skipped = 0

    # Determine whether to use ANSI colors (once, before the loop)
    use_color = sys.stdout.isatty() and os.environ.get("NO_COLOR") is None
    # On Windows 10+, enable ANSI escape sequence processing
    if use_color and os.name == "nt":
        try:
            import ctypes

            kernel32 = ctypes.windll.kernel32
            # Enable VIRTUAL_TERMINAL_PROCESSING for stdout
            STD_OUTPUT_HANDLE = -11
            ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004
            handle = kernel32.GetStdHandle(STD_OUTPUT_HANDLE)
            mode = ctypes.c_ulong()
            kernel32.GetConsoleMode(handle, ctypes.byref(mode))
            kernel32.SetConsoleMode(
                handle, mode.value | ENABLE_VIRTUAL_TERMINAL_PROCESSING
            )
        except Exception:
            use_color = False

    def _color(text: str, code: str) -> str:
        return f"\033[{code}m{text}\033[0m" if use_color else text

    PASS_LABEL = _color("PASS", "32")
    GENERATED_LABEL = PASS_LABEL
    SKIP_LABEL = _color("SKIP", "33")
    FAIL_LABEL = _color("FAIL", "31")

    # Run tests in parallel using ProcessPoolExecutor
    # Results are collected and printed in original test order for predictable output
    results: dict[str, tuple[str, str]] = {}

    with ProcessPoolExecutor(max_workers=num_workers) as executor:
        # Submit all tests
        future_to_test = {
            executor.submit(
                run_single_test,
                mode=args.mode,
                path=str(target_path),
                input_file=test_file,
                expected_dir=expected_dir,
                platform_expected_dirs=target_platform_expected_dirs,
                failure_output_dir=failure_output_dir,
                raw_output_dir=raw_output_dir,
                inputs_dir_str=inputs_dir_str,
                generate=args.generate,
                enabled_features=enabled_features,
                clang_path=clang_path,
                global_flags=global_flags,
                system_header_threshold=args.system_header_threshold,
            ): test_file
            for test_file in tests
        }

        # Collect results as they complete
        for future in as_completed(future_to_test):
            test_file = future_to_test[future]
            test_name = test_file.name
            try:
                status, message = future.result()
                results[test_name] = (status, message)
            except Exception as e:
                results[test_name] = (TestStatus.FAIL, f"Exception: {e}")

    # Print results in original test order
    for test_file in tests:
        test_name = test_file.name
        status, message = results[test_name]

        if status == TestStatus.PASS:
            passed += 1
            print(f"  [{PASS_LABEL}] {test_name}")
        elif status == TestStatus.GENERATED:
            passed += 1
            print(f"  [{GENERATED_LABEL}] {test_name}")
            print(f"         {message}")
        elif status == TestStatus.SKIP:
            skipped += 1
            print(f"  [{SKIP_LABEL}] {test_name}")
            print(f"         {message}")
        else:
            failed += 1
            print(f"  [{FAIL_LABEL}] {test_name}")
            print(f"         {message}")

    print()
    print(f"Results: {passed} passed, {failed} failed, {skipped} skipped")

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
