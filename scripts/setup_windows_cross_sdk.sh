#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOWNLOAD_DIR="${ROOT_DIR}/.deps/msys2-sdk-downloads"
HOST_TOOLS_DIR="${ROOT_DIR}/.deps/host-tools"

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

  local apt_dir="${ROOT_DIR}/.deps/apt-downloads"
  mkdir -p "${apt_dir}" "${HOST_TOOLS_DIR}/lld-18"
  (
    cd "${apt_dir}"
    apt-get download lld-18
  )
  dpkg-deb -x "${apt_dir}"/lld-18_*_amd64.deb "${HOST_TOOLS_DIR}/lld-18"
  ln -sf ../lld-18/usr/bin/ld.lld-18 "${HOST_TOOLS_DIR}/bin/ld.lld"
}

setup_clangarm64() {
  local root="${ROOT_DIR}/.deps/msys2-clangarm64-18"
  local base="https://repo.msys2.org/mingw/clangarm64"
  local packages=(
    mingw-w64-clang-aarch64-llvm-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-llvm-libs-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-clang-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-clang-libs-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-clang-tools-extra-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-compiler-rt-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-libc%2B%2B-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-libunwind-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-lld-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-headers-git-12.0.0.r264.g5c63f0a96-1-any.pkg.tar.zst
    mingw-w64-clang-aarch64-crt-git-12.0.0.r264.g5c63f0a96-1-any.pkg.tar.zst
    mingw-w64-clang-aarch64-winpthreads-git-12.0.0.r264.g5c63f0a96-1-any.pkg.tar.zst
    mingw-w64-clang-aarch64-zlib-ng-compat-2.3.3-2-any.pkg.tar.zst
    mingw-w64-clang-aarch64-zstd-1.5.7-2-any.pkg.tar.zst
  )
  for pkg in "${packages[@]}"; do
    fetch_extract "${root}" "${base}/${pkg}"
  done
}

setup_clang64() {
  local root="${ROOT_DIR}/.deps/msys2-clang64-18"
  local base="https://repo.msys2.org/mingw/clang64"
  local packages=(
    mingw-w64-clang-x86_64-llvm-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-llvm-libs-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-clang-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-clang-libs-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-clang-tools-extra-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-compiler-rt-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-libc%2B%2B-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-libunwind-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-lld-18.1.8-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-headers-git-12.0.0.r264.g5c63f0a96-1-any.pkg.tar.zst
    mingw-w64-clang-x86_64-crt-git-12.0.0.r264.g5c63f0a96-1-any.pkg.tar.zst
    mingw-w64-clang-x86_64-winpthreads-git-12.0.0.r264.g5c63f0a96-1-any.pkg.tar.zst
    mingw-w64-clang-x86_64-zlib-ng-compat-2.3.3-2-any.pkg.tar.zst
    mingw-w64-clang-x86_64-zstd-1.5.7-2-any.pkg.tar.zst
  )
  for pkg in "${packages[@]}"; do
    fetch_extract "${root}" "${base}/${pkg}"
  done
}

setup_lld
setup_clangarm64
setup_clang64
