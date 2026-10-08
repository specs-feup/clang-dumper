#!/usr/bin/env python3

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))

from run_tests import (
    TestStatus,
    check_address_consistency,
    normalize_captured_output,
    normalize_static_output,
    normalize_system_source_blocks,
    run_single_test,
    strip_clang_diagnostics,
    unresolved_node_ids,
)


def clang_dumper_tool() -> Path:
    configured_path = os.environ.get("CLANG_DUMPER_TOOL")
    if configured_path:
        return Path(configured_path)
    return Path(__file__).resolve().parents[1] / "build" / "tool"


def protobuf_verifier() -> Path:
    configured_path = os.environ.get("CLANG_DUMPER_VERIFY_PROTOBUF")
    if configured_path:
        return Path(configured_path)
    return Path(__file__).resolve().parents[1] / "build" / "verify_protobuf"


@unittest.skipUnless(
    clang_dumper_tool().is_file() and protobuf_verifier().is_file(),
    "build/tool and build/verify_protobuf are required for stream integration tests",
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

    def assert_valid_dump(self, path: Path) -> bytes:
        dump = path.read_bytes()
        self.assertTrue(dump.startswith(b"CLAVAPB1"), "missing Protobuf AST stream magic")
        result = subprocess.run(
            [str(protobuf_verifier()), str(path)],
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        return dump

    def test_standalone_output_requires_a_file(self) -> None:
        result = self.run_tool(self.source)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("-o <path> is required for standalone protobuf output", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_syntax_check_does_not_produce_a_protocol_stream(self) -> None:
        command = [
            str(clang_dumper_tool()),
            "-syntax-check-only",
            "-c",
            str(self.source),
            "-id=42",
            "-system-header-threshold=1",
            "--",
        ]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "")
        self.assertNotIn("CLAVAPB1", result.stderr)

    def test_syntax_check_keeps_diagnostics_on_stderr(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            invalid_source = Path(directory) / "invalid.cpp"
            invalid_source.write_text("int main( {\n", encoding="utf-8")
            command = [
                str(clang_dumper_tool()),
                "-syntax-check-only",
                "-c",
                str(invalid_source),
                "-id=42",
                "-system-header-threshold=1",
                "--",
            ]
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(result.stdout, "")
            self.assertIn("error:", result.stderr)
            self.assertNotIn("CLAVAPB1", result.stderr)

    def test_file_output_is_a_separate_protocol_stream(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast.dump"
            separated = self.run_tool(self.source, output)
            self.assertEqual(separated.returncode, 0, separated.stderr)
            self.assertEqual(output.read_bytes()[:8], b"CLAVAPB1")
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
            plain_dump = self.assert_valid_dump(plain_output)
            decompressed_path = Path(directory) / "decompressed.pb"
            decompressed_path.write_bytes(decompressed.stdout)
            self.assertEqual(self.assert_valid_dump(decompressed_path), plain_dump)

    def test_diagnostics_remain_on_stderr(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast dump with spaces.dump"
            result = self.run_tool(self.throwing_source, output)
            dump = self.assert_valid_dump(output)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("error:", result.stderr)
            self.assertNotIn("<Compiler Instance Data>", result.stderr)
            self.assertTrue(dump.startswith(b"CLAVAPB1"))

    def test_file_output_truncates_existing_contents(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast.dump"
            output.write_text("stale data\n", encoding="utf-8")
            result = self.run_tool(self.source, output)
            self.assertEqual(result.returncode, 0)
            self.assert_valid_dump(output)
            self.assertNotIn(b"stale data", output.read_bytes())

    def test_bad_file_output_path_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "missing" / "ast.dump"
            result = self.run_tool(self.source, output)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Cannot open AST dump output", result.stderr)

    def test_rejects_multiple_sources_for_one_output(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast.dump"
            command = [
                str(clang_dumper_tool()),
                "-c",
                str(self.source),
                str(Path(__file__).parent / "inputs" / "includes.cpp"),
                "-o",
                str(output),
                "--",
            ]
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("require exactly one source file", result.stderr)

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


    def test_accepts_ccache_injected_color_flag_in_tool_arguments(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "ast.dump"
            command = [
                str(clang_dumper_tool()),
                "-fcolor-diagnostics",
                "-c",
                str(self.source),
                "-id=42",
                "-system-header-threshold=1",
                "-o",
                str(output),
                "--",
                "-std=c++17",
            ]
            result = subprocess.run(command, capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(output.is_file())

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

    def test_generated_roots_keep_relative_paths_across_invocations(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            roots = [root / "A", root / "C"]
            for parse_root in roots:
                (parse_root / "include").mkdir(parents=True)
                (parse_root / "include" / "value.h").write_text(
                    "#define VALUE 7\n", encoding="utf-8"
                )
                (parse_root / "src.cpp").write_text(
                    '#include "include/value.h"\n'
                    '#warning generated-root-warning\n'
                    'const char *path = __FILE__;\n'
                    'const char *base = __BASE_FILE__;\n'
                    "int value = VALUE;\n",
                    encoding="utf-8",
                )

            dumps: list[bytes] = []
            dump_paths: list[bytes] = []
            diagnostics: list[str] = []
            dependencies: list[str] = []
            for parse_root in roots:
                output = root / f"{parse_root.name}.dump"
                dependency = root / f"{parse_root.name}.d"
                result = subprocess.run(
                    [
                        str(clang_dumper_tool()),
                        "-c",
                        "-o",
                        str(output),
                        "-MD",
                        "-MF",
                        str(dependency),
                        "src.cpp",
                        "--",
                        "-I.",
                    ],
                    cwd=parse_root,
                    capture_output=True,
                    text=True,
                    check=False,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                dump_bytes = self.assert_valid_dump(output)
                normalized_dump = dump_bytes.replace(
                    str(parse_root.resolve()).encode(), b"<PARSE_ROOT>"
                )
                dumps.append(normalized_dump)
                dump_paths.append(dump_bytes)
                diagnostics.append(result.stderr)
                dependencies.append(dependency.read_text(encoding="utf-8"))

            self.assertEqual(dumps[0], dumps[1])
            self.assertEqual(diagnostics[0], diagnostics[1])
            self.assertEqual(
                dependencies[0].split(": ", 1)[1],
                dependencies[1].split(": ", 1)[1],
            )
            self.assertIn("src.cpp", diagnostics[0])
            self.assertNotIn(str(root / "A"), diagnostics[0])
            self.assertIn("src.cpp", dependencies[0])
            self.assertNotIn(str(root / "A" / "src.cpp"), dependencies[0])
            self.assertNotIn(str(root / "A").encode(), dump_paths[0])
            self.assertNotIn(str(root / "C").encode(), dump_paths[1])
            self.assertIn(b"src.cpp", dump_paths[0])

            cache = root / "cache"
            cache_environment = os.environ.copy()
            cache_environment.update(
                CCACHE_DIR=str(cache),
                CCACHE_COMPILERTYPE="clang",
                CCACHE_DEPEND="true",
                CCACHE_NOHASHDIR="true",
                CCACHE_NOCOMPRESS="true",
            )
            cache_commands = []
            for parse_root in roots:
                output = parse_root / "ast.dump"
                dependency = parse_root / "ast.d"
                self.assertFalse(output.exists())
                self.assertFalse(dependency.exists())
                cache_commands.append(
                    (
                        parse_root,
                        output,
                        dependency,
                    )
                )

            first_root, first_output, first_dependency = cache_commands[0]
            first = subprocess.run(
                [
                    "ccache",
                    str(clang_dumper_tool()),
                    "-c",
                    "-o",
                    "ast.dump",
                    "-MD",
                    "-MF",
                    "ast.d",
                    "src.cpp",
                    "--",
                    "-I.",
                ],
                cwd=first_root,
                capture_output=True,
                text=True,
                check=False,
                env=cache_environment,
            )
            self.assertEqual(first.returncode, 0, first.stderr)
            first_dump = first_output.read_bytes()
            self.assertTrue(first_dependency.is_file())

            second_root, second_output, second_dependency = cache_commands[1]
            self.assertFalse(second_output.exists())
            self.assertFalse(second_dependency.exists())
            second = subprocess.run(
                [
                    "ccache",
                    str(clang_dumper_tool()),
                    "-c",
                    "-o",
                    "ast.dump",
                    "-MD",
                    "-MF",
                    "ast.d",
                    "src.cpp",
                    "--",
                    "-I.",
                ],
                cwd=second_root,
                capture_output=True,
                text=True,
                check=False,
                env=cache_environment,
            )
            stats = subprocess.run(
                ["ccache", "--print-stats"],
                capture_output=True,
                text=True,
                check=True,
                env=cache_environment,
            ).stdout

            self.assertEqual(second.returncode, 0, second.stderr)
            self.assertEqual(second_output.read_bytes(), first_dump)
            self.assertTrue(second_dependency.is_file())
            self.assertIn("cache_miss\t1", stats)
            self.assertIn("direct_cache_hit\t1", stats)
            self.assert_valid_dump(second_output)
            self.assertNotIn(str(root / "A").encode(), second_output.read_bytes())


class ProtobufValidationModeTest(unittest.TestCase):
    stream = b"CLAVAPB1\x00test-stream"

    def write_executable(self, path: Path, contents: str) -> Path:
        path.write_text(contents, encoding="utf-8")
        path.chmod(0o755)
        return path

    def run_fixture(
        self, verifier_accepts_stream: bool
    ) -> tuple[str, str, bytes, bytes | None]:
        with tempfile.TemporaryDirectory(prefix="clang-dumper-runner-protobuf-") as temp:
            root = Path(temp)
            input_file = root / "inputs" / "simple_function.cpp"
            input_file.parent.mkdir()
            input_file.write_text("int value;\n", encoding="utf-8")
            tool = self.write_executable(
                root / "fake-tool",
                "#!/usr/bin/env python3\n"
                "import pathlib, sys\n"
                "args = sys.argv[1:]\n"
                "pathlib.Path(args[args.index('-o') + 1]).write_bytes("
                + repr(self.stream)
                + ")\n",
            )
            verifier_exit = 0 if verifier_accepts_stream else 1
            verifier = self.write_executable(
                root / "fake-verifier",
                "#!/usr/bin/env python3\n"
                "import pathlib, sys\n"
                "sys.exit("
                + str(verifier_exit)
                + " if pathlib.Path(sys.argv[1]).read_bytes() == "
                + repr(self.stream)
                + " else 1)\n",
            )
            raw_dir = root / "raw"
            failures = root / "failures"
            result = run_single_test(
                mode="tool",
                path=str(tool),
                input_file=input_file,
                expected_dir=root / "expected",
                platform_expected_dirs=[],
                failure_output_dir=failures,
                raw_output_dir=raw_dir,
                inputs_dir_str=str(input_file.parent),
                generate=False,
                enabled_features=set(),
                protobuf_verifier=str(verifier),
            )
            raw_copy = (raw_dir / "simple_function.cpp.pb").read_bytes()
            failure_copy_path = failures / "simple_function.cpp.pb"
            failure_copy = (
                failure_copy_path.read_bytes()
                if failure_copy_path.exists()
                else None
            )
            return result[0], result[1], raw_copy, failure_copy

    def test_valid_stream_is_checked_without_reading_text_snapshots(self) -> None:
        status, message, raw_copy, failure_copy = self.run_fixture(True)
        self.assertEqual(status, TestStatus.PASS)
        self.assertEqual(message, "Valid Protobuf AST stream")
        self.assertEqual(raw_copy, self.stream)
        self.assertIsNone(failure_copy)

    def test_invalid_stream_is_preserved_as_binary_failure_output(self) -> None:
        status, message, raw_copy, failure_copy = self.run_fixture(False)
        self.assertEqual(status, TestStatus.FAIL)
        self.assertIn("Protobuf AST stream verification failed", message)
        self.assertEqual(raw_copy, self.stream)
        self.assertEqual(failure_copy, self.stream)

    def test_plugin_stream_and_diagnostics_are_captured_separately(self) -> None:
        with tempfile.TemporaryDirectory(prefix="clang-dumper-runner-plugin-") as temp:
            root = Path(temp)
            input_file = root / "inputs" / "simple_function.cpp"
            input_file.parent.mkdir()
            input_file.write_text("int value;\n", encoding="utf-8")
            plugin = root / "libplugin.so"
            plugin.write_bytes(b"fixture plugin")
            clang = self.write_executable(
                root / "fake-clang",
                "#!/usr/bin/env python3\n"
                "import sys\n"
                "assert '-w' not in sys.argv\n"
                "sys.stdout.buffer.write(" + repr(self.stream) + ")\n"
                "sys.stderr.write('fixture warning: kept on stderr\\n')\n",
            )
            verifier = self.write_executable(
                root / "fake-verifier",
                "#!/usr/bin/env python3\n"
                "import pathlib, sys\n"
                "sys.exit(0 if pathlib.Path(sys.argv[1]).read_bytes() == "
                + repr(self.stream)
                + " else 1)\n",
            )
            raw_dir = root / "raw"
            status, message = run_single_test(
                mode="plugin",
                path=str(plugin),
                input_file=input_file,
                expected_dir=root / "expected",
                platform_expected_dirs=[],
                failure_output_dir=root / "failures",
                raw_output_dir=raw_dir,
                inputs_dir_str=str(input_file.parent),
                generate=False,
                enabled_features=set(),
                clang_path=str(clang),
                protobuf_verifier=str(verifier),
            )

            self.assertEqual(status, TestStatus.PASS)
            self.assertEqual(message, "Valid Protobuf AST stream")
            self.assertEqual(
                (raw_dir / "simple_function.cpp.pb").read_bytes(), self.stream
            )
            self.assertEqual(
                (raw_dir / "simple_function.cpp.stderr").read_text(encoding="utf-8"),
                "fixture warning: kept on stderr\n",
            )


class PythonProtobufPinTest(unittest.TestCase):
    def test_python_pin_is_separate_and_used_by_ci(self) -> None:
        root = Path(__file__).resolve().parents[1]
        pins = dict(
            line.split("=", 1)
            for line in (root / "protobuf-version.env").read_text().splitlines()
            if line and not line.startswith("#")
        )
        self.assertEqual(pins["PROTOBUF_PYTHON_VERSION"], "5.28.3")
        self.assertEqual(pins["PROTOBUF_VERSION"], "28.3")
        self.assertEqual(pins["PROTOC_VERSION"], "28.3")
        self.assertEqual(pins["JAVA_PROTOC_MINIMUM_VERSION"], "4.28.3")
        self.assertEqual(pins["JAVA_RUNTIME_MINIMUM_VERSION"], "4.28.3")

        workflow = (root / ".github" / "workflows" / "build.yml").read_text()
        venv_creation = 'python3 -m venv "${PROTOBUF_VENV}"'
        pip_command = (
            '"${PROTOBUF_VENV}/bin/python" -m pip install '
            '--only-binary=:all: "protobuf==${PROTOBUF_PYTHON_VERSION}"'
        )
        self.assertEqual(workflow.count(venv_creation), 2)
        self.assertEqual(workflow.count(pip_command), 2)
        self.assertEqual(workflow.count('PROTOBUF_VENV="${RUNNER_TEMP}/protobuf-python-venv"'), 2)
        self.assertEqual(workflow.count('echo "${PROTOBUF_VENV}/bin" >> "$GITHUB_PATH"'), 1)
        self.assertEqual(
            workflow.count('echo "PROTOBUF_VENV=${PROTOBUF_VENV}" >> "$GITHUB_ENV"'), 1
        )
        self.assertEqual(
            workflow.count('-DPython3_EXECUTABLE="${PROTOBUF_VENV}/bin/python"'), 1
        )
        self.assertEqual(workflow.count("source protobuf-version.env"), 2)


@unittest.skipUnless(shutil.which("cmake"), "CMake is required for archive tool selection tests")
class MacOSArchiveToolSelectionTest(unittest.TestCase):
    def test_macos_static_libraries_use_the_active_xcode_archiver_pair(self) -> None:
        root = Path(__file__).resolve().parents[1]
        workflow = (root / ".github" / "workflows" / "build.yml").read_text()
        linux_job = workflow.split("  linux:", 1)[1].split("  macos:", 1)[0]
        macos_job = workflow.split("  macos:", 1)[1].split("  windows:", 1)[0]

        self.assertNotIn("APPLE_AR", linux_job)
        self.assertIn('echo "APPLE_AR=$(xcrun --find ar)" >> "$GITHUB_ENV"', macos_job)
        self.assertIn('echo "APPLE_RANLIB=$(xcrun --find ranlib)" >> "$GITHUB_ENV"', macos_job)
        self.assertIn('-DCMAKE_AR="${APPLE_AR}"', macos_job)
        self.assertIn('-DCMAKE_RANLIB="${APPLE_RANLIB}"', macos_job)
        self.assertIn("grep -E '^CMAKE_(AR|RANLIB):' build/CMakeCache.txt", macos_job)

    def test_cmake_uses_explicit_archive_tools_in_static_library_rules(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            source_dir = Path(tmp_dir)
            (source_dir / "sample.cc").write_text("int sample() { return 0; }\n")
            (source_dir / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(ArchiveToolSelection LANGUAGES CXX)\n"
                "add_library(sample STATIC sample.cc)\n"
            )
            build_dir = source_dir / "build"
            result = subprocess.run(
                [
                    "cmake",
                    "-G",
                    "Unix Makefiles",
                    "-S",
                    str(source_dir),
                    "-B",
                    str(build_dir),
                    "-DCMAKE_AR=/xcode/toolchain/usr/bin/ar",
                    "-DCMAKE_RANLIB=/xcode/toolchain/usr/bin/ranlib",
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

            archive_rule = (build_dir / "CMakeFiles" / "sample.dir" / "link.txt").read_text()
            self.assertIn("/xcode/toolchain/usr/bin/ar qc", archive_rule)
            self.assertIn("/xcode/toolchain/usr/bin/ranlib", archive_rule)


@unittest.skipUnless(shutil.which("cmake"), "CMake is required for interpreter selection tests")
class PythonInterpreterSelectionTest(unittest.TestCase):
    def test_cmake_honors_the_explicit_python_interpreter(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            temp_root = Path(tmp_dir)
            source_dir = temp_root / "cmake-source"
            source_dir.mkdir()
            (source_dir / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(PythonInterpreterSelection NONE)\n"
                "find_package(Python3 COMPONENTS Interpreter REQUIRED)\n"
                'file(WRITE "${CMAKE_BINARY_DIR}/selected-python.txt" "${Python3_EXECUTABLE}")\n'
            )
            build_dir = temp_root / "cmake-build"
            subprocess.run(
                [
                    "cmake",
                    "-S",
                    str(source_dir),
                    "-B",
                    str(build_dir),
                    f"-DPython3_EXECUTABLE={sys.executable}",
                ],
                check=True,
                capture_output=True,
                text=True,
            )

            selected_python = Path((build_dir / "selected-python.txt").read_text())
            self.assertEqual(selected_python.resolve(), Path(sys.executable).resolve())


@unittest.skipUnless(shutil.which("cmake"), "CMake is required for dependency patch tests")
class WindowsCOFFProtobufPatchTest(unittest.TestCase):
    def test_pinned_patch_is_scoped_idempotent_and_rejects_drift(self) -> None:
        root = Path(__file__).resolve().parents[1]
        patch_script = root / "cmake" / "patch_protobuf_28_3_for_coff.cmake"
        cmake_lists = (root / "CMakeLists.txt").read_text()
        self.assertIn("if(WIN32 AND CMAKE_CROSSCOMPILING)", cmake_lists)
        self.assertIn(
            "target_compile_definitions(absl_synchronization PRIVATE _WIN32_WINNT=0x0600)",
            cmake_lists,
        )
        self.assertIn("clang_dumper_enable_mingw_cctz_winstring", cmake_lists)
        self.assertIn("patch_protobuf_28_3_for_coff.cmake", cmake_lists)
        old_guard = (
            "#if defined(__GNUC__) && defined(__clang__) && !defined(__APPLE__) && \\\n"
            "    !defined(_MSC_VER)\n"
            "#define PROTOBUF_DESCRIPTOR_WEAK_MESSAGES_ALLOWED\n"
        )
        new_guard = (
            "#if defined(__GNUC__) && defined(__clang__) && !defined(__APPLE__) && \\\n"
            "    !defined(_MSC_VER) && !defined(_WIN32)\n"
            "#define PROTOBUF_DESCRIPTOR_WEAK_MESSAGES_ALLOWED\n"
        )

        def invoke(source_dir: Path, version: str) -> subprocess.CompletedProcess[str]:
            return subprocess.run(
                [
                    "cmake",
                    f"-DPROTOBUF_SOURCE_DIR={source_dir}",
                    f"-DPROTOBUF_SOURCE_VERSION={version}",
                    "-P",
                    str(patch_script),
                ],
                capture_output=True,
                text=True,
                check=False,
            )

        with tempfile.TemporaryDirectory() as tmp_dir:
            source_dir = Path(tmp_dir)
            port_def = source_dir / "src" / "google" / "protobuf" / "port_def.inc"
            port_def.parent.mkdir(parents=True)
            port_def.write_text("before\n" + old_guard + "after\n")

            first = invoke(source_dir, "28.3")
            self.assertEqual(first.returncode, 0, first.stderr)
            first_result = port_def.read_text()
            self.assertEqual(first_result, "before\n" + new_guard + "after\n")

            second = invoke(source_dir, "28.3")
            self.assertEqual(second.returncode, 0, second.stderr)
            self.assertEqual(port_def.read_text(), first_result)

            wrong_version = invoke(source_dir, "28.4")
            self.assertNotEqual(wrong_version.returncode, 0)
            self.assertIn("audited only for Protobuf 28.3", wrong_version.stderr)

            port_def.write_text("changed upstream guard\n")
            changed_source = invoke(source_dir, "28.3")
            self.assertNotEqual(changed_source.returncode, 0)
            self.assertIn("feature guard changed", changed_source.stderr)

    def test_winstring_option_is_visible_in_abseil_target_directory_only(self) -> None:
        root = Path(__file__).resolve().parents[1]
        helper = root / "cmake" / "enable_mingw_cctz_winstring.cmake"
        with tempfile.TemporaryDirectory() as tmp_dir:
            source_dir = Path(tmp_dir)
            nested_dir = source_dir / "absl" / "time"
            nested_dir.mkdir(parents=True)
            source = nested_dir / "time_zone_lookup.cc"
            source.write_text("// compile option scope fixture\n")
            (nested_dir / "CMakeLists.txt").write_text(
                f'add_custom_target(absl_time_zone SOURCES "{source.as_posix()}")\n'
            )
            cmake_lists = f"""cmake_minimum_required(VERSION 3.20)
project(WinstringScope NONE)
add_subdirectory(absl/time)
include("{helper.as_posix()}")
clang_dumper_enable_mingw_cctz_winstring("{source.as_posix()}" absl_time_zone)
get_source_file_property(target_options "{source.as_posix()}" TARGET_DIRECTORY absl_time_zone COMPILE_OPTIONS)
if(NOT "-include" IN_LIST target_options OR NOT "winstring.h" IN_LIST target_options)
    message(FATAL_ERROR "winstring include options missing from the Abseil target directory: ${{target_options}}")
endif()
get_source_file_property(root_options "{source.as_posix()}" COMPILE_OPTIONS)
if(NOT root_options STREQUAL "NOTFOUND")
    message(FATAL_ERROR "winstring options leaked into the parent directory: ${{root_options}}")
endif()
"""
            (source_dir / "CMakeLists.txt").write_text(cmake_lists)
            result = subprocess.run(
                ["cmake", "-S", str(source_dir), "-B", str(source_dir / "build")],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


@unittest.skipUnless(shutil.which("cmake"), "CMake is required for Abseil option tests")
class AbseilArchitectureFlagGroupingTest(unittest.TestCase):
    def test_cmake_keeps_each_architecture_selector_with_its_flag(self) -> None:
        root = Path(__file__).resolve().parents[1]
        helper = root / "cmake" / "group_abseil_arch_copts.cmake"
        top_level = (root / "CMakeLists.txt").read_text()
        self.assertIn('if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")', top_level)

        with tempfile.TemporaryDirectory() as tmp_dir:
            source_dir = Path(tmp_dir)
            source = source_dir / "fixture.cc"
            source.write_text("int abseil_arch_options_fixture() { return 0; }\n")
            cmake_lists = f"""cmake_minimum_required(VERSION 3.20)
project(AbseilArchitectureFlagGrouping LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(_randen_options
    -Xarch_x86_64 -maes
    -Xarch_x86_64 -msse4.1
    -Xarch_arm64 -march=armv8-a+crypto)
foreach(_target IN ITEMS absl_random_internal_randen_hwaes absl_random_internal_randen_hwaes_impl)
    add_library(${{_target}} OBJECT "${{CMAKE_CURRENT_SOURCE_DIR}}/fixture.cc")
    target_compile_options(${{_target}} PRIVATE ${{_randen_options}} -Wall)
endforeach()
include("{helper.as_posix()}")
clang_dumper_group_abseil_arch_copts(absl_random_internal_randen_hwaes)
clang_dumper_group_abseil_arch_copts(absl_random_internal_randen_hwaes_impl)
"""
            (source_dir / "CMakeLists.txt").write_text(cmake_lists)
            build_dir = source_dir / "build"
            result = subprocess.run(
                ["cmake", "-S", str(source_dir), "-B", str(build_dir)],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

            compile_commands = json.loads((build_dir / "compile_commands.json").read_text())
            self.assertEqual(len(compile_commands), 2)
            for entry in compile_commands:
                command = entry["command"].split()
                x86_selectors = [
                    index for index, option in enumerate(command) if option == "-Xarch_x86_64"
                ]
                arm_selectors = [
                    index for index, option in enumerate(command) if option == "-Xarch_arm64"
                ]
                self.assertEqual(len(x86_selectors), 2, command)
                self.assertEqual(len(arm_selectors), 1, command)
                self.assertEqual(command[x86_selectors[0] + 1], "-maes", command)
                self.assertEqual(command[x86_selectors[1] + 1], "-msse4.1", command)
                self.assertEqual(command[arm_selectors[0] + 1], "-march=armv8-a+crypto", command)
                self.assertIn("-Wall", command)


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


class AddressConsistencyTest(unittest.TestCase):
    def test_accepts_single_format(self) -> None:
        self.assertEqual(
            check_address_consistency(
                {"ADDR_001": ["0x7fffff939e08_1", "0x7fffff939e08_1"]}
            ),
            [],
        )
        self.assertEqual(
            check_address_consistency(
                {"ADDR_001": ["00007fffff939e08_1", "00007fffff939e08_1"]}
            ),
            [],
        )

    def test_rejects_mixed_address_formats(self) -> None:
        # Same pointer emitted in LLVM raw_ostream format (0x-prefixed) and
        # CRT %p format (zero-padded, no prefix): the signature of a node ID
        # bypassing clava::getId, which breaks Clava on Windows.
        errors = check_address_consistency(
            {"ADDR_001": ["0x7fffff93a898_1", "00007fffff93a898_1"]}
        )
        self.assertEqual(len(errors), 1)
        self.assertIn("Inconsistent address format", errors[0])

    def test_rejects_different_addresses(self) -> None:
        errors = check_address_consistency(
            {"ADDR_001": ["0x7fffff93a898_1", "0x7fffff9390f0_1"]}
        )
        self.assertEqual(len(errors), 1)
        self.assertIn("Inconsistent address for", errors[0])


if __name__ == "__main__":
    unittest.main()
