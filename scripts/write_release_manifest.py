#!/usr/bin/env python3
"""Write and verify Protobuf clang-dumper release and local manifests."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

from google.protobuf import descriptor_pb2


ROOT = Path(__file__).resolve().parents[1]
SCHEMA_NAME = "clang-dumper-ast-wire.proto"
DESCRIPTOR_NAME = "clang-dumper-ast-wire.pb"
MANIFEST_NAME = "clang-dumper-release-manifest.json"
SCHEMA_VERSION = 1
PROTOCOL_MAJOR = 1
PROTOCOL_MINOR = 1
SEMANTIC_CONTRACT = "clava-ast-wire-v1"
FRAMING = "CLAVAPB1 plus protobuf varint-delimited Envelope(Chunk)"
MAX_RECORD_BYTES = 64 * 1024 * 1024
LLVM18_OPENMP_STMT_CLASSES = (
    "OMPAtomicDirective",
    "OMPBarrierDirective",
    "OMPCancelDirective",
    "OMPCancellationPointDirective",
    "OMPCanonicalLoop",
    "OMPCriticalDirective",
    "OMPDepobjDirective",
    "OMPDispatchDirective",
    "OMPDistributeDirective",
    "OMPDistributeParallelForDirective",
    "OMPDistributeParallelForSimdDirective",
    "OMPDistributeSimdDirective",
    "OMPErrorDirective",
    "OMPFlushDirective",
    "OMPForDirective",
    "OMPForSimdDirective",
    "OMPGenericLoopDirective",
    "OMPInteropDirective",
    "OMPMaskedDirective",
    "OMPMaskedTaskLoopDirective",
    "OMPMaskedTaskLoopSimdDirective",
    "OMPMasterDirective",
    "OMPMasterTaskLoopDirective",
    "OMPMasterTaskLoopSimdDirective",
    "OMPMetaDirective",
    "OMPOrderedDirective",
    "OMPParallelDirective",
    "OMPParallelForDirective",
    "OMPParallelForSimdDirective",
    "OMPParallelGenericLoopDirective",
    "OMPParallelMaskedDirective",
    "OMPParallelMaskedTaskLoopDirective",
    "OMPParallelMaskedTaskLoopSimdDirective",
    "OMPParallelMasterDirective",
    "OMPParallelMasterTaskLoopDirective",
    "OMPParallelMasterTaskLoopSimdDirective",
    "OMPParallelSectionsDirective",
    "OMPScanDirective",
    "OMPScopeDirective",
    "OMPSectionDirective",
    "OMPSectionsDirective",
    "OMPSimdDirective",
    "OMPSingleDirective",
    "OMPTargetDataDirective",
    "OMPTargetDirective",
    "OMPTargetEnterDataDirective",
    "OMPTargetExitDataDirective",
    "OMPTargetParallelDirective",
    "OMPTargetParallelForDirective",
    "OMPTargetParallelForSimdDirective",
    "OMPTargetParallelGenericLoopDirective",
    "OMPTargetSimdDirective",
    "OMPTargetTeamsDirective",
    "OMPTargetTeamsDistributeDirective",
    "OMPTargetTeamsDistributeParallelForDirective",
    "OMPTargetTeamsDistributeParallelForSimdDirective",
    "OMPTargetTeamsDistributeSimdDirective",
    "OMPTargetTeamsGenericLoopDirective",
    "OMPTargetUpdateDirective",
    "OMPTaskDirective",
    "OMPTaskLoopDirective",
    "OMPTaskLoopSimdDirective",
    "OMPTaskgroupDirective",
    "OMPTaskwaitDirective",
    "OMPTaskyieldDirective",
    "OMPTeamsDirective",
    "OMPTeamsDistributeDirective",
    "OMPTeamsDistributeParallelForDirective",
    "OMPTeamsDistributeParallelForSimdDirective",
    "OMPTeamsDistributeSimdDirective",
    "OMPTeamsGenericLoopDirective",
    "OMPTileDirective",
    "OMPUnrollDirective",
)


def pins() -> dict[str, str]:
    result: dict[str, str] = {}
    path = ROOT / "protobuf-version.env"
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        key, separator, value = line.partition("=")
        if not separator or not key or not value:
            raise ValueError(f"Invalid toolchain pin in {path}: {raw_line!r}")
        if key in result:
            raise ValueError(f"Duplicate toolchain pin {key!r} in {path}")
        result[key] = value
    required = {
        "PROTOBUF_VERSION",
        "PROTOC_VERSION",
        "JAVA_PROTOC_MINIMUM_VERSION",
        "JAVA_RUNTIME_MINIMUM_VERSION",
    }
    missing = required - result.keys()
    if missing:
        raise ValueError(f"Missing toolchain pins in {path}: {sorted(missing)}")
    if result["PROTOBUF_VERSION"] != result["PROTOC_VERSION"]:
        raise ValueError("Native protoc and C++ runtime pins must match exactly")
    return result


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def copy_if_changed(source: Path, destination: Path) -> None:
    if destination.is_file() and sha256(source) == sha256(destination):
        return
    shutil.copy2(source, destination)


def write_if_changed(path: Path, contents: str) -> None:
    if path.is_file() and path.read_text(encoding="utf-8") == contents:
        return
    path.write_text(contents, encoding="utf-8")


def verify_descriptor(schema: Path, descriptor: Path, protoc: Path | None = None) -> None:
    if not schema.is_file():
        raise ValueError(f"Protobuf schema is missing: {schema}")
    if not descriptor.is_file():
        raise ValueError(f"Protobuf descriptor set is missing: {descriptor}")

    descriptor_set = descriptor_pb2.FileDescriptorSet()
    try:
        descriptor_set.ParseFromString(descriptor.read_bytes())
    except Exception as error:  # protobuf raises DecodeError, varies by runtime
        raise ValueError(f"Invalid Protobuf descriptor set: {descriptor}") from error
    matching = [
        item
        for item in descriptor_set.file
        if item.name == "clava_ast_wire.proto"
    ]
    if len(matching) != 1 or matching[0].package != "astwire.v1":
        raise ValueError(
            "Descriptor set must contain exactly one astwire.v1/clava_ast_wire.proto file"
        )

    if protoc is None:
        return

    expected_protoc = pins()["PROTOC_VERSION"]
    version = subprocess.run(
        [str(protoc), "--version"], check=True, capture_output=True, text=True
    ).stdout.strip()
    if version != f"libprotoc {expected_protoc}":
        raise ValueError(
            f"Native protoc version mismatch: expected libprotoc {expected_protoc}, got {version}"
        )
    with tempfile.TemporaryDirectory(prefix="clang-dumper-descriptor-") as temporary:
        temporary_root = Path(temporary)
        schema_copy = temporary_root / "clava_ast_wire.proto"
        shutil.copyfile(schema, schema_copy)
        generated = temporary_root / "descriptor.pb"
        subprocess.run(
            [
                str(protoc),
                f"--proto_path={temporary_root}",
                f"--descriptor_set_out={generated}",
                str(schema_copy),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        if generated.read_bytes() != descriptor.read_bytes():
            raise ValueError(
                "Descriptor set does not match the selected schema and pinned protoc"
            )


def classify(filename: str) -> tuple[str, str, str]:
    if filename in {SCHEMA_NAME, DESCRIPTOR_NAME}:
        return "any", "any", "protocol"
    if filename == "clang-dumper-generated-enums.zip":
        return "any", "any", "generated-enums"

    patterns = (
        (r"^clang-dumper-(linux|macos)-(.+?)-plugin[.]so$", "plugin"),
        (r"^clang-dumper-(linux|macos)-(.+?)-includes[.]zip$", "includes"),
        (r"^clang-dumper-(linux|macos)-(.+)$", "tool"),
        (r"^clang-dumper-windows-(.+?)[.]exe$", "tool"),
        (r"^clang-dumper-windows-(.+?)-includes[.]zip$", "includes"),
    )
    for pattern, kind in patterns:
        match = re.match(pattern, filename)
        if match is None:
            continue
        if filename.startswith("clang-dumper-windows-"):
            return "windows", match.group(1), kind
        return match.group(1), match.group(2), kind
    raise ValueError(f"Unrecognized clang-dumper release asset: {filename}")


def asset_entry(path: Path, platform: str, arch: str, kind: str, llvm_major: int) -> dict:
    return {
        "filename": path.name,
        "platform": platform,
        "arch": arch,
        "kind": kind,
        "llvm_major": llvm_major,
        "sha256": sha256(path),
    }


def manifest_data(schema: Path, descriptor: Path, assets: list[dict], llvm_major: int) -> dict:
    if llvm_major != 18:
        raise ValueError(
            f"LLVM {llvm_major} has no checked generic-payload contract yet; "
            "update the OMP class inventory before publishing this release"
        )
    toolchain = pins()
    return {
        "schema_version": SCHEMA_VERSION,
        "protocol": {
            "id": "clava-ast-wire",
            "major": PROTOCOL_MAJOR,
            "minor": PROTOCOL_MINOR,
            "semantic_contract": SEMANTIC_CONTRACT,
            "framing": FRAMING,
            "max_record_bytes": MAX_RECORD_BYTES,
            "producer_version": f"clang-dumper-{llvm_major}",
            "llvm_major": llvm_major,
            "schema_sha256": sha256(schema),
            "descriptor_sha256": sha256(descriptor),
        },
        "toolchain": {
            "protobuf_version": toolchain["PROTOBUF_VERSION"],
            "protoc_version": toolchain["PROTOC_VERSION"],
        },
        "compatibility": {
            "minimum_java_protoc_version": toolchain["JAVA_PROTOC_MINIMUM_VERSION"],
            "minimum_java_runtime_version": toolchain["JAVA_RUNTIME_MINIMUM_VERSION"],
        },
        "generic_payload_contracts": {
            "attributes": {
                "class_name_pattern": "<closed AttributeKind enum value>Attr",
                "payload": "AttributeData",
            },
            "openmp": {
                "class_names": list(LLVM18_OPENMP_STMT_CLASSES),
                "payload": "StmtData",
            },
        },
        "assets": assets,
    }


def verify_manifest(manifest_path: Path) -> None:
    root = manifest_path.parent.resolve()
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema_version") != SCHEMA_VERSION:
        raise ValueError("Unsupported clang-dumper release manifest version")
    protocol = manifest.get("protocol")
    if not isinstance(protocol, dict):
        raise ValueError("Manifest protocol metadata is missing")
    expected_protocol = {
        "id": "clava-ast-wire",
        "major": PROTOCOL_MAJOR,
        "minor": PROTOCOL_MINOR,
        "semantic_contract": SEMANTIC_CONTRACT,
        "framing": FRAMING,
        "max_record_bytes": MAX_RECORD_BYTES,
    }
    for key, value in expected_protocol.items():
        if protocol.get(key) != value:
            raise ValueError(f"Unsupported protocol metadata {key}: {protocol.get(key)!r}")
    llvm_major = protocol.get("llvm_major")
    if not isinstance(llvm_major, int) or llvm_major <= 0:
        raise ValueError("Manifest protocol LLVM major must be a positive integer")
    expected_pins = pins()
    expected_fields = {
        "protobuf_version": expected_pins["PROTOBUF_VERSION"],
        "protoc_version": expected_pins["PROTOC_VERSION"],
    }
    if manifest.get("toolchain") != expected_fields:
        raise ValueError("Manifest native Protobuf toolchain does not match pinned versions")
    expected_compatibility = {
        "minimum_java_protoc_version": expected_pins["JAVA_PROTOC_MINIMUM_VERSION"],
        "minimum_java_runtime_version": expected_pins["JAVA_RUNTIME_MINIMUM_VERSION"],
    }
    if manifest.get("compatibility") != expected_compatibility:
        raise ValueError("Manifest Java Protobuf compatibility does not match pinned versions")
    if llvm_major != 18:
        raise ValueError(
            f"LLVM {llvm_major} has no checked generic-payload contract yet; "
            "update the OMP class inventory before publishing this release"
        )
    expected_generic_contracts = {
        "attributes": {
            "class_name_pattern": "<closed AttributeKind enum value>Attr",
            "payload": "AttributeData",
        },
        "openmp": {
            "class_names": list(LLVM18_OPENMP_STMT_CLASSES),
            "payload": "StmtData",
        },
    }
    if manifest.get("generic_payload_contracts") != expected_generic_contracts:
        raise ValueError("Manifest generic-payload contracts do not match the pinned LLVM schema")

    assets = manifest.get("assets")
    if not isinstance(assets, list) or not assets:
        raise ValueError("Manifest must contain release assets")
    by_name: dict[str, Path] = {}
    for entry in assets:
        if not isinstance(entry, dict):
            raise ValueError("Manifest asset entries must be objects")
        filename = entry.get("filename")
        if not isinstance(filename, str) or Path(filename).name != filename:
            raise ValueError(f"Invalid manifest asset filename: {filename!r}")
        if filename in by_name:
            raise ValueError(f"Duplicate manifest asset filename: {filename}")
        if entry.get("kind") not in {"tool", "plugin", "includes", "protocol", "generated-enums"}:
            raise ValueError(f"Invalid manifest asset kind for {filename}: {entry.get('kind')!r}")
        if not isinstance(entry.get("platform"), str) or not isinstance(entry.get("arch"), str):
            raise ValueError(f"Manifest platform/architecture is missing for {filename}")
        path = (root / filename).resolve()
        if path.parent != root or not path.is_file():
            raise ValueError(f"Manifest asset is missing or escapes its directory: {filename}")
        if entry.get("sha256") != sha256(path):
            raise ValueError(f"Manifest asset SHA-256 mismatch: {filename}")
        if entry.get("llvm_major") != llvm_major:
            raise ValueError(f"Manifest asset LLVM major mismatch: {filename}")
        by_name[filename] = path

    schema = by_name.get(SCHEMA_NAME)
    descriptor = by_name.get(DESCRIPTOR_NAME)
    if schema is None or descriptor is None:
        raise ValueError("Manifest must include the canonical schema and descriptor assets")
    if protocol.get("schema_sha256") != sha256(schema):
        raise ValueError("Manifest schema SHA-256 does not match its schema asset")
    if protocol.get("descriptor_sha256") != sha256(descriptor):
        raise ValueError("Manifest descriptor SHA-256 does not match its descriptor asset")
    if protocol.get("semantic_contract") != SEMANTIC_CONTRACT:
        raise ValueError("Unsupported Protobuf semantic contract")
    verify_descriptor(schema, descriptor)


def write_release(asset_dir: Path, llvm_major: int, protoc: Path | None) -> None:
    asset_dir.mkdir(parents=True, exist_ok=True)
    schema = asset_dir / SCHEMA_NAME
    descriptor = asset_dir / DESCRIPTOR_NAME
    verify_descriptor(schema, descriptor, protoc)
    assets = []
    for path in sorted(asset_dir.iterdir()):
        if not path.is_file() or path.name == MANIFEST_NAME:
            continue
        platform, arch, kind = classify(path.name)
        assets.append(asset_entry(path, platform, arch, kind, llvm_major))
    output = asset_dir / MANIFEST_NAME
    write_if_changed(
        output,
        json.dumps(manifest_data(schema, descriptor, assets, llvm_major), indent=2, sort_keys=True)
        + "\n",
    )
    verify_manifest(output)


def write_local_build(
    output_dir: Path,
    tool: Path,
    descriptor: Path,
    platform: str,
    arch: str,
    llvm_major: int,
    protoc: Path | None,
    plugin: Path | None,
) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    schema_source = ROOT / "wire" / "clava_ast_wire.proto"
    schema = output_dir / SCHEMA_NAME
    descriptor_copy = output_dir / DESCRIPTOR_NAME
    tool_copy = output_dir / tool.name
    verify_descriptor(schema_source, descriptor, protoc)
    for source, destination in (
        (schema_source, schema),
        (descriptor, descriptor_copy),
        (tool, tool_copy),
    ):
        if not source.is_file():
            raise ValueError(f"Local manifest input is missing: {source}")
        if source.resolve() != destination.resolve():
            copy_if_changed(source, destination)

    plugin_copy = None
    if plugin is not None:
        if not plugin.is_file():
            raise ValueError(f"Local plugin input is missing: {plugin}")
        plugin_copy = output_dir / plugin.name
        if plugin.resolve() != plugin_copy.resolve():
            copy_if_changed(plugin, plugin_copy)

    verify_descriptor(schema, descriptor_copy, protoc)
    assets = [
        asset_entry(schema, "any", "any", "protocol", llvm_major),
        asset_entry(descriptor_copy, "any", "any", "protocol", llvm_major),
        asset_entry(tool_copy, platform, arch, "tool", llvm_major),
    ]
    if plugin_copy is not None:
        assets.append(asset_entry(plugin_copy, platform, arch, "plugin", llvm_major))
    output = output_dir / MANIFEST_NAME
    write_if_changed(
        output,
        json.dumps(manifest_data(schema, descriptor_copy, assets, llvm_major), indent=2, sort_keys=True)
        + "\n",
    )
    verify_manifest(output)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--asset-dir", type=Path, help="Write a release manifest for existing assets")
    modes.add_argument("--local-output-dir", type=Path, help="Copy local protocol/tool assets and write a manifest")
    modes.add_argument("--verify-manifest", type=Path, help="Verify hashes and compatibility metadata")
    modes.add_argument("--verify-protocol-artifacts", action="store_true", help="Verify a schema/descriptor pair with pinned protoc")
    parser.add_argument("--schema", type=Path)
    parser.add_argument("--llvm-major", type=int)
    parser.add_argument("--tool", type=Path)
    parser.add_argument("--plugin", type=Path)
    parser.add_argument("--descriptor", type=Path)
    parser.add_argument("--protoc", type=Path)
    parser.add_argument("--platform", choices=("linux", "macos", "windows"))
    parser.add_argument("--arch")
    args = parser.parse_args()

    if args.verify_manifest:
        verify_manifest(args.verify_manifest)
        return
    if args.verify_protocol_artifacts:
        if not args.schema or not args.descriptor or not args.protoc:
            parser.error("--verify-protocol-artifacts requires --schema, --descriptor, and --protoc")
        verify_descriptor(args.schema, args.descriptor, args.protoc)
        return
    if args.llvm_major is None:
        parser.error("--asset-dir and --local-output-dir require --llvm-major")
    if args.asset_dir:
        if not args.protoc:
            parser.error("--asset-dir requires --protoc to verify pinned descriptor provenance")
        write_release(args.asset_dir, args.llvm_major, args.protoc)
        return
    if not args.tool or not args.tool.is_file():
        parser.error("--local-output-dir requires an existing --tool file")
    if not args.descriptor or not args.descriptor.is_file():
        parser.error("--local-output-dir requires an existing --descriptor file")
    if not args.platform or not args.arch:
        parser.error("--local-output-dir requires --platform and --arch")
    if not args.protoc:
        parser.error("--local-output-dir requires --protoc to verify pinned descriptor provenance")
    write_local_build(
        args.local_output_dir,
        args.tool,
        args.descriptor,
        args.platform,
        args.arch,
        args.llvm_major,
        args.protoc,
        args.plugin,
    )


if __name__ == "__main__":
    main()
