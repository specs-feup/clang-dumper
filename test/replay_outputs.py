#!/usr/bin/env python3
"""
Replay raw clang-dumper test outputs captured by run_tests.py.

This lets CI publish target-platform output once, then lets local development
iterate on Python normalization and expected-file comparison without rerunning
target binaries.
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Optional

from run_tests import (
    TestStatus,
    check_address_consistency,
    compare_normalized_outputs,
    normalize_captured_lines,
)


def replay_single_output(
    raw_file: Path,
    expected_dir: Path,
    failure_output_dir: Optional[Path],
    inputs_dir_str: str,
) -> tuple[str, str]:
    test_name = raw_file.name.removesuffix(".stderr")
    if test_name == raw_file.name:
        return TestStatus.SKIP, "Not a captured stderr file"

    expected_file = expected_dir / f"{test_name}.expected"
    if not expected_file.exists():
        return TestStatus.FAIL, f"Expected file not found: {expected_file}"

    with raw_file.open(encoding="utf-8", errors="replace") as raw_stream:
        normalized_output, placeholder_to_raw = normalize_captured_lines(
            raw_stream,
            inputs_dir_str,
        )

    consistency_errors = check_address_consistency(placeholder_to_raw)
    if consistency_errors:
        return TestStatus.FAIL, "Address consistency errors:\n" + "\n".join(
            consistency_errors
        )

    mismatch = compare_normalized_outputs(
        test_name,
        expected_file.read_text(encoding="utf-8"),
        normalized_output,
    )
    if mismatch is None:
        return TestStatus.PASS, "PASSED"

    if failure_output_dir is not None:
        failure_output_dir.mkdir(parents=True, exist_ok=True)
        failure_output_file = failure_output_dir / f"{test_name}.expected"
        failure_output_file.write_text(normalized_output, encoding="utf-8")

    return TestStatus.FAIL, mismatch


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Replay raw clang-dumper outputs captured by run_tests.py"
    )
    parser.add_argument(
        "--raw-output-dir",
        required=True,
        help="Directory produced by run_tests.py --raw-output-dir",
    )
    parser.add_argument(
        "--test-dir",
        default=None,
        help="Path to test directory (default: directory containing this script)",
    )
    parser.add_argument(
        "--inputs-dir",
        default=None,
        help=(
            "Override the source inputs directory recorded in _manifest.json. "
            "Only needed when replaying an older capture without a manifest."
        ),
    )
    parser.add_argument(
        "--failure-output-dir",
        default=None,
        help="Write normalized outputs for failed comparisons to this directory.",
    )
    args = parser.parse_args()

    raw_output_dir = Path(args.raw_output_dir)
    manifest_file = raw_output_dir / "_manifest.json"
    manifest: dict[str, object] = {}
    if manifest_file.exists():
        manifest = json.loads(manifest_file.read_text(encoding="utf-8"))

    if args.test_dir:
        test_dir = Path(args.test_dir)
    else:
        test_dir = Path(__file__).parent
    expected_dir = test_dir / "expected"

    if not raw_output_dir.exists():
        print(f"ERROR: Raw output directory not found: {raw_output_dir}", file=sys.stderr)
        sys.exit(1)
    if not expected_dir.exists():
        print(f"ERROR: Expected directory not found: {expected_dir}", file=sys.stderr)
        sys.exit(1)

    inputs_dir_value = args.inputs_dir or manifest.get("inputs_dir")
    if not isinstance(inputs_dir_value, str) or not inputs_dir_value:
        print(
            "ERROR: Missing inputs_dir. Provide --inputs-dir or replay a directory "
            "with _manifest.json.",
            file=sys.stderr,
        )
        sys.exit(1)
    inputs_dir_str = inputs_dir_value.replace("\\", "/")

    failure_output_dir = (
        Path(args.failure_output_dir) if args.failure_output_dir else None
    )

    raw_files = sorted(raw_output_dir.glob("*.stderr"))
    if not raw_files:
        print(f"ERROR: No captured .stderr files found in {raw_output_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"Replaying {len(raw_files)} captured output(s) from {raw_output_dir}")
    if manifest:
        mode = manifest.get("mode", "unknown")
        features = manifest.get("enabled_features", [])
        print(f"Capture mode: {mode}")
        if isinstance(features, list) and features:
            print(f"Captured features: {', '.join(str(feature) for feature in features)}")
    print()

    passed = 0
    failed = 0
    skipped = 0

    for raw_file in raw_files:
        test_name = raw_file.name.removesuffix(".stderr")
        status, message = replay_single_output(
            raw_file,
            expected_dir,
            failure_output_dir,
            inputs_dir_str,
        )

        if status == TestStatus.PASS:
            passed += 1
            print(f"  [PASS] {test_name}")
        elif status == TestStatus.SKIP:
            skipped += 1
            print(f"  [SKIP] {test_name}")
            print(f"         {message}")
        else:
            failed += 1
            print(f"  [FAIL] {test_name}")
            print(f"         {message}")

    print()
    print(f"Results: {passed} passed, {failed} failed, {skipped} skipped")
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
