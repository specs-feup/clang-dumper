#!/usr/bin/env python3
"""
Test runner for the eager FlatBuffers clang-dumper tool and plugin.

Runs both producers on the registered C/C++ corpus and validates each complete
size-prefixed FlatBuffers stream with the native schema verifier.

Usage:
    # Tool mode (default)
    python run_tests.py --mode tool --path /path/to/tool
    python run_tests.py --mode tool --path /path/to/tool --verifier /path/to/verify_flatbuffers

    # Plugin mode
    python run_tests.py --mode plugin --path /path/to/plugin.so --clang-path /path/to/clang
    python run_tests.py --mode plugin --path /path/to/plugin.so --clang-path /path/to/clang --verifier /path/to/verify_flatbuffers
"""

import argparse
import json
import os
import platform
import shlex
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass, field
from pathlib import Path
from typing import Literal, Optional, get_args

# Type alias for mode
Mode = Literal["tool", "plugin"]
TEST_INPUTS_DIR = Path(__file__).resolve().parent / "inputs"


@dataclass
class TestConfig:
    """Configuration for a single test file."""

    id: int = 0
    flags: list[str] = field(default_factory=list)
    requires: set[str] = field(default_factory=set)
    system_header_threshold: Optional[int] = None
    expected_gcc_asm: Optional[str] = None
    expected_ms_asm: Optional[str] = None


# Helper to create simple test configs
def T(
    id: int = 0,
    flags: Optional[list[str]] = None,
    requires: Optional[set[str]] = None,
    system_header_threshold: Optional[int] = None,
    expected_gcc_asm: Optional[str] = None,
    expected_ms_asm: Optional[str] = None,
) -> TestConfig:
    """Shorthand for creating TestConfig instances."""
    return TestConfig(
        id=id,
        flags=flags or [],
        requires=requires or set(),
        system_header_threshold=system_header_threshold,
        expected_gcc_asm=expected_gcc_asm,
        expected_ms_asm=expected_ms_asm,
    )


# Test registry with per-test configuration
# Every test file MUST have an entry here - no default fallback to catch typos
# Use T() helper: T(id, flags=[...], requires={...})
TEST_REGISTRY: dict[str, TestConfig] = {
    "simple_function.cpp": T(42),
    "source_locations.cpp": T(),
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
    "asm_source.cpp": T(
        flags=["--target=i686-pc-windows-msvc", "-fms-extensions", "-fasm-blocks"],
        requires={"x86"},
        expected_gcc_asm="movl %1, %0\n\taddl $1, %0",
        expected_ms_asm=(
            "\n    mov eax, value\n    jmp local_done\n  local_done:\n"
            "    add eax, 1\n  "
        ),
    ),
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
    "clava_issue48.c": T(),
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
    "system_header_threshold.cpp": T(
        flags=["-isystem", str(TEST_INPUTS_DIR / "system_headers")],
    ),
    "system_header_threshold_option.cpp": T(
        flags=["-isystem", str(TEST_INPUTS_DIR / "system_headers")],
        system_header_threshold=-1,
    ),
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

CUDA_TEST_FLAGS = ["--no-cuda-version-check", "--cuda-host-only"]


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


def run_flatbuffers_and_verify(
    mode: Mode,
    path: str,
    verifier: str,
    input_file: str,
    test_id: int,
    clang_path: Optional[str] = None,
    extra_flags: Optional[list[str]] = None,
    system_header_threshold: Optional[int] = 1,
    verifier_args: Optional[list[str]] = None,
) -> tuple[int, str, str, bytes, str]:
    """Run one producer and verify every framed record in its output stream."""
    flags = extra_flags or []
    with tempfile.TemporaryDirectory(prefix="clang-dumper-flatbuffers-") as directory:
        dump_path = Path(directory) / "ast.clv2"
        if mode == "tool":
            cmd = [path, f"-id={test_id}"]
            if system_header_threshold is not None:
                cmd.append(f"-system-header-threshold={system_header_threshold}")
            cmd += ["-c", input_file, "-o", str(dump_path), "--"] + flags
        else:
            assert clang_path is not None, "clang_path required for plugin mode"
            cmd = [
                clang_path,
                f"-fplugin={path}",
                "-Xclang", "-plugin", "-Xclang", "DumpAst",
                "-Xclang", "-plugin-arg-DumpAst",
                "-Xclang", f"-file-id={test_id}",
                "-Xclang", "-plugin-arg-DumpAst",
                "-Xclang", f"-output={dump_path}",
            ]
            if system_header_threshold is not None:
                cmd += [
                    "-Xclang", "-plugin-arg-DumpAst",
                    "-Xclang", f"-system-header-threshold={system_header_threshold}",
                ]
            cmd += flags + ["-fsyntax-only", input_file]

        proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
        dump = dump_path.read_bytes() if dump_path.is_file() else b""
        verification = ""
        if proc.returncode == 0:
            verified = subprocess.run(
                [verifier, str(dump_path), *(verifier_args or [])],
                capture_output=True,
                text=True,
                check=False,
            )
            verification = verified.stdout + verified.stderr
            if verified.returncode != 0:
                return verified.returncode, proc.stdout, proc.stderr, dump, verification
        return proc.returncode, proc.stdout, proc.stderr, dump, verification


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


def run_single_test(
    mode: Mode,
    path: str,
    verifier: str,
    input_file: Path,
    failure_output_dir: Optional[Path],
    raw_output_dir: Optional[Path],
    enabled_features: set[str],
    clang_path: Optional[str] = None,
    global_flags: Optional[list[str]] = None,
    system_header_threshold: Optional[int] = 1,
) -> tuple[str, str]:
    """Run one corpus case and validate its full FlatBuffers record stream."""
    test_name = input_file.name
    try:
        config = get_test_config(test_name)
    except KeyError as error:
        return TestStatus.FAIL, str(error)

    missing_features = config.requires - enabled_features
    if missing_features:
        return TestStatus.SKIP, f"Missing features: {', '.join(sorted(missing_features))}"

    flags = list(global_flags or []) + config.flags
    verifier_args = []
    if config.expected_gcc_asm is not None:
        verifier_args.extend(
            ["--expect-gcc-asm-hex", config.expected_gcc_asm.encode("utf-8").hex()]
        )
    if config.expected_ms_asm is not None:
        verifier_args.extend(
            ["--expect-ms-asm-hex", config.expected_ms_asm.encode("utf-8").hex()]
        )
    if input_file.suffix == ".cu":
        flags.extend(flag for flag in CUDA_TEST_FLAGS if flag not in flags)
    return_code, stdout, stderr, dump, verification = run_flatbuffers_and_verify(
        mode,
        path,
        verifier,
        str(input_file),
        config.id,
        clang_path,
        flags,
        config.system_header_threshold
        if config.system_header_threshold is not None
        else system_header_threshold,
        verifier_args,
    )

    if raw_output_dir is not None:
        raw_output_dir.mkdir(parents=True, exist_ok=True)
        (raw_output_dir / f"{test_name}.clv2").write_bytes(dump)
        (raw_output_dir / f"{test_name}.stderr").write_text(stderr, encoding="utf-8")

    if return_code != 0:
        if failure_output_dir is not None:
            failure_output_dir.mkdir(parents=True, exist_ok=True)
            (failure_output_dir / f"{test_name}.stderr").write_text(stderr, encoding="utf-8")
            (failure_output_dir / f"{test_name}.clv2").write_bytes(dump)
        return TestStatus.FAIL, (
            f"Producer or FlatBuffers verifier exited with code {return_code}\n"
            f"Verifier: {verification or '(no verifier output)'}\n"
            f"Stderr:\n{stderr or '(empty)'}"
        )

    if not dump:
        return TestStatus.FAIL, "Producer returned success without writing a FlatBuffers stream"
    return TestStatus.PASS, verification.strip() or "complete FlatBuffers stream verified"


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
        "--verifier",
        required=True,
        help="Path to verify_flatbuffers built against the pinned v2 schema",
    )
    parser.add_argument(
        "--clang-path",
        default=None,
        help="Path to clang executable (required for plugin mode)",
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
        default=1,
        help=(
            "Positive system-header expansion threshold for test output "
            "(default: 1). Level N is expanded and its immediate children "
            "are serialized as boundary leaves; use 0 or a negative value "
            "for unlimited traversal."
        ),
    )
    parser.add_argument(
        "--failure-output-dir",
        default=None,
        help="Write failed FlatBuffers dumps and diagnostics for inspection.",
    )
    parser.add_argument(
        "--raw-output-dir",
        default=None,
        help=(
            "Write raw FlatBuffers dumps and stderr for each test to this "
            "directory, plus a _manifest.json file."
        ),
    )
    args = parser.parse_args()

    # Validate plugin mode requirements
    if args.mode == "plugin" and args.clang_path is None:
        parser.error("--clang-path is required when using --mode plugin")
    verifier_path = Path(args.verifier)
    if not verifier_path.is_file():
        parser.error(f"FlatBuffers verifier not found: {verifier_path}")

    # Resolve paths
    if args.test_dir:
        test_dir = Path(args.test_dir).resolve()
    else:
        test_dir = Path(__file__).resolve().parent

    inputs_dir = test_dir / "inputs"

    if not inputs_dir.exists():
        print(f"ERROR: Inputs directory not found: {inputs_dir}", file=sys.stderr)
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

    if raw_output_dir is not None:
        raw_output_dir.mkdir(parents=True, exist_ok=True)
        manifest = {
            "format": "flatbuffers-v2",
            "mode": args.mode,
            "inputs_dir": str(inputs_dir.resolve()),
            "enabled_features": sorted(enabled_features),
            "extra_clang_args": global_flags,
            "system_header_threshold": args.system_header_threshold,
            "verifier": str(verifier_path.resolve()),
        }
        (raw_output_dir / "_manifest.json").write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )

    num_workers = args.jobs if args.jobs is not None else os.cpu_count() or 1
    if num_workers < 1:
        parser.error("--jobs must be at least 1")
    print(
        f"Validating {len(tests)} test(s) "
        f"in {args.mode} mode using {num_workers} parallel workers..."
    )
    if enabled_features:
        print(f"Enabled features: {', '.join(sorted(enabled_features))}")
    if global_flags:
        print(f"Extra compiler args: {shlex.join(global_flags)}")
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
                verifier=str(verifier_path),
                input_file=test_file,
                failure_output_dir=failure_output_dir,
                raw_output_dir=raw_output_dir,
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
