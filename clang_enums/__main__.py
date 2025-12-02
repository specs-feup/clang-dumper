"""
Entry point for running as a module: python -m clang_enums
"""

from .main import main
import sys

if __name__ == "__main__":
    sys.exit(main())
