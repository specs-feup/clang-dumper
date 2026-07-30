"""
C++ header preprocessing using clang++ -E.
"""

import logging
import subprocess
from pathlib import Path

logger = logging.getLogger(__name__)


class PreprocessorError(Exception):
    """Raised when preprocessing a header fails."""
    pass


def preprocess_header(
    header_path: Path,
    include_dir: Path,
    clang_executable: Path,
    clang_args: list[str] | None = None,
) -> str:
    """
    Preprocess a C++ header file using clang++ -E.
    
    This expands all macros and includes, producing a flat text file
    that can be easily parsed for enum definitions.
    
    Args:
        header_path: Absolute path to the header file.
        include_dir: Path to the LLVM include directory.
        clang_executable: Path to the clang++ executable.
    
    Returns:
        The preprocessed content as a string.
        
    Raises:
        PreprocessorError: If preprocessing fails.
    """
    args = [
        str(clang_executable),
        "-Wno-deprecated",
        "-E",  # Preprocess only
        *(clang_args or []),
        str(header_path),
        "-isystem", str(include_dir),
    ]
    
    logger.debug(f"Running: {' '.join(args)}")
    
    try:
        result = subprocess.run(
            args,
            capture_output=True,
            text=True,
            check=False,  # We'll check manually for better error messages
        )
    except FileNotFoundError:
        raise PreprocessorError(
            f"Clang executable not found: {clang_executable}\n"
            "Make sure LLVM/Clang is installed and the path is correct."
        )
    
    if result.returncode != 0:
        raise PreprocessorError(
            f"Failed to preprocess '{header_path}':\n"
            f"Command: {' '.join(args)}\n"
            f"Exit code: {result.returncode}\n"
            f"stderr:\n{result.stderr}"
        )
    
    logger.info(f"Preprocessed '{header_path.name}'")
    return result.stdout


def get_clang_executable(llvm_dir: Path) -> Path:
    """
    Get the path to clang++ from the LLVM directory.
    
    The LLVM directory is expected to be the cmake config dir,
    e.g., /usr/lib/llvm-<version>/lib/cmake/llvm
    
    Args:
        llvm_dir: Path to the LLVM cmake directory.
        
    Returns:
        Path to the clang++ executable.
    """
    import platform
    
    # Navigate from lib/cmake/llvm to the bin directory
    llvm_prefix = llvm_dir.resolve().parent.parent.parent
    
    exe_name = "clang++.exe" if platform.system() == "Windows" else "clang++"
    return llvm_prefix / "bin" / exe_name


def get_include_dir(llvm_dir: Path) -> Path:
    """
    Get the path to the LLVM include directory.
    
    Args:
        llvm_dir: Path to the LLVM cmake directory.
        
    Returns:
        Path to the include directory.
    """
    llvm_prefix = llvm_dir.resolve().parent.parent.parent
    return llvm_prefix / "include"
