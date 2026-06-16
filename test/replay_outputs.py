#!/usr/bin/env python3
"""
Replay raw clang-dumper test outputs captured by run_tests.py.

This lets CI publish target-platform output once, then lets local development
iterate on Python normalization and expected-file comparison without rerunning
target binaries.
"""

import argparse
import gzip
import io
import json
import os
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed
from pathlib import Path
from typing import Optional

from run_tests import (
    TestStatus,
    check_address_consistency,
    compare_normalized_outputs,
    normalize_captured_lines,
    platform_expected_dir,
    platform_expected_dirs,
    read_expected_output,
    resolve_expected_file,
)


def replay_single_output(
    raw_file: Path,
    expected_dir: Path,
    target_platform_expected_dirs: list[Path],
    failure_output_dir: Optional[Path],
    inputs_dir_str: str,
    write_platform_baseline_dir: Optional[Path],
) -> tuple[str, str]:
    test_name = raw_file.name.removesuffix(".stderr")
    if test_name == raw_file.name:
        return TestStatus.SKIP, "Not a captured stderr file"

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

    if write_platform_baseline_dir is not None:
        shared_expected_file = expected_dir / f"{test_name}.expected"
        if not shared_expected_file.exists():
            return TestStatus.FAIL, f"Expected file not found: {shared_expected_file}"

        mismatch = compare_normalized_outputs(
            test_name,
            read_expected_output(shared_expected_file),
            normalized_output,
        )
        if mismatch is None:
            return TestStatus.PASS, "Matches shared baseline"

        write_platform_baseline_dir.mkdir(parents=True, exist_ok=True)
        platform_expected_file = write_platform_baseline_dir / f"{test_name}.expected.gz"
        with platform_expected_file.open("wb") as raw_stream:
            with gzip.GzipFile(
                fileobj=raw_stream,
                mode="wb",
                mtime=0,
            ) as gzip_stream:
                with io.TextIOWrapper(gzip_stream, encoding="utf-8") as text_stream:
                    text_stream.write(normalized_output)
        return TestStatus.GENERATED, f"Generated {platform_expected_file}"

    expected_file, expected_source = resolve_expected_file(
        expected_dir,
        target_platform_expected_dirs,
        test_name,
    )
    if not expected_file.exists():
        return TestStatus.FAIL, f"Expected file not found: {expected_file}"

    mismatch = compare_normalized_outputs(
        test_name,
        read_expected_output(expected_file),
        normalized_output,
    )
    if mismatch is None:
        return TestStatus.PASS, "PASSED"

    if failure_output_dir is not None:
        failure_output_dir.mkdir(parents=True, exist_ok=True)
        failure_output_file = failure_output_dir / f"{test_name}.expected"
        failure_output_file.write_text(normalized_output, encoding="utf-8")

    return TestStatus.FAIL, (
        f"{mismatch}\n"
        f"  Expected file: {expected_file} ({expected_source} baseline)"
    )


def replay_raw_file(
    raw_file: Path,
    expected_dir: Path,
    target_platform_expected_dirs: list[Path],
    failure_output_dir: Optional[Path],
    inputs_dir_str: str,
    write_platform_baseline_dir: Optional[Path],
) -> tuple[str, str, str]:
    status, message = replay_single_output(
        raw_file,
        expected_dir,
        target_platform_expected_dirs,
        failure_output_dir,
        inputs_dir_str,
        write_platform_baseline_dir,
    )
    return raw_file.name.removesuffix(".stderr"), status, message


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
    parser.add_argument(
        "--jobs",
        type=int,
        default=None,
        help="Number of parallel replay workers (default: CPU count).",
    )
    parser.add_argument(
        "--baseline-platform",
        default=None,
        help=(
            "Use test/expected-platforms/<platform>/<test>.expected when it "
            "exists, falling back to test/expected/<test>.expected. Defaults "
            "to _manifest.json baseline_platform when available."
        ),
    )
    parser.add_argument(
        "--write-platform-baselines",
        default=None,
        metavar="PLATFORM",
        help=(
            "Write test/expected-platforms/<platform> baselines for captured "
            "outputs that differ from the shared expected files."
        ),
    )
    parser.add_argument(
        "--prune-platform-baselines",
        action="store_true",
        help=(
            "Before --write-platform-baselines, delete existing generated "
            "*.expected files for that platform."
        ),
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
    manifest_platform = manifest.get("baseline_platform")
    baseline_platform = args.baseline_platform
    if baseline_platform is None and isinstance(manifest_platform, str):
        baseline_platform = manifest_platform
    target_platform_expected_dirs = platform_expected_dirs(test_dir, baseline_platform)

    write_platform_baseline_dir: Optional[Path] = None
    if args.write_platform_baselines:
        write_platform_baseline_dir = platform_expected_dir(
            test_dir,
            args.write_platform_baselines,
        )
        if args.prune_platform_baselines and write_platform_baseline_dir.exists():
            for existing_file in write_platform_baseline_dir.glob("*.expected*"):
                existing_file.unlink()
    elif args.prune_platform_baselines:
        parser.error("--prune-platform-baselines requires --write-platform-baselines")

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
    if baseline_platform:
        print(f"Baseline platform: {baseline_platform}")
    if write_platform_baseline_dir is not None:
        print(f"Writing platform baselines to: {write_platform_baseline_dir}")
    print()

    passed = 0
    failed = 0
    skipped = 0
    generated = 0
    num_workers = args.jobs if args.jobs is not None else os.cpu_count() or 1
    if num_workers < 1:
        parser.error("--jobs must be at least 1")

    results: dict[str, tuple[str, str]] = {}

    with ProcessPoolExecutor(max_workers=num_workers) as executor:
        future_to_raw_file = {
            executor.submit(
                replay_raw_file,
                raw_file,
                expected_dir,
                target_platform_expected_dirs,
                failure_output_dir,
                inputs_dir_str,
                write_platform_baseline_dir,
            ): raw_file
            for raw_file in raw_files
        }

        for future in as_completed(future_to_raw_file):
            raw_file = future_to_raw_file[future]
            test_name = raw_file.name.removesuffix(".stderr")
            try:
                _, status, message = future.result()
                results[test_name] = (status, message)
            except Exception as exc:
                results[test_name] = (TestStatus.FAIL, f"Exception: {exc}")

    for raw_file in raw_files:
        test_name = raw_file.name.removesuffix(".stderr")
        status, message = results[test_name]

        if status == TestStatus.PASS:
            passed += 1
            print(f"  [PASS] {test_name}")
        elif status == TestStatus.GENERATED:
            generated += 1
            print(f"  [GENERATED] {test_name}")
            print(f"              {message}")
        elif status == TestStatus.SKIP:
            skipped += 1
            print(f"  [SKIP] {test_name}")
            print(f"         {message}")
        else:
            failed += 1
            print(f"  [FAIL] {test_name}")
            print(f"         {message}")

    print()
    print(
        f"Results: {passed} passed, {failed} failed, "
        f"{skipped} skipped, {generated} generated"
    )
    sys.exit(0 if failed == 0 else 1)


if __name__ == "__main__":
    main()
