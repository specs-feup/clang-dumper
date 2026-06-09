#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <arm64|x86_64> <output-zip>" >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TARGET="$1"
OUTPUT_ZIP="$2"

case "${TARGET}" in
  arm64)
    SDK_ROOT="${ROOT_DIR}/.deps/msys2-clangarm64-18/clangarm64"
    ;;
  x86_64)
    SDK_ROOT="${ROOT_DIR}/.deps/msys2-clang64-18/clang64"
    ;;
  *)
    echo "Unknown Windows include package target: ${TARGET}" >&2
    exit 1
    ;;
esac

if [[ ! -d "${SDK_ROOT}" ]]; then
  echo "SDK root not found: ${SDK_ROOT}" >&2
  exit 1
fi

STAGING_DIR="${ROOT_DIR}/.deps/windows-includes-${TARGET}"
rm -rf "${STAGING_DIR}"
mkdir -p \
  "${STAGING_DIR}/01-libcxx" \
  "${STAGING_DIR}/02-clang" \
  "${STAGING_DIR}/03-mingw"

cp -a "${SDK_ROOT}/include/c++/v1/." "${STAGING_DIR}/01-libcxx/"
cp -a "${SDK_ROOT}/lib/clang/18/include/." "${STAGING_DIR}/02-clang/"

rsync -a \
  --exclude 'c++' \
  --exclude 'clang' \
  --exclude 'clang-c' \
  --exclude 'llvm' \
  --exclude 'llvm-c' \
  "${SDK_ROOT}/include/." \
  "${STAGING_DIR}/03-mingw/"

mkdir -p "$(dirname "${OUTPUT_ZIP}")"
rm -f "${OUTPUT_ZIP}"
(
  cd "${STAGING_DIR}"
  python3 -m zipfile -c "${OUTPUT_ZIP}" .
)
