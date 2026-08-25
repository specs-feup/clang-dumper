#!/usr/bin/env python3

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from clang_enums.extractor import EnumExtractor


class AnchoredExtractionTest(unittest.TestCase):
    HEADER = """
    class Kind { int x; };
    class Wrapper {
    public:
        enum Kind { A, B, C };
        struct Inner { enum Kind { X, Y }; };
    };
    enum Kind { WRONG };
    """

    def test_anchors_on_class_name(self) -> None:
        values = EnumExtractor.extract(self.HEADER, "Kind", class_name="Wrapper")
        self.assertEqual(values, ["A", "B", "C"])

    def test_prefers_first_enum_in_class_body(self) -> None:
        values = EnumExtractor.extract(self.HEADER, "Kind", occurrence=3)
        self.assertEqual(values, ["WRONG"])


class PositionalFallbackTest(unittest.TestCase):
    HEADER = """
    enum Color { RED, GREEN };
    enum Color { ALPHA };
    """

    def test_fallback_uses_occurrence(self) -> None:
        self.assertEqual(EnumExtractor.extract(self.HEADER, "Color"), ["RED", "GREEN"])
        self.assertEqual(
            EnumExtractor.extract(self.HEADER, "Color", occurrence=2), ["ALPHA"]
        )


class LoudFailureTest(unittest.TestCase):
    def test_missing_enum_raises(self) -> None:
        with self.assertRaisesRegex(ValueError, "'Missing'"):
            EnumExtractor.extract("enum Other { A };", "Missing")

    def test_missing_class_raises(self) -> None:
        with self.assertRaisesRegex(ValueError, "'Kind'.*'Missing'"):
            EnumExtractor.extract("enum Kind { A };", "Kind", class_name="Missing")

    def test_enum_outside_class_raises(self) -> None:
        with self.assertRaises(ValueError):
            EnumExtractor.extract("enum Kind { A };", "Kind", class_name="Wrapper")

    def test_empty_enum_raises(self) -> None:
        with self.assertRaisesRegex(ValueError, "zero values"):
            EnumExtractor.extract("enum Empty { };", "Empty")


if __name__ == "__main__":
    unittest.main()
