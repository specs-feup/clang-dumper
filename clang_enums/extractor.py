"""
Enum extraction logic using regex parsing.
"""

import re
from dataclasses import dataclass
from typing import Optional


@dataclass
class ExtractedEnum:
    """Result of enum extraction."""
    name: str
    values: list[str]


class EnumExtractor:
    """
    Extracts enum values from preprocessed C++ source code.
    
    Uses multi-line regex matching for robust parsing, handling:
    - `enum Name { ... }` and `enum class Name { ... }`
    - Values with assignments (e.g., `Value = 0x10`)
    - Multiple values per line
    - Comments and preprocessor directives
    """
    
    # Regex to match enum definition opening
    # Captures: (1) optional 'class', (2) enum name, (3) everything after '{'
    # Note: The base type can be multi-word like "unsigned char"
    ENUM_PATTERN = re.compile(
        r'enum\s+(?:class\s+)?(\w+)\s*(?::[^{]+)?\s*\{',
        re.MULTILINE
    )

    # Maximum parenthesis nesting depth tolerated in the attribute/specifier
    # run between a class/struct keyword and its name (e.g.
    # `class __attribute__((visibility("default"))) W {`). Real-world
    # attributes nest far shallower than this.
    MAX_ATTRIBUTE_PAREN_DEPTH = 4

    @classmethod
    def extract(
        cls,
        content: str,
        enum_name: str,
        occurrence: int = 1,
        class_name: Optional[str] = None,
    ) -> list[str]:
        """
        Extract enum values from preprocessed C++ content.

        Args:
            content: The preprocessed C++ source code.
            enum_name: The name of the enum to extract.
            occurrence: Which occurrence to extract (1-based). Only used when
                       class_name is None. Useful when the same enum name appears
                       multiple times in different scopes.
            class_name: If set, anchor the search inside the body of this class
                       instead of relying on a global occurrence position.

        Returns:
            List of enum value names.

        Raises:
            ValueError: If the enum is not found or cannot be parsed.
        """
        # Find all occurrences of the enum definition
        # The pattern handles multi-word base types like "unsigned char"
        pattern = re.compile(
            rf'enum\s+(?:class\s+)?{re.escape(enum_name)}\s*(?::[^{{]+)?\s*\{{',
            re.MULTILINE
        )

        if class_name is not None:
            match = cls._find_enum_in_class(content, pattern, enum_name, class_name)
        else:
            matches = list(pattern.finditer(content))

            if not matches:
                raise ValueError(f"Enum '{enum_name}' not found in content")

            if occurrence > len(matches):
                raise ValueError(
                    f"Requested occurrence {occurrence} of enum '{enum_name}', "
                    f"but only {len(matches)} found"
                )

            # Get the specific occurrence (1-based index)
            match = matches[occurrence - 1]

        body_start = match.end()  # Position after the '{'
        body_end = cls._find_matching_brace(content, body_start - 1)

        if body_end is None:
            raise ValueError(f"Could not find closing brace for enum '{enum_name}'")

        # Parse enum values
        values = cls._parse_enum_body(content[body_start:body_end])

        if not values:
            raise ValueError(f"Enum '{enum_name}' extracted zero values")

        return values

    @classmethod
    def _find_enum_in_class(
        cls,
        content: str,
        enum_pattern: re.Pattern[str],
        enum_name: str,
        class_name: str,
    ) -> re.Match[str]:
        """
        Find the first enum matching enum_pattern declared directly inside the
        body of the named class, so extraction does not depend on global
        ordering. Enums declared inside nested classes/structs/enums are not
        considered.
        """
        class_pattern = re.compile(
            rf'\b(?:class|struct)\s+(?:{cls._attribute_run()})?'
            rf'{re.escape(class_name)}\b[^;{{}}]*\{{',
            re.MULTILINE
        )

        unbalanced_body = False

        for class_match in class_pattern.finditer(content):
            body_start = class_match.end()
            body_end = cls._find_matching_brace(content, body_start - 1)
            if body_end is None:
                unbalanced_body = True
                continue

            # Scan only this class body: skip over any nested {...} block
            # (nested class/struct/enum) so enums declared there never match.
            pos = body_start
            while pos < body_end:
                brace_pos = cls._find_outside_literal(content, '{', pos, body_end)
                enum_match = enum_pattern.search(content, pos, body_end)

                if enum_match is None:
                    break

                if brace_pos != -1 and brace_pos < enum_match.start():
                    nested_end = cls._find_matching_brace(content, brace_pos)
                    if nested_end is None:
                        unbalanced_body = True
                        break
                    pos = nested_end + 1
                    continue

                return enum_match

        if unbalanced_body:
            raise ValueError(
                f"Enum '{enum_name}' not found inside class '{class_name}': "
                f"class body has unbalanced braces"
            )
        raise ValueError(f"Enum '{enum_name}' not found inside class '{class_name}'")

    @classmethod
    def _attribute_run(cls) -> str:
        """
        Regex fragment matching a bounded run of attribute/specifier tokens
        that may appear between a class/struct keyword and its name.

        The run may contain parenthesized groups (e.g. GNU attributes) with
        balanced parentheses up to MAX_ATTRIBUTE_PAREN_DEPTH, but no braces or
        semicolons, so forward declarations can never match. The fragment is
        written so each input character has exactly one way to be consumed,
        avoiding catastrophic backtracking.
        """
        plain = r'[^;{}()]*'

        def group(depth: int) -> str:
            if depth == 0:
                return rf'\({plain}\)'
            return rf'\({plain}(?:{group(depth - 1)}{plain})*\)'

        balanced = group(cls.MAX_ATTRIBUTE_PAREN_DEPTH)
        return f'{plain}(?:{balanced}{plain})*'

    @classmethod
    def _find_matching_brace(cls, content: str, open_brace_pos: int) -> Optional[int]:
        """
        Return the position of the '}' matching the '{' at open_brace_pos,
        handling nested braces (e.g., in initializers) and skipping string and
        character literals so braces inside them do not affect matching,
        or None if unbalanced.
        """
        length = len(content)
        brace_count = 1
        pos = open_brace_pos + 1

        while pos < length:
            char = content[pos]
            if char == '"' or char == "'":
                pos = cls._skip_literal(content, pos)
                continue
            if char == '{':
                brace_count += 1
            elif char == '}':
                brace_count -= 1
                if brace_count == 0:
                    return pos
            pos += 1

        return None

    @staticmethod
    def _find_outside_literal(content: str, char: str, start: int, end: int) -> int:
        """
        Return the index of the first occurrence of char in content[start:end]
        that is not inside a string or character literal, or -1 if none.
        """
        length = len(content)
        pos = start

        while pos < end and pos < length:
            current = content[pos]
            if current == '"' or current == "'":
                pos = EnumExtractor._skip_literal(content, pos)
                continue
            if current == char:
                return pos
            pos += 1

        return -1

    @staticmethod
    def _skip_literal(content: str, start: int) -> int:
        """
        Return the index just past the string or character literal that opens
        with the quote at start, honoring backslash escapes.
        """
        quote = content[start]
        length = len(content)
        pos = start + 1

        while pos < length:
            char = content[pos]
            if char == '\\':
                pos += 2
            elif char == quote:
                return pos + 1
            else:
                pos += 1

        return pos
    
    @classmethod
    def _parse_enum_body(cls, body: str) -> list[str]:
        """
        Parse the body of an enum definition to extract value names.
        
        Handles:
        - Simple values: `Value,`
        - Values with assignments: `Value = 0x10,`
        - Inline comments
        - Preprocessor directives
        """
        values: list[str] = []
        
        # Remove comments
        body = cls._remove_comments(body)
        
        # Remove preprocessor line markers (from clang++ -E output)
        body = cls._remove_preprocessor_directives(body)
        
        # Split by comma, handling potential multi-line values
        # First, normalize whitespace
        body = re.sub(r'\s+', ' ', body)
        
        # Split by comma
        parts = body.split(',')
        
        for part in parts:
            part = part.strip()
            
            # Skip empty parts
            if not part:
                continue
            
            # Skip preprocessor directives (they shouldn't appear after -E, but just in case)
            if part.startswith('#'):
                continue
            
            # Extract the value name (before any '=' or other characters)
            value_name = cls._extract_value_name(part)
            
            if value_name:
                values.append(value_name)
        
        return values
    
    @classmethod
    def _remove_comments(cls, text: str) -> str:
        """Remove C and C++ style comments from text."""
        # Remove single-line comments
        text = re.sub(r'//[^\n]*', '', text)
        # Remove multi-line comments
        text = re.sub(r'/\*.*?\*/', '', text, flags=re.DOTALL)
        return text
    
    @classmethod
    def _remove_preprocessor_directives(cls, text: str) -> str:
        """Remove preprocessor line markers from text (e.g., # 123 "file.h" 3)."""
        # These appear after clang++ -E preprocessing
        # Pattern: # <number> "path" [flags]
        return re.sub(r'^#\s*\d+\s+"[^"]*".*$', '', text, flags=re.MULTILINE)
    
    @classmethod
    def _extract_value_name(cls, part: str) -> Optional[str]:
        """
        Extract the enum value name from a part of the enum body.
        
        Examples:
            'CK_BitCast' -> 'CK_BitCast'
            'CK_BitCast = 0' -> 'CK_BitCast'
            'LastKind = CK_BitCast' -> 'LastKind'
        """
        # Handle assignment
        if '=' in part:
            part = part.split('=')[0].strip()
        
        # Handle closing brace (last value might not have trailing comma)
        if '}' in part:
            part = part.split('}')[0].strip()
        
        # Validate it's a valid identifier
        if re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', part):
            return part
        
        return None
