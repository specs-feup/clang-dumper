import unittest

from corpus_runner import CorpusJob, aggregate_bucket, extract_jobs, to_driver_flags


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
                    ["-std", "c++17", "-faligned-alloc-unavailable", "-DMACOS"],
                    True,
                ),
                CorpusJob(["-std", "c++17", "-DNO_ERRORS"], False),
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


if __name__ == "__main__":
    unittest.main()
