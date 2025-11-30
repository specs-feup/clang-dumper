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
import os
import platform
import re
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass, field
from pathlib import Path
from typing import Literal, get_args

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
    id: int = 0, flags: list[str] | None = None, requires: set[str] | None = None
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
    "2mm.c": T(),
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
    "ast-dump-expr.c": T(),
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
    "builtin_types.cl": T(),
    "builtin_types.cpp": T(),
    "c-casts.c": T(),
    "c89.c": T(),
    "c99.c": T(),
    "character.cpp": T(),
    "cl_attribute.cl": T(),
    "class_template.cpp": T(),
    "class_template.h": T(flags=["-x", "c++"]),
    "classes.cpp": T(),
    "clava_issue09.h": T(flags=["-x", "c++"]),
    "clava_issue10.cpp": T(),
    "clava_issue11.cpp": T(),
    "clava_issue13.cpp": T(),
    "clava_issue14.h": T(flags=["-x", "c++"]),
    "clava_issue15.cpp": T(),
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
    "nas_bt.c": T(),
    "nas_ft.c": T(),
    "nas_lu.c": T(),
    "nas_ua.c": T(),
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
    "variadic.c": T(),
    "while.cpp": T(),
}

# Placeholders for normalized paths
PATH_PLACEHOLDER = "<TEST_DIR>"
SYSTEM_INCLUDE_PLACEHOLDER = "<SYSTEM_INCLUDE>"
CLANG_INCLUDE_PLACEHOLDER = "<CLANG_INCLUDE>"
GCC_INCLUDE_PLACEHOLDER = "<GCC_INCLUDE>"

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
    (r"/opt/homebrew/opt/llvm@?\d*/lib/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),
    (r"/Applications/Xcode\.app/.+/lib/clang/[\d.]+/include", CLANG_INCLUDE_PLACEHOLDER),

    # Windows: MSYS2/MinGW and LLVM installations
    (r"[A-Za-z]:[/\\]msys64[/\\]mingw\d+[/\\]lib[/\\]clang[/\\][\d.]+[/\\]include", CLANG_INCLUDE_PLACEHOLDER),
    (r"[A-Za-z]:[/\\]Program Files[/\\]LLVM[/\\]lib[/\\]clang[/\\][\d.]+[/\\]include", CLANG_INCLUDE_PLACEHOLDER),
    (r"[A-Za-z]:[/\\]mingw64-clang-\d+[/\\]lib[/\\]clang[/\\][\d.]+[/\\]include", CLANG_INCLUDE_PLACEHOLDER),
    # Windows: Custom LLVM installation paths (e.g., CI environments)
    (r"[A-Za-z]:[/\\]llvm[/\\]lib[/\\]clang[/\\][\d.]+[/\\]include", CLANG_INCLUDE_PLACEHOLDER),

    # ==================== GCC HEADERS ====================
    # Linux: Canonicalize /usr/bin/../lib/gcc/ to /usr/lib/gcc/
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
    (r"/Library/Developer/CommandLineTools/SDKs/MacOSX[\d.]*\.sdk/usr/include", SYSTEM_INCLUDE_PLACEHOLDER),
    (r"/Applications/Xcode\.app/.+/SDKs/MacOSX[\d.]*\.sdk/usr/include", SYSTEM_INCLUDE_PLACEHOLDER),

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
    groups.append(r"(?P<addr>0x[0-9a-fA-F]+_\d+)")
    group_map["addr"] = None  # Sentinel: handled specially in replacement function
    
    combined = "|".join(groups)
    return re.compile(combined), group_map

# Pre-compiled at module load time
_UNIFIED_REGEX, _UNIFIED_REPLACEMENTS = _build_combined_pattern()


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
        unique_addresses = set(raw_addresses)
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
    clang_path: str | None = None,
    extra_flags: list[str] | None = None,
) -> tuple[int, str, str, dict[str, list[str]]]:
    """
    Run the clang-dumper tool or plugin and normalize output via streaming.

    This function combines subprocess execution with output normalization,
    processing stderr line-by-line as it arrives to reduce memory pressure
    and improve performance on large outputs.

    Args:
        mode: Either "tool" or "plugin"
        path: Path to the tool executable or plugin shared library
        input_file: Path to the input source file
        test_id: The test ID for address disambiguation
        inputs_dir_str: Pre-resolved inputs directory path for normalization
        clang_path: Path to clang executable (required for plugin mode)
        extra_flags: Additional compiler flags to pass

    Returns:
        tuple: (return_code, stdout, normalized_stderr, address_mapping)
    """
    flags = extra_flags or []

    if mode == "tool":
        cmd = [path, f"-id={test_id}", input_file, "--"] + flags
    else:
        # Plugin mode - invoke clang with the plugin loaded
        assert clang_path is not None, "clang_path required for plugin mode"
        cmd = (
            [
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
            + flags
            + [
                "-fsyntax-only",
                input_file,
            ]
        )

    # State for address normalization
    address_map: dict[str, str] = {}  # raw_address -> placeholder
    placeholder_to_raw: dict[str, list[str]] = {}  # placeholder -> [raw_addresses]
    counter = [1]  # Use list to allow mutation in nested function
    
    # Precompute path variants for replacement on Windows
    # On Windows, Path.resolve() returns backslashes (e.g., D:\path\to\inputs)
    # but LLVM/Clang typically outputs forward slashes (e.g., D:/path/to/inputs)
    # We need both variants for proper normalization
    inputs_dir_str_fwd = inputs_dir_str.replace("\\", "/")  # Forward slash version
    inputs_dir_str_bwd = inputs_dir_str.replace("/", "\\")  # Backslash version

    def unified_replacer(match: re.Match[str]) -> str:
        """Single-pass replacement function for both paths and addresses."""
        group_name = match.lastgroup
        if group_name is None:
            return match.group(0)
        
        if group_name == "addr":
            raw_address = match.group(0)
            if raw_address not in address_map:
                placeholder = f"ADDR_{counter[0]:03d}"
                address_map[raw_address] = placeholder
                placeholder_to_raw[placeholder] = [raw_address]
                counter[0] += 1
            else:
                placeholder = address_map[raw_address]
                if raw_address not in placeholder_to_raw[placeholder]:
                    placeholder_to_raw[placeholder].append(raw_address)
            return placeholder
        
        replacement = _UNIFIED_REPLACEMENTS.get(group_name)
        if replacement is not None:
            return replacement
        return match.group(0)

    # Run subprocess with streaming stderr processing
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    
    # Process stderr line-by-line for memory efficiency on large outputs
    normalized_lines: list[str] = []
    assert proc.stderr is not None
    for line in proc.stderr:
        # Fast string replacement for inputs_dir (before regex)
        # Replace both forward and backslash variants to handle platform differences
        line = line.replace(inputs_dir_str_fwd, PATH_PLACEHOLDER)
        line = line.replace(inputs_dir_str_bwd, PATH_PLACEHOLDER)
        # Single-pass regex for system paths and addresses
        line = _UNIFIED_REGEX.sub(unified_replacer, line)
        normalized_lines.append(line)
    
    # Read stdout and wait for process
    stdout, _ = proc.communicate()
    
    normalized_stderr = "".join(normalized_lines)
    return proc.returncode, stdout, normalized_stderr, placeholder_to_raw


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
    inputs_dir_str: str,
    generate: bool,
    enabled_features: set[str],
    clang_path: str | None = None,
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

    expected_file = expected_dir / f"{test_name}.expected"

    # Verify expected file exists (unless generating)
    if not generate and not expected_file.exists():
        return TestStatus.FAIL, (
            f"Expected file not found: {expected_file}\n"
            f"Run with --generate to create it, or check if the test is properly registered."
        )

    # Run the tool/plugin with streaming normalization
    return_code, stdout, normalized_output, placeholder_to_raw = run_tool_and_normalize(
        mode, path, str(input_file), config.id, inputs_dir_str, clang_path, config.flags
    )

    if return_code != 0:
        return TestStatus.FAIL, f"Tool exited with code {return_code}"

    # Check address consistency
    consistency_errors = check_address_consistency(placeholder_to_raw)
    if consistency_errors:
        return TestStatus.FAIL, "Address consistency errors:\n" + "\n".join(
            consistency_errors
        )

    if generate:
        # Generate mode: save normalized output as expected
        expected_file.parent.mkdir(parents=True, exist_ok=True)
        expected_file.write_text(normalized_output, encoding="utf-8")
        return TestStatus.GENERATED, f"Generated {expected_file}"

    expected_output = expected_file.read_text(encoding="utf-8")

    if normalized_output == expected_output:
        return TestStatus.PASS, "PASSED"

    # Find first difference for error message
    normalized_lines = normalized_output.splitlines(keepends=True)
    expected_lines = expected_output.splitlines(keepends=True)

    for i, (norm_line, exp_line) in enumerate(zip(normalized_lines, expected_lines), 1):
        if norm_line != exp_line:
            return TestStatus.FAIL, (
                f"Mismatch at line {i}:\n"
                f"  Expected: {exp_line.rstrip()!r}\n"
                f"  Got:      {norm_line.rstrip()!r}"
            )

    # Different number of lines
    if len(normalized_lines) != len(expected_lines):
        return TestStatus.FAIL, (
            f"Line count mismatch: expected {len(expected_lines)}, got {len(normalized_lines)}"
        )

    return TestStatus.FAIL, "Unknown difference"


def get_default_plugin_extension() -> str:
    """Get the default plugin file extension for the current platform."""
    system = platform.system()
    if system == "Darwin":  # macOS
        return ".dylib"
    elif system == "Windows":
        return ".dll"
    else:
        return ".so"


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

    if not inputs_dir.exists():
        print(f"ERROR: Inputs directory not found: {inputs_dir}", file=sys.stderr)
        sys.exit(1)

    if not args.generate and not expected_dir.exists():
        print(f"ERROR: Expected directory not found: {expected_dir}", file=sys.stderr)
        sys.exit(1)

    # Verify target path exists (tool executable or plugin library)
    target_path = Path(args.path)
    if not target_path.exists():
        print(
            f"ERROR: {'Plugin' if args.mode == 'plugin' else 'Tool'} not found: {target_path}",
            file=sys.stderr,
        )
        sys.exit(1)

    # Verify clang path for plugin mode
    clang_path: str | None = None
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
    enabled_features: set[str] = set()
    if args.enable_cuda:
        enabled_features.add("cuda")

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
    inputs_dir_str = str(inputs_dir.resolve())

    num_workers = os.cpu_count() or 1
    print(
        f"{'Generating' if args.generate else 'Running'} {len(tests)} test(s) "
        f"in {args.mode} mode using {num_workers} parallel workers..."
    )
    if enabled_features:
        print(f"Enabled features: {', '.join(sorted(enabled_features))}")
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
                inputs_dir_str=inputs_dir_str,
                generate=args.generate,
                enabled_features=enabled_features,
                clang_path=clang_path,
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
