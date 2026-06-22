#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CLANG_VERSION="${CLANG_VERSION:-18}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}"
SKIP_ENUM_GENERATION="${SKIP_ENUM_GENERATION:-OFF}"

if [[ -n "${CLANG_ENUMS_HOST_CLANG:-}" ]]; then
  if [[ "${CLANG_ENUMS_HOST_CLANG}" == */* ]]; then
    HOST_CLANGXX="${CLANG_ENUMS_HOST_CLANG}"
  else
    HOST_CLANGXX="$(command -v "${CLANG_ENUMS_HOST_CLANG}" || true)"
  fi
else
  HOST_CLANGXX="$(command -v "clang++-${CLANG_VERSION}" || command -v clang++ || true)"
fi

if [[ -z "${HOST_CLANGXX}" ]]; then
  echo "clang++ is required for enum preprocessing" >&2
  exit 1
fi

build_target() {
  local build_dir="$1"
  local cross_prefix="$2"
  local sdk_root="$3"

  cmake -S "${ROOT_DIR}" -B "${ROOT_DIR}/${build_dir}" -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/toolchain-mingw.cmake" \
    -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
    -DCROSS_PREFIX="${cross_prefix}" \
    -DMINGW_SYSROOT="${sdk_root}" \
    -DLLVM_WINDOWS_ROOT="${sdk_root}" \
    -DHOST_LLD_DIR="${ROOT_DIR}/.deps/host-tools/bin" \
    -DCLANG_VERSION="${CLANG_VERSION}" \
    -DSKIP_ENUM_GENERATION="${SKIP_ENUM_GENERATION}" \
    -DCLANG_ENUMS_HOST_CLANG="${HOST_CLANGXX}"

  cmake --build "${ROOT_DIR}/${build_dir}" --target tool -j"$(nproc)"
}

if [[ $# -eq 0 ]]; then
  set -- arm64 x86_64
fi

for target in "$@"; do
  case "${target}" in
    arm64)
      build_target build-win-arm64 aarch64-w64-mingw32 "${ROOT_DIR}/.deps/msys2-clangarm64-${CLANG_VERSION}/clangarm64"
      ;;
    x86_64)
      build_target build-win-x86_64 x86_64-w64-mingw32 "${ROOT_DIR}/.deps/msys2-clang64-${CLANG_VERSION}/clang64"
      ;;
    *)
      echo "Unknown Windows target: ${target}" >&2
      exit 1
      ;;
  esac
done
