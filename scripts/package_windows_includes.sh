#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <arm64|x86_64> <output-zip>" >&2
  exit 1
fi

: "${LLVM_VERSION:?LLVM_VERSION is required}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_ZIP="$2"
if [[ "${OUTPUT_ZIP}" != /* ]]; then
  OUTPUT_ZIP="${ROOT_DIR}/${OUTPUT_ZIP}"
fi

case "$1" in
  arm64)
    SDK_ROOT="${ROOT_DIR}/.deps/msys2-clangarm64-${LLVM_VERSION}/clangarm64"
    TRIPLE="aarch64-w64-mingw32"
    ;;
  x86_64)
    SDK_ROOT="${ROOT_DIR}/.deps/msys2-clang64-${LLVM_VERSION}/clang64"
    TRIPLE="x86_64-w64-mingw32"
    ;;
  *)
    echo "Unknown Windows include package target: $1" >&2
    exit 1
    ;;
esac

RESOURCE_DIR="$(find "${SDK_ROOT}/lib/clang" -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)"

python3 "${ROOT_DIR}/scripts/package_includes.py" \
  --platform windows \
  --staging "${ROOT_DIR}/.deps/windows-includes-$1" \
  --output "${OUTPUT_ZIP}" \
  -- "clang++-${LLVM_VERSION}" --target="${TRIPLE}" --sysroot="${SDK_ROOT}" -stdlib=libc++ \
  -resource-dir "${RESOURCE_DIR}" -E -x c++ - -v
