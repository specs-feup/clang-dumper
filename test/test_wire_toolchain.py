#!/usr/bin/env python3
"""Exercise pinned FlatBuffers compiler/runtime checks and generated drift gate."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "scripts" / "generate_complete_wire.py"
INVENTORY = ROOT / "wire" / "generated.sha256"


def run_generator(flatc: Path, output: Path, inventory: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [
            "python3",
            str(GENERATOR),
            "--flatc",
            str(flatc),
            "--out",
            str(output),
            "--manifest",
            str(inventory),
        ],
        cwd=ROOT,
        capture_output=True,
        text=True,
        check=False,
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--flatbuffers-source", type=Path, required=True)
    parser.add_argument("--flatc", type=Path, required=True)
    args = parser.parse_args()
    flatbuffers_source = args.flatbuffers_source.resolve()
    flatc = args.flatc.resolve()
    pin = {}
    for line in (ROOT / "flatbuffers-version.env").read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("=")
        if separator:
            pin[key] = value
    version_parts = pin["FLATBUFFERS_VERSION"].split(".")
    if len(version_parts) != 3 or not version_parts[2].isdigit():
        raise SystemExit("FlatBuffers version must have three numeric components")
    wrong_version = ".".join(
        [version_parts[0], version_parts[1], str(int(version_parts[2]) - 1)]
    )
    if not flatbuffers_source.is_dir() or not flatc.is_file():
        parser.error("the configured pinned FlatBuffers source and flatc must exist")

    with tempfile.TemporaryDirectory(prefix="clang-dumper-wire-contract-") as temporary:
        temporary_dir = Path(temporary)
        baseline = run_generator(flatc, temporary_dir / "baseline", INVENTORY)
        if baseline.returncode != 0:
            raise SystemExit(f"pinned generator unexpectedly failed:\n{baseline.stderr}")

        bad_inventory = temporary_dir / "generated.sha256"
        inventory_text = INVENTORY.read_text(encoding="utf-8")
        first, rest = inventory_text[0], inventory_text[1:]
        bad_inventory.write_text(("0" if first != "0" else "1") + rest, encoding="utf-8")
        drift = run_generator(flatc, temporary_dir / "drift", bad_inventory)
        if drift.returncode == 0 or "Generated FlatBuffers outputs differ" not in drift.stderr:
            raise SystemExit("generated-output inventory drift was not rejected")

        wrong_flatc = temporary_dir / "wrong-flatc"
        wrong_flatc.write_text("#!/bin/sh\necho 'flatc version 0.0.0'\n", encoding="utf-8")
        wrong_flatc.chmod(0o755)
        compiler = run_generator(wrong_flatc, temporary_dir / "wrong-compiler", INVENTORY)
        if compiler.returncode == 0 or "Expected flatc" not in compiler.stderr:
            raise SystemExit("an unpinned flatc version was not rejected")

        base_header = flatbuffers_source / "include" / "flatbuffers" / "base.h"
        original_header = base_header.read_text(encoding="utf-8")
        marker = f"#define FLATBUFFERS_VERSION_REVISION {version_parts[2]}"
        if marker not in original_header:
            raise SystemExit("pinned FlatBuffers runtime revision marker changed")
        try:
            base_header.write_text(
                original_header.replace(
                    marker,
                    f"#define FLATBUFFERS_VERSION_REVISION {int(version_parts[2]) - 1}",
                    1,
                ),
                encoding="utf-8",
            )
            runtime_check = subprocess.run(
                [
                    "cmake",
                    "-S",
                    str(ROOT),
                    "-B",
                    str(temporary_dir / "wrong-runtime-build"),
                    "-DSKIP_ENUM_GENERATION=ON",
                    f"-DFETCHCONTENT_SOURCE_DIR_FLATBUFFERS={flatbuffers_source}",
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
                check=False,
            )
        finally:
            base_header.write_text(original_header, encoding="utf-8")
        mismatch = (
            f"runtime {wrong_version} does not match pinned "
            f"{pin['FLATBUFFERS_VERSION']}"
        )
        if runtime_check.returncode == 0 or mismatch not in runtime_check.stdout + runtime_check.stderr:
            raise SystemExit(
                "an unpinned FlatBuffers runtime was not rejected:\n"
                f"{runtime_check.stdout}\n{runtime_check.stderr}"
            )

    print("pinned flatc/runtime checks and generated-output drift rejection passed")


if __name__ == "__main__":
    main()
