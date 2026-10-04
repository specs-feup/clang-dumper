#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/load_llvm_version.sh"
load_llvm_version "${ROOT_DIR}/llvm-version.env"
source "${ROOT_DIR}/flatbuffers-version.env"

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

FLATBUFFERS_SOURCE_DIR="${ROOT_DIR}/.deps/flatbuffers-${FLATBUFFERS_COMMIT}"
FLATBUFFERS_HOST_BUILD_DIR="${ROOT_DIR}/.deps/flatbuffers-host-${FLATBUFFERS_COMMIT}"
FLATC_HOST="${FLATBUFFERS_HOST_BUILD_DIR}/flatc"

prepare_host_flatc() {
  if [[ ! -f "${FLATBUFFERS_SOURCE_DIR}/CMakeLists.txt" ]]; then
    mkdir -p "${ROOT_DIR}/.deps"
    git clone --filter=blob:none --no-checkout \
      https://github.com/google/flatbuffers.git "${FLATBUFFERS_SOURCE_DIR}"
  fi
  git -C "${FLATBUFFERS_SOURCE_DIR}" checkout --detach "${FLATBUFFERS_COMMIT}"

  local actual_commit
  actual_commit="$(git -C "${FLATBUFFERS_SOURCE_DIR}" rev-parse HEAD)"
  if [[ "${actual_commit}" != "${FLATBUFFERS_COMMIT}" ]]; then
    echo "FlatBuffers checkout ${actual_commit} does not match pinned ${FLATBUFFERS_COMMIT}" >&2
    exit 1
  fi

  cmake -S "${FLATBUFFERS_SOURCE_DIR}" -B "${FLATBUFFERS_HOST_BUILD_DIR}" \
    -DFLATBUFFERS_BUILD_TESTS=OFF \
    -DFLATBUFFERS_BUILD_FLATC=ON \
    -DFLATBUFFERS_BUILD_FLATHASH=OFF \
    -DFLATBUFFERS_INSTALL=OFF
  cmake --build "${FLATBUFFERS_HOST_BUILD_DIR}" --target flatc -j"$(nproc)"

  if [[ ! -x "${FLATC_HOST}" ]]; then
    echo "Pinned FlatBuffers build did not produce ${FLATC_HOST}" >&2
    exit 1
  fi
  local actual_version
  actual_version="$("${FLATC_HOST}" --version)"
  if [[ "${actual_version}" != "flatc version ${FLATBUFFERS_VERSION}" ]]; then
    echo "flatc '${actual_version}' does not match pinned ${FLATBUFFERS_VERSION}" >&2
    exit 1
  fi
}

prepare_host_flatc

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
    -DCLANG_ENUMS_HOST_CLANG="${HOST_CLANGXX}" \
    -DFETCHCONTENT_SOURCE_DIR_FLATBUFFERS="${FLATBUFFERS_SOURCE_DIR}" \
    -DAST_WIRE_FLATC_HOST="${FLATC_HOST}"

  cmake --build "${ROOT_DIR}/${build_dir}" --target tool verify_flatbuffers -j"$(nproc)"
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
