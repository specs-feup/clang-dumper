# Building

```sh
mkdir build
cd build
cmake ..
make
```

The CMakeLists.txt has two targets, `plugin` and `tool`, use `<MAKE_CMD> <target>` to build a specific target.

The target `tool` has been successfully built in Ubuntu and macOS. Windows
executables are cross-compiled from Linux.

## Dependencies

**Python3 is required to build this project**

```sh
# Required for all targets
sudo apt install clang-18 libclang-18-dev llvm-18-dev zlib1g-dev

# Required for building the stand-alone tool
sudo apt install libpolly-18-dev libedit-dev libzstd-dev
```

## Windows Cross Builds

Windows builds are produced from Linux only. The Windows plugin is not built.

```sh
scripts/setup_windows_cross_sdk.sh
scripts/build_windows_cross.sh
```

The scripts build:

```text
build-win-arm64/tool.exe
build-win-x86_64/tool.exe
```

To stage release executables:

```sh
mkdir -p dist
cp build-win-arm64/tool.exe dist/clang-dumper-windows-arm64.exe
cp build-win-x86_64/tool.exe dist/clang-dumper-windows-x86_64.exe
```

The produced executables are statically linked against LLVM/Clang and the
MinGW/LLVM runtime libraries. They should only import Windows system/UCRT DLLs.
