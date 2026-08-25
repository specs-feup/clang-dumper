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

    def test_global_occurrence_selects_third_definition(self) -> None:
        values = EnumExtractor.extract(self.HEADER, "Kind", occurrence=3)
        self.assertEqual(values, ["WRONG"])


class NestedScopeTest(unittest.TestCase):
    def test_skips_nested_class_declared_before_enum(self) -> None:
        header = """
        class Wrapper {
            struct Inner { enum Kind { X, Y }; };
            enum Kind { A, B };
        };
        """
        values = EnumExtractor.extract(header, "Kind", class_name="Wrapper")
        self.assertEqual(values, ["A", "B"])


class LiteralBracesTest(unittest.TestCase):
    def test_ignores_braces_inside_string_and_char_literals(self) -> None:
        header = '''
        class Holder {
            const char* open = "{";
            char close = '}';
            char escaped = '\\"';
            enum Mode { ON, OFF };
        };
        '''
        values = EnumExtractor.extract(header, "Mode", class_name="Holder")
        self.assertEqual(values, ["ON", "OFF"])


class AnchorToleranceTest(unittest.TestCase):
    def test_attributed_class_anchor(self) -> None:
        header = (
            'class __attribute__((visibility("default"))) W {\n'
            "public:\n"
            "    enum Kind { Q };\n"
            "};\n"
        )
        values = EnumExtractor.extract(header, "Kind", class_name="W")
        self.assertEqual(values, ["Q"])

    def test_attributed_forward_declaration_does_not_match(self) -> None:
        header = 'class __attribute__((visibility("default"))) W;\n'
        with self.assertRaisesRegex(ValueError, "'Kind'.*'W'"):
            EnumExtractor.extract(header, "Kind", class_name="W")

    def test_inheritance_clause_anchor(self) -> None:
        header = """
        class Base { };
        class Derived : public Base {
            enum Kind { P, Q };
        };
        """
        values = EnumExtractor.extract(header, "Kind", class_name="Derived")
        self.assertEqual(values, ["P", "Q"])


class ExplicitBaseTypeTest(unittest.TestCase):
    def test_enum_with_explicit_base_type(self) -> None:
        values = EnumExtractor.extract(
            "enum Kind : unsigned char { LOW, HIGH };", "Kind"
        )
        self.assertEqual(values, ["LOW", "HIGH"])

    def test_scoped_enum_with_explicit_base_type_in_class(self) -> None:
        header = """
        class Flags {
            enum class Kind : unsigned char { R, W, X };
        };
        """
        values = EnumExtractor.extract(header, "Kind", class_name="Flags")
        self.assertEqual(values, ["R", "W", "X"])


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

    def test_unbalanced_class_body_raises_specific_reason(self) -> None:
        header = "class Wrapper { enum Kind { A };"
        with self.assertRaisesRegex(ValueError, "unbalanced braces"):
            EnumExtractor.extract(header, "Kind", class_name="Wrapper")

    def test_empty_enum_raises(self) -> None:
        with self.assertRaisesRegex(ValueError, "zero values"):
            EnumExtractor.extract("enum Empty { };", "Empty")


if __name__ == "__main__":
    unittest.main()
