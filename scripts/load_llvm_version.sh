#!/usr/bin/env bash

load_llvm_version() {
  local manifest="$1"
  if [[ ! -f "${manifest}" ]]; then
    echo "LLVM version manifest not found: ${manifest}" >&2
    return 1
  fi

  set -a
  # shellcheck source=/dev/null
  source "${manifest}"
  set +a

  : "${LLVM_VERSION:?LLVM_VERSION is required in ${manifest}}"
  : "${LLVM_RELEASE:?LLVM_RELEASE is required in ${manifest}}"
  : "${MSYS2_LLVM_PACKAGE_RELEASE:?MSYS2_LLVM_PACKAGE_RELEASE is required in ${manifest}}"
  : "${MSYS2_MINGW_PACKAGE_RELEASE:?MSYS2_MINGW_PACKAGE_RELEASE is required in ${manifest}}"
  : "${MSYS2_ZLIB_NG_RELEASE:?MSYS2_ZLIB_NG_RELEASE is required in ${manifest}}"
  : "${MSYS2_ZSTD_RELEASE:?MSYS2_ZSTD_RELEASE is required in ${manifest}}"
  : "${WINDOWS_SDK_RELEASE_TAG:?WINDOWS_SDK_RELEASE_TAG is required in ${manifest}}"
  : "${WINDOWS_SDK_ASSET:?WINDOWS_SDK_ASSET is required in ${manifest}}"

  if [[ -n "${CLANG_VERSION:-}" && "${CLANG_VERSION}" != "${LLVM_VERSION}" ]]; then
    echo "CLANG_VERSION=${CLANG_VERSION} conflicts with LLVM_VERSION=${LLVM_VERSION} from ${manifest}" >&2
    return 1
  fi
  CLANG_VERSION="${LLVM_VERSION}"
  export CLANG_VERSION
}
