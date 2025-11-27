#!/usr/bin/env python3
"""
Test runner for clang-dumper tool.

Runs the tool on sample C/C++ source files, normalizes memory addresses in the output,
and compares against expected baseline files.

Usage:
    python run_tests.py --tool-path /path/to/tool
    python run_tests.py --tool-path /path/to/tool --generate  # Generate expected files
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

# Fixed IDs for each test file (randomized but deterministic)
# Every test file MUST have an entry here - no default fallback to catch typos
TEST_IDS = {
    "simple_function.cpp": 42,
    "class_decl.cpp": 17,
    "expressions.cpp": 73,
}

# Regex pattern to match memory addresses like 0x7fff1234_42 or 0x1234abcd_0
ADDRESS_PATTERN = re.compile(r"0x[0-9a-fA-F]+_(\d+)")

# Placeholder for normalized paths
PATH_PLACEHOLDER = "<TEST_DIR>"


def get_test_id(test_name: str) -> int:
    """
    Get the fixed ID for a test file.

    Raises:
        KeyError: If the test file is not registered in TEST_IDS (prevents typos)
    """
    if test_name not in TEST_IDS:
        raise KeyError(
            f"Test file '{test_name}' not found in TEST_IDS. "
            f"Add an entry for it in run_tests.py to register this test."
        )
    return TEST_IDS[test_name]


def normalize_paths(output: str, inputs_dir: Path) -> str:
    """
    Normalize file system paths in the output.

    Replaces the test inputs directory path with a placeholder to make
    expected files portable across different systems.

    Args:
        output: The output string to normalize
        inputs_dir: The path to the test inputs directory

    Returns:
        Output with paths replaced by placeholder
    """
    # Normalize the inputs directory path (handle both forward and back slashes)
    inputs_dir_str = str(inputs_dir.resolve())
    # Replace the path with placeholder
    normalized = output.replace(inputs_dir_str, PATH_PLACEHOLDER)
    # Also handle Windows-style paths if present
    normalized = normalized.replace(inputs_dir_str.replace("/", "\\"), PATH_PLACEHOLDER)
    return normalized


def normalize_addresses(output: str) -> tuple[str, dict[str, list[str]]]:
    """
    Normalize memory addresses in the output.

    Replaces each unique address with a deterministic placeholder (ADDR_001, ADDR_002, etc.)
    ordered by first appearance.

    Returns:
        tuple: (normalized_output, address_mapping)
            - normalized_output: Output with addresses replaced by placeholders
            - address_mapping: Dict mapping placeholder -> list of raw addresses seen
    """
    address_map: dict[str, str] = {}  # raw_address -> placeholder
    placeholder_to_raw: dict[str, list[str]] = {}  # placeholder -> [raw_addresses]
    counter = 1

    def replace_address(match: re.Match) -> str:
        nonlocal counter
        raw_address = match.group(0)

        if raw_address not in address_map:
            placeholder = f"ADDR_{counter:03d}"
            address_map[raw_address] = placeholder
            placeholder_to_raw[placeholder] = [raw_address]
            counter += 1
        else:
            placeholder = address_map[raw_address]
            # Track all occurrences (should all be the same raw address)
            if raw_address not in placeholder_to_raw[placeholder]:
                placeholder_to_raw[placeholder].append(raw_address)

        return placeholder

    normalized = ADDRESS_PATTERN.sub(replace_address, output)
    return normalized, placeholder_to_raw


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


def run_tool(tool_path: str, input_file: str, test_id: int) -> tuple[int, str, str]:
    """
    Run the clang-dumper tool on an input file.

    Returns:
        tuple: (return_code, stdout, stderr)
    """
    cmd = [tool_path, f"-id={test_id}", input_file, "--"]

    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
    )

    return result.returncode, result.stdout, result.stderr


def discover_tests(inputs_dir: Path) -> list[Path]:
    """Discover test input files in the inputs directory."""
    extensions = {".c", ".cpp", ".cc", ".cxx"}
    tests = []

    for file in sorted(inputs_dir.iterdir()):
        if file.is_file() and file.suffix in extensions:
            tests.append(file)

    return tests


def run_single_test(
    tool_path: str,
    input_file: Path,
    expected_dir: Path,
    inputs_dir: Path,
    generate: bool,
) -> tuple[bool, str]:
    """
    Run a single test case.

    Returns:
        tuple: (passed, message)
    """
    test_name = input_file.name

    # Verify test is registered in TEST_IDS
    try:
        test_id = get_test_id(test_name)
    except KeyError as e:
        return False, str(e)
    expected_file = expected_dir / f"{test_name}.expected"

    # Verify expected file exists (unless generating)
    if not generate and not expected_file.exists():
        return False, (
            f"Expected file not found: {expected_file}\n"
            f"Run with --generate to create it, or check if the test is properly registered."
        )

    # Run the tool
    return_code, stdout, stderr = run_tool(tool_path, str(input_file), test_id)

    if return_code != 0:
        return False, f"Tool exited with code {return_code}\nstderr: {stderr}"

    # Normalize paths first (before addresses, as paths may contain hex-like sequences)
    path_normalized_output = normalize_paths(stderr, inputs_dir)

    # Normalize addresses in stderr (that's where the AST dump goes)
    normalized_output, placeholder_to_raw = normalize_addresses(path_normalized_output)

    # Check address consistency
    consistency_errors = check_address_consistency(placeholder_to_raw)
    if consistency_errors:
        return False, "Address consistency errors:\n" + "\n".join(consistency_errors)

    if generate:
        # Generate mode: save normalized output as expected
        expected_file.write_text(normalized_output, encoding="utf-8")
        return True, f"Generated {expected_file}"

    expected_output = expected_file.read_text(encoding="utf-8")

    if normalized_output == expected_output:
        return True, "PASSED"

    # Find first difference for error message
    normalized_lines = normalized_output.splitlines(keepends=True)
    expected_lines = expected_output.splitlines(keepends=True)

    for i, (norm_line, exp_line) in enumerate(zip(normalized_lines, expected_lines), 1):
        if norm_line != exp_line:
            return False, (
                f"Mismatch at line {i}:\n"
                f"  Expected: {exp_line.rstrip()!r}\n"
                f"  Got:      {norm_line.rstrip()!r}"
            )

    # Different number of lines
    if len(normalized_lines) != len(expected_lines):
        return False, (
            f"Line count mismatch: expected {len(expected_lines)}, got {len(normalized_lines)}"
        )

    return False, "Unknown difference"


def main():
    parser = argparse.ArgumentParser(description="Test runner for clang-dumper tool")
    parser.add_argument(
        "--tool-path",
        required=True,
        help="Path to the clang-dumper tool executable",
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

    args = parser.parse_args()

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

    # Verify tool exists
    tool_path = Path(args.tool_path)
    if not tool_path.exists():
        print(f"ERROR: Tool not found: {tool_path}", file=sys.stderr)
        sys.exit(1)

    # Discover and run tests
    tests = discover_tests(inputs_dir)

    if not tests:
        print(f"WARNING: No test files found in {inputs_dir}", file=sys.stderr)
        sys.exit(0)

    # Verify all registered tests have corresponding input files
    discovered_names = {t.name for t in tests}
    missing_inputs = []
    for registered_test in TEST_IDS.keys():
        if registered_test not in discovered_names:
            missing_inputs.append(registered_test)

    if missing_inputs:
        print("ERROR: Registered tests without input files:", file=sys.stderr)
        for name in missing_inputs:
            print(f"  - {name}", file=sys.stderr)
        print(
            f"Either create these files in {inputs_dir} or remove them from TEST_IDS.",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"{'Generating' if args.generate else 'Running'} {len(tests)} test(s)...")
    print()

    passed = 0
    failed = 0

    for test_file in tests:
        test_name = test_file.name
        success, message = run_single_test(
            str(tool_path),
            test_file,
            expected_dir,
            inputs_dir,
            args.generate,
        )

        if success:
            passed += 1
            status = "PASS" if not args.generate else "GENERATED"
            print(f"  [{status}] {test_name}")
            if args.generate:
                print(f"         {message}")
        else:
            failed += 1
            print(f"  [FAIL] {test_name}")
            print(f"         {message}")

    print()
    print(f"Results: {passed} passed, {failed} failed")

    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
