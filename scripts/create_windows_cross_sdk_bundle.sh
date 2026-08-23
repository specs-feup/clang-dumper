#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/load_llvm_version.sh"
load_llvm_version "${ROOT_DIR}/llvm-version.env"
source "${ROOT_DIR}/scripts/windows_sdk_packages.sh"

DOWNLOAD_DIR="${ROOT_DIR}/.deps/msys2-sdk-downloads"
OUTPUT_DIR="${ROOT_DIR}/.deps/windows-sdk-release"
OUTPUT="${OUTPUT_DIR}/${WINDOWS_SDK_ASSET}"

archives=()

for repo_triplet in 'clang64 x86_64' 'clangarm64 aarch64'; do
  set -- ${repo_triplet}
  triplet="$2"
  mapfile -t triplet_archives < <(msys2_sdk_archives "${triplet}")
  archives+=("${triplet_archives[@]}")
done

for archive in "${archives[@]}"; do
  if [[ ! -f "${DOWNLOAD_DIR}/${archive}" ]]; then
    echo "Missing SDK archive: ${DOWNLOAD_DIR}/${archive}" >&2
    exit 1
  fi
done

mkdir -p "${OUTPUT_DIR}"
(
  cd "${DOWNLOAD_DIR}"
  printf '%s\n' "${archives[@]}" | sort | xargs sha256sum > SHA256SUMS
  tar --sort=name --mtime='@0' --owner=0 --group=0 --numeric-owner \
    -cf "${OUTPUT}" SHA256SUMS "${archives[@]}"
  rm SHA256SUMS
)

sha256sum "${OUTPUT}"
