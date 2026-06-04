#!/usr/bin/env python3
"""
Clang Enum Extractor

Extracts C++ enum definitions from Clang/LLVM headers and generates:
- C++ string arrays for runtime enum-to-string conversion
- Java enum value lists

Usage:
    python -m clang_enums <llvm_dir> <output_dir>
    
    llvm_dir: Path to LLVM cmake directory (e.g., /usr/lib/llvm-18/lib/cmake/llvm)
    output_dir: Directory to write generated files to
"""

import argparse
import logging
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

from .codegen import write_cpp_file, write_java_files
from .config import HEADERS
from .extractor import EnumExtractor
from .models import EnumConfig, HeaderConfig
from .preprocessor import (
    PreprocessorError,
    get_clang_executable,
    get_include_dir,
    preprocess_header,
)

logger = logging.getLogger(__name__)


@dataclass
class ProcessedHeader:
    """Result of processing a single header file."""
    header_name: str
    enums: list[tuple[EnumConfig, list[str]]]
    errors: list[str]


def process_header(
    header_config: HeaderConfig,
    include_dir: Path,
    clang_executable: Path,
    clang_args: list[str],
) -> ProcessedHeader:
    """
    Process a single header file: preprocess and extract all configured enums.
    
    Args:
        header_config: Configuration for the header.
        include_dir: Path to the LLVM include directory.
        clang_executable: Path to the clang++ executable.
        
    Returns:
        ProcessedHeader with extracted enum values and any errors.
    """
    header_path = include_dir / header_config.path
    header_name = Path(header_config.path).name
    
    extracted_enums: list[tuple[EnumConfig, list[str]]] = []
    errors: list[str] = []
    
    try:
        content = preprocess_header(header_path, include_dir, clang_executable, clang_args)
    except PreprocessorError as e:
        errors.append(str(e))
        return ProcessedHeader(header_name, extracted_enums, errors)
    
    for enum_config in header_config.enums:
        try:
            values = EnumExtractor.extract(
                content,
                enum_config.name,
                enum_config.occurrence,
            )
            extracted_enums.append((enum_config, values))
            logger.debug(
                f"  Extracted {len(values)} values from '{enum_config.name}'"
            )
        except ValueError as e:
            errors.append(f"Error extracting '{enum_config.name}': {e}")
    
    return ProcessedHeader(header_name, extracted_enums, errors)


def main(argv: Optional[list[str]] = None) -> int:
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Extract enum definitions from Clang/LLVM headers.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "llvm_dir",
        type=Path,
        help="Path to LLVM cmake directory (e.g., /usr/lib/llvm-18/lib/cmake/llvm)",
    )
    parser.add_argument(
        "output_dir",
        type=Path,
        help="Directory to write generated files to",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Enable verbose logging",
    )
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=4,
        help="Number of parallel jobs (default: 4)",
    )
    parser.add_argument(
        "--clang-executable",
        type=Path,
        help="clang++ executable to use for preprocessing LLVM headers",
    )
    parser.add_argument(
        "--clang-arg",
        action="append",
        default=[],
        help="Extra argument to pass to clang++ while preprocessing; may be repeated",
    )
    
    args = parser.parse_args(argv)
    
    # Configure logging
    log_level = logging.DEBUG if args.verbose else logging.INFO
    logging.basicConfig(
        level=log_level,
        format="%(levelname)s: %(message)s",
    )
    
    # Validate inputs
    llvm_dir = args.llvm_dir.resolve()
    if not llvm_dir.exists():
        logger.error(f"LLVM directory does not exist: {llvm_dir}")
        return 1
    
    # Set up paths
    clang_executable = args.clang_executable or get_clang_executable(llvm_dir)
    include_dir = get_include_dir(llvm_dir)
    
    if not clang_executable.exists():
        logger.error(f"Clang executable not found: {clang_executable}")
        return 1
    
    if not include_dir.exists():
        logger.error(f"Include directory not found: {include_dir}")
        return 1
    
    logger.info(f"Using LLVM directory: {llvm_dir}")
    logger.info(f"Using clang++: {clang_executable}")
    logger.info(f"Using include directory: {include_dir}")
    if args.clang_arg:
        logger.info(f"Using extra clang++ args: {' '.join(args.clang_arg)}")
    
    # Create output directories
    output_dir = args.output_dir.resolve()
    cpp_output_dir = output_dir / "enums_cpp"
    java_output_dir = output_dir / "enums_java"
    
    cpp_output_dir.mkdir(parents=True, exist_ok=True)
    java_output_dir.mkdir(parents=True, exist_ok=True)
    
    logger.info(f"Output directory: {output_dir}")
    
    # Process headers in parallel
    all_errors: list[str] = []
    
    with ThreadPoolExecutor(max_workers=args.jobs) as executor:
        futures = {
            executor.submit(
                process_header, header_config, include_dir, clang_executable, args.clang_arg
            ): header_config
            for header_config in HEADERS
        }
        
        for future in as_completed(futures):
            header_config = futures[future]
            try:
                result = future.result()
            except Exception as e:
                logger.error(f"Unexpected error processing {header_config.path}: {e}")
                all_errors.append(str(e))
                continue
            
            if result.errors:
                for error in result.errors:
                    logger.error(error)
                all_errors.extend(result.errors)
            
            if result.enums:
                # Write C++ file
                cpp_path = write_cpp_file(cpp_output_dir, result.header_name, result.enums)
                logger.info(f"Generated: {cpp_path}")
                
                # Write Java files
                java_paths = write_java_files(java_output_dir, result.header_name, result.enums)
                for path in java_paths:
                    logger.info(f"Generated: {path}")
    
    if all_errors:
        logger.warning(f"Completed with {len(all_errors)} error(s)")
        return 1
    
    logger.info("Done!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
