import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from corpus_runner import (
    CorpusJob,
    _parse_node_td,
    _parse_tablegen_inheritance,
    aggregate_bucket,
    extract_jobs,
    load_inventory,
    missing_requirements,
    parse_requires,
    summarize_diagnostics,
    to_driver_flags,
)


class CorpusRunnerTests(unittest.TestCase):
    def test_extract_jobs_keeps_run_configurations_independent(self):
        jobs, reason = extract_jobs(
            """// RUN: %clang_cc1 -std=c++17 -faligned-alloc-unavailable -verify -DMACOS %s
// RUN: %clang_cc1 -std=c++17 -DNO_ERRORS %s
"""
        )

        self.assertIsNone(reason)
        self.assertEqual(
            jobs,
            [
                CorpusJob(
                    ["-std=c++17", "-faligned-alloc-unavailable", "-DMACOS"],
                    True,
                ),
                CorpusJob(["-std=c++17", "-DNO_ERRORS"], False),
            ],
        )

    def test_aggregate_clean_and_expected_error_is_clean(self):
        self.assertEqual(aggregate_bucket(["EXPECTED_ERR", "CLEAN"]), "CLEAN")

    def test_aggregate_mixed_jobs_is_partial(self):
        self.assertEqual(aggregate_bucket(["CLEAN", "PARSE_FAIL"]), "PARTIAL")

    def test_driver_translation_preserves_cc1_target_options(self):
        flags, dropped = to_driver_flags([
            "-target-feature", "+simd128",
            "-target-cpu", "x86-64",
            "-target-abi", "gnu",
            "-mfpmath", "vfp",
        ])

        self.assertEqual(dropped, [])
        self.assertEqual(
            flags,
            [
                "-Xclang", "-target-feature", "-Xclang", "+simd128",
                "-Xclang", "-target-cpu", "-Xclang", "x86-64",
                "-Xclang", "-target-abi", "-Xclang", "gnu",
                "-Xclang", "-mfpmath", "-Xclang", "vfp",
            ],
        )

        flags, dropped = to_driver_flags(["-faligned-alloc-unavailable"])
        self.assertEqual(flags, ["-Xclang", "-faligned-alloc-unavailable"])
        self.assertEqual(dropped, [])

    def test_requires_supports_and_groups_and_or_alternatives(self):
        requirements = parse_requires(
            """// REQUIRES: x86-registered-target, posix
// REQUIRES: aarch64-registered-target || arm-registered-target
"""
        )

        self.assertEqual(
            missing_requirements(requirements, {"x86-registered-target", "posix"}),
            ["aarch64-registered-target || arm-registered-target"],
        )

    def test_summarize_diagnostics_keeps_compiler_errors(self):
        summary = summarize_diagnostics(
            "protocol output\nfoo.c:4:2: error: invalid target\n"
            "more dump output\n"
        )
        self.assertEqual(summary, "foo.c:4:2: error: invalid target")

    def test_extract_jobs_expands_shell_quotes_and_lit_source_directory(self):
        jobs, reason = extract_jobs(
            "// RUN: %clang_cc1 -include %S/Inputs/header.h "
            "-DNAME='A B' -verify=expected %s\n",
            Path("/tmp/clang/test/Sema"),
        )

        self.assertIsNone(reason)
        self.assertEqual(
            jobs,
            [CorpusJob(
                [
                    "-include", "/tmp/clang/test/Sema/Inputs/header.h",
                    "-DNAME=A B",
                ],
                True,
            )],
        )

    def test_inventory_parses_multiline_nodes_and_transitive_attributes(self):
        with TemporaryDirectory() as directory:
            root = Path(directory)
            basic = root / "include" / "clang" / "Basic"
            basic.mkdir(parents=True)
            (basic / "StmtNodes.td").write_text(
                "def Stmt : StmtNode<?, 1>;\n"
                "def Expr : StmtNode<Stmt, 1>;\n"
                "def ReturnStmt : StmtNode<Stmt>;\n"
            )
            (basic / "DeclNodes.td").write_text(
                "def Decl : DeclNode<?, \"\", 1>;\n"
                "def\nThing : DeclNode<Decl>;\n"
            )
            (basic / "TypeNodes.td").write_text(
                "def Type : TypeNode<?, 1>;\n"
                "def AlreadyType : TypeNode<Type>;\n"
            )
            (basic / "Attr.td").write_text(
                "class Attr {}\n"
                "class InheritableAttr : Attr {}\n"
                "class InheritableParamAttr : InheritableAttr {}\n"
                "def Direct : Attr {}\n"
                "def Inherited : InheritableParamAttr {}\n"
            )

            nodes, abstract = _parse_node_td(basic / "DeclNodes.td")
            self.assertEqual(nodes["Thing"], "Decl")
            self.assertEqual(abstract, {"Decl"})

            parents, definitions = _parse_tablegen_inheritance(
                basic / "Attr.td"
            )
            self.assertEqual(parents["InheritableParamAttr"],
                             {"InheritableAttr"})
            self.assertEqual(definitions, {"Direct", "Inherited"})

            inventory = load_inventory(root / "test")
            self.assertEqual(inventory["decl data"], ["ThingDecl"])
            self.assertEqual(inventory["type data"], ["AlreadyType"])
            self.assertEqual(
                inventory["attr data"], ["DirectAttr", "InheritedAttr"]
            )


if __name__ == "__main__":
    unittest.main()
