#!/usr/bin/env python3
"""Package the complete wire schema and write local or release manifests."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import zipfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCHEMA_DIR = ROOT / "wire" / "v2"
SCHEMA_ASSET = "clang-dumper-wire-schema-v2.zip"
MANIFEST_NAME = "clang-dumper-release-manifest.json"


def flatbuffers_pin() -> tuple[str, str]:
    values = {}
    for line in (ROOT / "flatbuffers-version.env").read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        key, value = line.split("=", 1)
        values[key] = value
    return values["FLATBUFFERS_VERSION"], values["FLATBUFFERS_COMMIT"]


def llvm_major_pin() -> int:
    for line in (ROOT / "llvm-version.env").read_text(encoding="utf-8").splitlines():
        if line.startswith("LLVM_VERSION="):
            return int(line.partition("=")[2])
    raise RuntimeError("llvm-version.env has no LLVM_VERSION")


def schema_files() -> list[Path]:
    return sorted(SCHEMA_DIR.glob("*.fbs"))


def schema_hash(files: list[Path]) -> str:
    digest = hashlib.sha256()
    for path in files:
        name = path.relative_to(ROOT).as_posix().encode("utf-8")
        digest.update(name + b"\0" + path.read_bytes() + b"\0")
    return digest.hexdigest()


def write_schema_archive(destination: Path) -> tuple[Path, str]:
    destination.parent.mkdir(parents=True, exist_ok=True)
    files = schema_files()
    with zipfile.ZipFile(
        destination, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9
    ) as archive:
        for path in files:
            name = path.relative_to(ROOT).as_posix()
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, path.read_bytes(), compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    return destination, schema_hash(files)


def asset_entry(path: Path, platform: str, arch: str, kind: str, llvm_major: int | None = None) -> dict:
    entry = {
        "filename": path.name,
        "platform": platform,
        "arch": arch,
        "kind": kind,
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
    }
    if llvm_major is not None:
        entry["llvm_major"] = llvm_major
    return entry


def manifest_data(schema_digest: str, schema_asset_digest: str, assets: list[dict]) -> dict:
    flatbuffers_version, flatbuffers_commit = flatbuffers_pin()
    return {
        "schema_version": 2,
        "flatbuffers": {
            "version": flatbuffers_version,
            "commit": flatbuffers_commit,
        },
        "wire_schema": {
            "version": 2,
            "entrypoint": "wire/v2/complete.fbs",
            "asset": SCHEMA_ASSET,
            "sha256": schema_digest,
            "asset_sha256": schema_asset_digest,
            "flatbuffers_version": flatbuffers_version,
        },
        "assets": assets,
    }


def write_json(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def classify_release_asset(filename: str) -> tuple[str, str, str]:
    if filename == "clang-dumper-generated-enums.zip":
        return "any", "any", "generated-enums"
    if filename == SCHEMA_ASSET:
        return "any", "any", "wire-schema"

    patterns = (
        (r"^clang-dumper-(linux|macos)-(.+?)-plugin[.]so$", "plugin"),
        (r"^clang-dumper-(linux|macos)-(.+?)-includes[.]zip$", "includes"),
        (r"^clang-dumper-(linux|macos)-(.+)$", "tool"),
        (r"^clang-dumper-windows-(.+?)[.]exe$", "tool"),
        (r"^clang-dumper-windows-(.+?)-includes[.]zip$", "includes"),
    )
    for pattern, kind in patterns:
        match = re.match(pattern, filename)
        if not match:
            continue
        if filename.startswith("clang-dumper-windows-"):
            return "windows", match.group(1), kind
        return match.group(1), match.group(2), kind
    raise ValueError(f"Unrecognized release asset filename: {filename}")


def write_release(asset_dir: Path, llvm_major: int) -> None:
    asset_dir.mkdir(parents=True, exist_ok=True)
    schema_path, digest = write_schema_archive(asset_dir / SCHEMA_ASSET)
    schema_asset_digest = hashlib.sha256(schema_path.read_bytes()).hexdigest()
    assets = []
    for path in sorted(asset_dir.iterdir()):
        if not path.is_file() or path.name == MANIFEST_NAME:
            continue
        platform, arch, kind = classify_release_asset(path.name)
        assets.append(asset_entry(path, platform, arch, kind, llvm_major))
    write_json(
        asset_dir / MANIFEST_NAME,
        manifest_data(digest, schema_asset_digest, assets),
    )


def write_local_build(output_dir: Path, tool_path: Path, platform: str, arch: str) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    schema_path, digest = write_schema_archive(output_dir / SCHEMA_ASSET)
    schema_asset_digest = hashlib.sha256(schema_path.read_bytes()).hexdigest()
    llvm_major = llvm_major_pin()
    assets = [
        asset_entry(schema_path, "any", "any", "wire-schema"),
        asset_entry(tool_path, platform, arch, "tool", llvm_major),
    ]
    write_json(
        output_dir / MANIFEST_NAME,
        manifest_data(digest, schema_asset_digest, assets),
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--asset-dir", type=Path, help="Write the release schema bundle and manifest here")
    modes.add_argument("--local-output-dir", type=Path, help="Write a local build manifest here")
    parser.add_argument("--llvm-major", type=int)
    parser.add_argument("--tool", type=Path)
    parser.add_argument("--platform", choices=("linux", "macos", "windows"))
    parser.add_argument("--arch")
    args = parser.parse_args()

    if args.asset_dir:
        if args.llvm_major is None:
            parser.error("--asset-dir requires --llvm-major")
        write_release(args.asset_dir, args.llvm_major)
        return

    if not args.tool or not args.tool.is_file():
        parser.error("--local-output-dir requires an existing --tool file")
    if not args.platform or not args.arch:
        parser.error("--local-output-dir requires --platform and --arch")
    output_dir = args.local_output_dir.resolve()
    tool_path = args.tool.resolve()
    if tool_path.parent != output_dir:
        parser.error("the local tool must be in the manifest output directory")
    write_local_build(output_dir, tool_path, args.platform, args.arch)


if __name__ == "__main__":
    main()
