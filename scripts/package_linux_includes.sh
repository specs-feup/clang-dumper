#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <x64|arm64> <output-zip>" >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/load_llvm_version.sh"
load_llvm_version "${ROOT_DIR}/llvm-version.env"

OUTPUT_ZIP="$2"
if [[ "${OUTPUT_ZIP}" != /* ]]; then
  OUTPUT_ZIP="${ROOT_DIR}/${OUTPUT_ZIP}"
fi

case "$1" in
  x64|arm64) ;;
  *)
    echo "Unknown Linux include package target: $1" >&2
    exit 1
    ;;
esac

python3 "${ROOT_DIR}/scripts/package_includes.py" \
  --platform linux \
  --staging "${ROOT_DIR}/.deps/linux-includes-$1" \
  --output "${OUTPUT_ZIP}" \
  -- "clang++-${LLVM_VERSION}" -E -x c++ - -v
