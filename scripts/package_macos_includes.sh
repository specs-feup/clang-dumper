#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <x64|arm64> <output-zip>" >&2
  exit 1
fi

: "${LLVM_PREFIX:?LLVM_PREFIX is required}"
: "${SDKROOT:?SDKROOT is required}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT_ZIP="$2"
if [[ "${OUTPUT_ZIP}" != /* ]]; then
  OUTPUT_ZIP="${ROOT_DIR}/${OUTPUT_ZIP}"
fi

case "$1" in
  x64|arm64) ;;
  *)
    echo "Unknown macOS include package target: $1" >&2
    exit 1
    ;;
esac

EXTRA_INCLUDE_ARGS=()
add_extra_include_dir() {
  local include_dir="$1"
  if [[ -d "${include_dir}" ]]; then
    EXTRA_INCLUDE_ARGS+=(--extra-include-dir "${include_dir}")
  fi
}

if [[ -n "${LIBOMP_PREFIX:-}" ]]; then
  add_extra_include_dir "${LIBOMP_PREFIX}/include"
elif command -v brew >/dev/null 2>&1 && brew --prefix libomp >/dev/null 2>&1; then
  add_extra_include_dir "$(brew --prefix libomp)/include"
fi

python3 "${ROOT_DIR}/scripts/package_includes.py" \
  --platform macos \
  --staging "${ROOT_DIR}/.deps/macos-includes-$1" \
  --output "${OUTPUT_ZIP}" \
  "${EXTRA_INCLUDE_ARGS[@]}" \
  -- "${LLVM_PREFIX}/bin/clang++" -isysroot "${SDKROOT}" -E -x c++ - -v
