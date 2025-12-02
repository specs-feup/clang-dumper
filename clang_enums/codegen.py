"""
Code generation for C++ and Java enum outputs.
"""

from pathlib import Path

from .models import EnumConfig


def generate_cpp_array(enum_config: EnumConfig, values: list[str]) -> str:
    """
    Generate a C++ std::string array definition for the enum values.
    
    Args:
        enum_config: The enum configuration.
        values: The raw enum values extracted from the header.
        
    Returns:
        C++ code defining the string array.
    """
    lines = [f"const std::string clava::{enum_config.cpp_var_name}[] = {{"]
    
    for value in values:
        # Skip excluded values
        if value in enum_config.exclude:
            continue
        
        # Apply mapper
        mapped_value = enum_config.mapper(value)
        lines.append(f'        "{mapped_value}",')
    
    lines.append("};")
    
    return "\n".join(lines)


def generate_cpp_file(enums: list[tuple[EnumConfig, list[str]]]) -> str:
    """
    Generate a complete C++ file with all enum arrays.
    
    Args:
        enums: List of (enum_config, values) tuples.
        
    Returns:
        Complete C++ file content.
    """
    parts = ['#include "../ClangEnums/ClangEnums.h"\n']
    
    for enum_config, values in enums:
        parts.append(generate_cpp_array(enum_config, values))
        parts.append("")  # Empty line between arrays
    
    return "\n".join(parts)


def generate_java_enum_values(enum_config: EnumConfig, values: list[str]) -> str:
    """
    Generate Java enum values list.
    
    Args:
        enum_config: The enum configuration.
        values: The raw enum values extracted from the header.
        
    Returns:
        Comma-separated list of enum values ending with semicolon.
    """
    # Filter excluded and map values
    mapped_values = [
        enum_config.mapper(v)
        for v in values
        if v not in enum_config.exclude
    ]
    
    return ",\n".join(mapped_values) + ";"


def write_cpp_file(
    output_dir: Path,
    header_name: str,
    enums: list[tuple[EnumConfig, list[str]]],
) -> Path:
    """
    Write a C++ file with enum string arrays.
    
    Args:
        output_dir: Directory to write the file to.
        header_name: Name of the source header (e.g., "Type.h").
        enums: List of (enum_config, values) tuples.
        
    Returns:
        Path to the written file.
    """
    filename = f"enums_{header_name.replace('.', '_')}.cpp"
    output_path = output_dir / filename
    
    content = generate_cpp_file(enums)
    output_path.write_text(content)
    
    return output_path


def write_java_files(
    output_dir: Path,
    header_name: str,
    enums: list[tuple[EnumConfig, list[str]]],
) -> list[Path]:
    """
    Write Java enum value files (one per enum).
    
    Args:
        output_dir: Directory to write the files to.
        header_name: Name of the source header (e.g., "Type.h").
        enums: List of (enum_config, values) tuples.
        
    Returns:
        List of paths to the written files.
    """
    written_files = []
    
    for enum_config, values in enums:
        filename = f"enums_{header_name.replace('.', '_')}_{enum_config.complete_name}.txt"
        output_path = output_dir / filename
        
        content = generate_java_enum_values(enum_config, values)
        output_path.write_text(content)
        
        written_files.append(output_path)
    
    return written_files
