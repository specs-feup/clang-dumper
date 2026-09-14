#!/usr/bin/env python3
"""Generate the source-schema hash embedded in the protobuf stream header."""

import argparse
import hashlib
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--schema", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    digest = hashlib.sha256(args.schema.read_bytes()).hexdigest()
    content = (
        "#pragma once\n"
        "namespace clava::proto {\n"
        "inline constexpr char ProtoSchemaHash[] = \""
        f"{digest}"
        "\";\n"
        "} // namespace clava::proto\n"
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.output.exists() and args.output.read_text() == content:
        return
    args.output.write_text(content)


if __name__ == "__main__":
    main()
