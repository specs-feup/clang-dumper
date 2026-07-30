#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/load_llvm_version.sh"
load_llvm_version "${ROOT_DIR}/llvm-version.env"

DOWNLOAD_DIR="${ROOT_DIR}/.deps/msys2-sdk-downloads"
SDK_BUNDLE_DIR="${ROOT_DIR}/.deps/windows-sdk-bundles"
HOST_TOOLS_DIR="${ROOT_DIR}/.deps/host-tools"

mkdir -p "${DOWNLOAD_DIR}" "${SDK_BUNDLE_DIR}" "${HOST_TOOLS_DIR}/bin"

download_sdk_bundle() {
  local bundle="${SDK_BUNDLE_DIR}/${WINDOWS_SDK_ASSET}"
  local url="https://github.com/specs-feup/clang-dumper/releases/download/${WINDOWS_SDK_RELEASE_TAG}/${WINDOWS_SDK_ASSET}"

  if [[ -z "${WINDOWS_SDK_SHA256}" ]]; then
    echo "WINDOWS_SDK_SHA256 is required in llvm-version.env" >&2
    exit 1
  fi

  if [[ ! -f "${bundle}" ]]; then
    curl -fL --retry 3 "${url}" -o "${bundle}"
  fi
  echo "${WINDOWS_SDK_SHA256}  ${bundle}" | sha256sum --check --status
  tar -xf "${bundle}" -C "${DOWNLOAD_DIR}"
  (
    cd "${DOWNLOAD_DIR}"
    sha256sum --check SHA256SUMS
  )
}

fetch_extract() {
  local root="$1"
  local url="$2"
  local file="${DOWNLOAD_DIR}/$(basename "${url}" | sed 's/%2B/+/g')"
  mkdir -p "${root}"
  if [[ ! -f "${file}" ]]; then
    download_sdk_bundle
  fi
  if [[ ! -f "${file}" ]]; then
    echo "Windows SDK bundle did not provide $(basename "${file}")" >&2
    exit 1
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
  fetch_extract "${root}" "${base}/mingw-w64-clang-${triplet}-openmp-${MSYS2_OPENMP_RELEASE}-any.pkg.tar.zst"
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
