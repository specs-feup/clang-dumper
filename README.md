# Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target tool plugin verify_flatbuffers --parallel
```

The `tool`, `plugin`, and `verify_flatbuffers` targets are available. The
FlatBuffers compiler and runtime are fetched from the commit pinned in
`flatbuffers-version.env`; CMake verifies their version before building.

The target `tool` has been successfully built in Ubuntu and macOS. Windows
executables are cross-compiled from Linux.

## Stand-alone output

The stand-alone tool always writes the eager FlatBuffers v2 protocol to the
required `-o` file. Clang diagnostics remain on stderr:

```sh
build/tool -c source.cpp -o source.clv2 -- -std=c++17
```

The plugin writes the same protocol when loaded with
`-Xclang -plugin-arg-DumpAst -Xclang -output=source.clv2`. Legacy text and
compression selectors are rejected. Validate a completed stream with:

```sh
build/verify_flatbuffers source.clv2
```

## Dependencies

**Python3 is required to build this project**

```sh
source llvm-version.env

# Required for all targets
sudo apt install python3 clang-${LLVM_VERSION} libclang-${LLVM_VERSION}-dev llvm-${LLVM_VERSION}-dev zlib1g-dev libxml2-dev

# Required for the stand-alone tool's static LLVM/Clang link
sudo apt install libpolly-${LLVM_VERSION}-dev libedit-dev libzstd-dev

# Required for Linux-hosted Windows cross builds
sudo apt install curl dpkg lld-${LLVM_VERSION} llvm-${LLVM_VERSION}-tools rsync tar zstd
```

Release builds include `clang-dumper-release-manifest.json` and the complete
`clang-dumper-wire-schema-v2.zip` schema bundle. See
[`wire/README.md`](wire/README.md) for the field/node workflow, generated-output
drift check, and schema hash contract.

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
