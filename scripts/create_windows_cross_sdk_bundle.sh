#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/load_llvm_version.sh"
load_llvm_version "${ROOT_DIR}/llvm-version.env"

DOWNLOAD_DIR="${ROOT_DIR}/.deps/msys2-sdk-downloads"
OUTPUT_DIR="${ROOT_DIR}/.deps/windows-sdk-release"
OUTPUT="${OUTPUT_DIR}/${WINDOWS_SDK_ASSET}"

llvm_packages=(llvm llvm-libs clang clang-libs clang-tools-extra compiler-rt 'libc++' libunwind lld)
mingw_packages=(headers-git crt-git winpthreads-git)
archives=()

for repo_triplet in 'clang64 x86_64' 'clangarm64 aarch64'; do
  set -- ${repo_triplet}
  triplet="$2"
  for package in "${llvm_packages[@]}"; do
    archives+=("mingw-w64-clang-${triplet}-${package}-${LLVM_RELEASE}-${MSYS2_LLVM_PACKAGE_RELEASE}-any.pkg.tar.zst")
  done
  for package in "${mingw_packages[@]}"; do
    archives+=("mingw-w64-clang-${triplet}-${package}-${MSYS2_MINGW_PACKAGE_RELEASE}-any.pkg.tar.zst")
  done
  archives+=(
    "mingw-w64-clang-${triplet}-zlib-ng-compat-${MSYS2_ZLIB_NG_RELEASE}-any.pkg.tar.zst"
    "mingw-w64-clang-${triplet}-zstd-${MSYS2_ZSTD_RELEASE}-any.pkg.tar.zst"
    "mingw-w64-clang-${triplet}-openmp-${MSYS2_OPENMP_RELEASE}-any.pkg.tar.zst"
  )
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
