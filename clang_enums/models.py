"""
Data models for enum extraction configuration.
"""

from dataclasses import dataclass, field
from typing import Callable, Optional


# Type alias for mapper functions
Mapper = Callable[[str], str]


def identity(value: str) -> str:
    """Default mapper that returns the value unchanged."""
    return value


def remove_prefix(prefix: str) -> Mapper:
    """Create a mapper that removes a prefix from enum values."""
    def mapper(value: str) -> str:
        return value.removeprefix(prefix)
    return mapper


def remove_suffix(suffix: str) -> Mapper:
    """Create a mapper that removes a suffix from enum values."""
    def mapper(value: str) -> str:
        return value.removesuffix(suffix)
    return mapper


def remove_prefix_and_upper(prefix: str) -> Mapper:
    """Create a mapper that removes a prefix and uppercases the result."""
    def mapper(value: str) -> str:
        return value.removeprefix(prefix).upper()
    return mapper


def to_upper() -> Mapper:
    """Create a mapper that uppercases the value."""
    return lambda value: value.upper()


def lookup_table(table: dict[str, str]) -> Mapper:
    """Create a mapper that uses a lookup table."""
    def mapper(value: str) -> str:
        return table.get(value, value)
    return mapper


@dataclass
class EnumConfig:
    """Configuration for a single enum to extract."""
    
    name: str
    """The name of the enum in the C++ source (e.g., 'CastKind')."""
    
    cpp_var_name: str
    """The name for the generated C++ variable (e.g., 'CAST_KIND')."""
    
    mapper: Mapper = field(default_factory=lambda: identity)
    """Function to transform enum value names."""
    
    exclude: set[str] = field(default_factory=set)
    """Set of enum values to exclude from output."""
    
    class_name: Optional[str] = None
    """If the enum is inside a class, the class name."""
    
    occurrence: int = 1
    """Which occurrence of the enum to extract (1-based). 
    Some headers have multiple enums with the same name in different scopes."""
    
    @property
    def complete_name(self) -> str:
        """Get the full name including class prefix if applicable."""
        if self.class_name:
            return f"{self.class_name}_{self.name}"
        return self.name


@dataclass
class HeaderConfig:
    """Configuration for a header file containing enums to extract."""
    
    path: str
    """Relative path from LLVM include directory (e.g., 'clang/AST/Type.h')."""
    
    enums: list[EnumConfig] = field(default_factory=list)
    """List of enums to extract from this header."""
