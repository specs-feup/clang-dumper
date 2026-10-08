# Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The CMakeLists.txt has two main targets, `plugin` and `tool`; pass one or
both to `cmake --build build --target ...` to select what to build. A normal
build also refreshes the local release manifest and its verified schema,
descriptor, tool, and plugin assets in the build directory.

The target `tool` has been successfully built in Ubuntu and macOS. Windows
executables are cross-compiled from Linux.

## Stand-alone output

The stand-alone tool requires `-o` for structured output. It never writes the
machine protocol to stderr, because Clang diagnostics would make that stream
unparseable:

```sh
build/tool -c source.cpp -o source.ast -- -std=c++17
```

For large dumps, the tool can stream a Zstandard frame directly to the output
without buffering the AST in memory:

```sh
build/tool -c source.cpp -o source.ast.zst -ast-dump-compression=zstd -- -std=c++17
```

The structured output is the versioned protobuf stream defined by
`wire/clava_ast_wire.proto`. It starts with the eight-byte `CLAVAPB1` magic
and contains varint-delimited `Envelope` messages. The first envelope is a
Header, the last is an End, and the middle envelopes are non-empty Chunk
messages containing ordered Record entries. The native writer targets about
64 KiB per Chunk; an individual record is still capped at 64 MiB. Release
assets include the source schema and its generated descriptor; consumers
should verify both against the protocol entries in
`clang-dumper-release-manifest.json`.

The native `ProtoObjects.h` and `ProtoEncode.h` files are checked-in adapter
headers regenerated from protoc's descriptor during the build. `ProtoObjects`
holds only the temporary value for one record; it is never retained as a
second AST. The build compares regenerated headers with the checked-in files
and fails on schema drift. Schema and descriptor artifacts are checked against
the pinned native `protoc` on every build, including cached builds.

## Protocol maintenance

`wire/clava_ast_wire.proto` is the canonical, Java-independent schema. Recent
wire fields retain the links needed to emit out-of-line function and partial
specialization template parameters, lambda init-capture names and forms,
GCC-assembly inline/goto labels, member-pointer class/pointee types, unresolved
dependent construction list-initialization, and FriendDecl owner/target
references. Function parameter-list sizes keep the flattened parameter IDs
grouped. Generic `AttributeData` is limited to names from the closed
`AttributeKind` enum; the release manifest lists the pinned LLVM OpenMP
statement classes that intentionally share `StmtData`.

After editing the schema, regenerate the checked-in adapters and run the normal
build checks:

```sh
cmake --build build --target ast_wire_proto
cmake --build build --target proto_adapter_regenerate
cmake --build build --parallel
```

The protocol source hash is embedded in each stream header. Publish the
canonical `.proto`, its descriptor, and the platform binaries from the same
compatible native build. `clang-dumper-release-manifest.json` binds their
SHA-256 hashes to the LLVM major, native Protobuf/protoc versions, semantic
contract, and minimum Java compiler/runtime versions. Update the pinned tool
and consumer dependencies explicitly when that manifest requires newer Java
Protobuf support. CI publishes only tags containing `-rc` as prereleases.

## Native tests

Native CI runs CTest for the schema, metadata, and plugin stream checks, then
runs the full input corpus through `test/run_tests.py` with the built
`verify_protobuf` executable. That corpus check validates each emitted binary
stream; it does not compare the legacy text snapshots under `test/expected/`.
Those snapshots remain available to the harness's explicit text comparison
path and offline replay of older text captures. Binary captures are preserved
as `.pb` files and are not accepted by the text replay script.

## Dependencies

**Python3 is required to build this project**

```sh
source llvm-version.env

# Required for all targets
sudo apt install python3 python3-protobuf clang-${LLVM_VERSION} libclang-${LLVM_VERSION}-dev llvm-${LLVM_VERSION}-dev zlib1g-dev libxml2-dev

# Required for building the stand-alone tool
sudo apt install libpolly-${LLVM_VERSION}-dev libedit-dev libzstd-dev

# Required for Linux-hosted Windows cross builds
sudo apt install curl dpkg lld-${LLVM_VERSION} llvm-${LLVM_VERSION}-tools rsync tar zstd
```

## Windows Cross Builds

Windows builds are produced from Linux only. The Windows plugin is not built.

```sh
scripts/setup_windows_cross_sdk.sh
scripts/build_windows_cross.sh
scripts/package_includes.sh windows arm64 dist/clang-dumper-windows-arm64-includes.zip
scripts/package_includes.sh windows x86_64 dist/clang-dumper-windows-x86_64-includes.zip
```

The scripts build:

```text
build-win-arm64/tool.exe
build-win-x86_64/tool.exe
```

The produced executables are statically linked against LLVM/Clang and the
MinGW/LLVM runtime libraries. They should only import Windows system/UCRT DLLs.

The MSYS2 package archives used by cross builds are pinned in
`llvm-version.env` and retrieved from the
[Windows builds SDKs](https://github.com/specs-feup/clang-dumper/releases/tag/windows-build-sdks)
release. To add a new immutable SDK version, place its required archives in
`.deps/msys2-sdk-downloads/`, update the package-version settings and
`WINDOWS_SDK_ASSET` in `llvm-version.env`, then run:

```sh
scripts/create_windows_cross_sdk_bundle.sh
```

Copy the resulting SHA-256 into `WINDOWS_SDK_SHA256`, then publish with
`scripts/publish_windows_cross_sdk_bundle.sh`. Existing asset names are never
overwritten.

## Include Packages

Release builds also publish include packages for each supported OS/architecture.
The packages contain the minimal copied include roots plus an `entrypoints.txt`
file. Consumers should pass each path listed in `entrypoints.txt`, in order, as
an include root relative to the archive root.

OpenMP headers are included when available.

```sh
scripts/package_includes.sh linux x64 dist/clang-dumper-linux-x64-includes.zip
scripts/package_includes.sh linux arm64 dist/clang-dumper-linux-arm64-includes.zip

scripts/package_includes.sh macos x64 dist/clang-dumper-macos-x64-includes.zip
scripts/package_includes.sh macos arm64 dist/clang-dumper-macos-arm64-includes.zip

scripts/package_includes.sh windows arm64 dist/clang-dumper-windows-arm64-includes.zip
scripts/package_includes.sh windows x86_64 dist/clang-dumper-windows-x86_64-includes.zip
```

# Creating 'include' packages

Clava ships with pre-assembled stdlibc/c++ for several OS (Windows, Linux and macOS).
For non-Windows packages, choose a reference system, install Clang in the same version as the dumper, and check which include folders Clang uses for C++:

```
clang++ -E -x c++ - -v </dev/null
```

Then, starting with a new, empty folder, copy each include folder to this folder, following the order by which it is listed, and creating folders that also follow this order:

```
01-libcxx
02-libc
...
```

Finally, zip this folder. The copied include folders should appear in the root of the zip
