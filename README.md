# Building

```sh
mkdir build
cd build
cmake ..
make
```

The CMakeLists.txt has two targets, `plugin` and `tool`, use `<MAKE_CMD> <target>` to build a specific target.

The target `tool` has been successfully built in Ubuntu and Windows (MinGW), make sure the executable clang-<VERSION> is in the path, as well as the path `/usr/lib/llvm-<VERSION>` (`C:/usr/...` in Windows).

## Dependencies

**Python3 is required to build this project**

```sh
# Required for all targets
sudo apt install clang-18 libclang-18-dev llvm-18-dev zlib1g-dev

# Required for building the stand-alone tool
sudo apt install libpolly-18-dev libedit-dev libzstd-dev
```

# Creating 'include' packages

Clava ships with pre-assembled stdlibc/c++ for several OS (Windows, Linux and MacOS).

To build those include packages, chose a reference system, install Clang in the same version as the dumper, and check which include folders Clang uses for C++:

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