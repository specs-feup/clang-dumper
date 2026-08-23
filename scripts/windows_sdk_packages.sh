#!/usr/bin/env bash
# Shared definition of the MSYS2 packages that make up the Windows cross SDK.
# Sourced by setup_windows_cross_sdk.sh and create_windows_cross_sdk_bundle.sh;
# requires the version variables loaded from llvm-version.env.

MSYS2_LLVM_PACKAGES=(llvm llvm-libs clang clang-libs clang-tools-extra compiler-rt libc++ libunwind lld)
MSYS2_MINGW_PACKAGES=(headers-git crt-git winpthreads-git)

# Prints the archive file names for one triplet (x86_64 or aarch64), in the
# order used when assembling SDK bundles.
msys2_sdk_archives() {
  local triplet="$1"
  local package

  for package in "${MSYS2_LLVM_PACKAGES[@]}"; do
    printf 'mingw-w64-clang-%s-%s-%s-%s-any.pkg.tar.zst\n' \
      "${triplet}" "${package}" "${LLVM_RELEASE}" "${MSYS2_LLVM_PACKAGE_RELEASE}"
  done
  for package in "${MSYS2_MINGW_PACKAGES[@]}"; do
    printf 'mingw-w64-clang-%s-%s-%s-any.pkg.tar.zst\n' \
      "${triplet}" "${package}" "${MSYS2_MINGW_PACKAGE_RELEASE}"
  done
  printf 'mingw-w64-clang-%s-zlib-ng-compat-%s-any.pkg.tar.zst\n' \
    "${triplet}" "${MSYS2_ZLIB_NG_RELEASE}"
  printf 'mingw-w64-clang-%s-zstd-%s-any.pkg.tar.zst\n' \
    "${triplet}" "${MSYS2_ZSTD_RELEASE}"
  printf 'mingw-w64-clang-%s-openmp-%s-any.pkg.tar.zst\n' \
    "${triplet}" "${MSYS2_OPENMP_RELEASE}"
}
