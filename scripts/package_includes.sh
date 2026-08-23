#!/usr/bin/env bash
# Packages the minimal include roots for one platform/architecture into a zip.
#
# Usage: package_includes.sh <platform> <arch> <output-zip>
#   platform: linux | macos | windows
#   arch:     x64 | arm64 (linux, macos); arm64 | x86_64 (windows)
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 <linux|macos|windows> <arch> <output-zip>" >&2
  exit 1
fi

PLATFORM="$1"
ARCH="$2"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/load_llvm_version.sh"
load_llvm_version "${ROOT_DIR}/llvm-version.env"

OUTPUT_ZIP="$3"
if [[ "${OUTPUT_ZIP}" != /* ]]; then
  OUTPUT_ZIP="${ROOT_DIR}/${OUTPUT_ZIP}"
fi

EXTRA_INCLUDE_ARGS=()
add_extra_include_dir() {
  local include_dir="$1"
  if [[ -d "${include_dir}" ]]; then
    EXTRA_INCLUDE_ARGS+=(--extra-include-dir "${include_dir}")
  fi
}

case "${PLATFORM}/${ARCH}" in
  linux/x64|linux/arm64)
    STAGING=".deps/linux-includes-${ARCH}"

    while IFS= read -r include_dir; do
      add_extra_include_dir "${include_dir}"
    done < <(find "/usr/lib/llvm-${LLVM_VERSION}" -path '*/include/omp.h' -printf '%h\n' 2>/dev/null | sort -u)

    CLANG_CMD=("clang++-${LLVM_VERSION}")
    ;;
  macos/x64|macos/arm64)
    : "${LLVM_PREFIX:?LLVM_PREFIX is required}"
    : "${SDKROOT:?SDKROOT is required}"
    STAGING=".deps/macos-includes-${ARCH}"

    if [[ -n "${LIBOMP_PREFIX:-}" ]]; then
      add_extra_include_dir "${LIBOMP_PREFIX}/include"
    elif command -v brew >/dev/null 2>&1 && brew --prefix libomp >/dev/null 2>&1; then
      add_extra_include_dir "$(brew --prefix libomp)/include"
    fi

    CLANG_CMD=("${LLVM_PREFIX}/bin/clang++" "-isysroot" "${SDKROOT}")
    ;;
  windows/arm64)
    SDK_ROOT="${ROOT_DIR}/.deps/msys2-clangarm64-${LLVM_VERSION}/clangarm64"
    TRIPLE="aarch64-w64-mingw32"
    STAGING=".deps/windows-includes-${ARCH}"

    RESOURCE_DIR="$(find "${SDK_ROOT}/lib/clang" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)"
    CLANG_CMD=("clang++-${LLVM_VERSION}" "--target=${TRIPLE}" "--sysroot=${SDK_ROOT}" "-stdlib=libc++" "-resource-dir" "${RESOURCE_DIR}")
    ;;
  windows/x86_64)
    SDK_ROOT="${ROOT_DIR}/.deps/msys2-clang64-${LLVM_VERSION}/clang64"
    TRIPLE="x86_64-w64-mingw32"
    STAGING=".deps/windows-includes-${ARCH}"

    RESOURCE_DIR="$(find "${SDK_ROOT}/lib/clang" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)"
    CLANG_CMD=("clang++-${LLVM_VERSION}" "--target=${TRIPLE}" "--sysroot=${SDK_ROOT}" "-stdlib=libc++" "-resource-dir" "${RESOURCE_DIR}")
    ;;
  *)
    echo "Unknown include package target: ${PLATFORM} ${ARCH}" >&2
    exit 1
    ;;
esac

python3 "${ROOT_DIR}/scripts/package_includes.py" \
  --platform "${PLATFORM}" \
  --staging "${ROOT_DIR}/${STAGING}" \
  --output "${OUTPUT_ZIP}" \
  "${EXTRA_INCLUDE_ARGS[@]}" \
  -- "${CLANG_CMD[@]}" -E -x c++ - -v
