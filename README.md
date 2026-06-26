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
scripts/package_windows_includes.sh arm64 dist/clang-dumper-windows-arm64-includes.zip
scripts/package_windows_includes.sh x86_64 dist/clang-dumper-windows-x86_64-includes.zip
```

The scripts build:

```text
build-win-arm64/tool.exe
build-win-x86_64/tool.exe
```

The produced executables are statically linked against LLVM/Clang and the
MinGW/LLVM runtime libraries. They should only import Windows system/UCRT DLLs.

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
