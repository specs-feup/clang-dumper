#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOWNLOAD_DIR="${ROOT_DIR}/.deps/msys2-sdk-downloads"
HOST_TOOLS_DIR="${ROOT_DIR}/.deps/host-tools"
CLANG_VERSION="${CLANG_VERSION:-${LLVM_VERSION:-18}}"
LLVM_RELEASE="${LLVM_RELEASE:-18.1.8}"
MSYS2_LLVM_PACKAGE_RELEASE="${MSYS2_LLVM_PACKAGE_RELEASE:-2}"
MSYS2_MINGW_PACKAGE_RELEASE="${MSYS2_MINGW_PACKAGE_RELEASE:-12.0.0.r747.g1a99f8514-1}"
MSYS2_ZLIB_NG_RELEASE="${MSYS2_ZLIB_NG_RELEASE:-2.3.3-2}"
MSYS2_ZSTD_RELEASE="${MSYS2_ZSTD_RELEASE:-1.5.7-2}"

mkdir -p "${DOWNLOAD_DIR}" "${HOST_TOOLS_DIR}/bin"

fetch_extract() {
  local root="$1"
  local url="$2"
  local file="${DOWNLOAD_DIR}/$(basename "${url}" | sed 's/%2B/+/g')"
  mkdir -p "${root}"
  if [[ ! -f "${file}" ]]; then
    curl -fL "${url}" -o "${file}"
  fi
  tar -I zstd -xf "${file}" -C "${root}"
}

setup_lld() {
  if command -v ld.lld >/dev/null 2>&1; then
    ln -sf "$(command -v ld.lld)" "${HOST_TOOLS_DIR}/bin/ld.lld"
    return
  fi

  if command -v "ld.lld-${CLANG_VERSION}" >/dev/null 2>&1; then
    ln -sf "$(command -v "ld.lld-${CLANG_VERSION}")" "${HOST_TOOLS_DIR}/bin/ld.lld"
    return
  fi

  local apt_dir="${ROOT_DIR}/.deps/apt-downloads"
  mkdir -p "${apt_dir}" "${HOST_TOOLS_DIR}/lld-${CLANG_VERSION}"
  (
    cd "${apt_dir}"
    apt-get download "lld-${CLANG_VERSION}"
  )
  dpkg-deb -x "${apt_dir}"/lld-"${CLANG_VERSION}"_*_amd64.deb "${HOST_TOOLS_DIR}/lld-${CLANG_VERSION}"
  ln -sf "../lld-${CLANG_VERSION}/usr/bin/ld.lld-${CLANG_VERSION}" "${HOST_TOOLS_DIR}/bin/ld.lld"
}

setup_msys2_sdk() {
  local repo="$1"
  local triplet="$2"
  local root="${ROOT_DIR}/.deps/msys2-${repo}-${CLANG_VERSION}"
  local base="https://repo.msys2.org/mingw/${repo}"
  local llvm_packages=(
    llvm
    llvm-libs
    clang
    clang-libs
    clang-tools-extra
    compiler-rt
    libc%2B%2B
    libunwind
    lld
  )
  local mingw_packages=(
    headers-git
    crt-git
    winpthreads-git
  )

  for package in "${llvm_packages[@]}"; do
    fetch_extract "${root}" "${base}/mingw-w64-clang-${triplet}-${package}-${LLVM_RELEASE}-${MSYS2_LLVM_PACKAGE_RELEASE}-any.pkg.tar.zst"
  done
  for package in "${mingw_packages[@]}"; do
    fetch_extract "${root}" "${base}/mingw-w64-clang-${triplet}-${package}-${MSYS2_MINGW_PACKAGE_RELEASE}-any.pkg.tar.zst"
  done
  fetch_extract "${root}" "${base}/mingw-w64-clang-${triplet}-zlib-ng-compat-${MSYS2_ZLIB_NG_RELEASE}-any.pkg.tar.zst"
  fetch_extract "${root}" "${base}/mingw-w64-clang-${triplet}-zstd-${MSYS2_ZSTD_RELEASE}-any.pkg.tar.zst"
}

if [[ $# -eq 0 ]]; then
  set -- arm64 x86_64
fi

setup_lld

for target in "$@"; do
  case "${target}" in
    arm64)
      setup_msys2_sdk clangarm64 aarch64
      ;;
    x86_64)
      setup_msys2_sdk clang64 x86_64
      ;;
    *)
      echo "Unknown Windows SDK target: ${target}" >&2
      exit 1
      ;;
  esac
done
