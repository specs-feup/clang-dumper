#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/load_llvm_version.sh"
load_llvm_version "${ROOT_DIR}/llvm-version.env"

ASSET="${ROOT_DIR}/.deps/windows-sdk-release/${WINDOWS_SDK_ASSET}"
if [[ ! -f "${ASSET}" ]]; then
  "${ROOT_DIR}/scripts/create_windows_cross_sdk_bundle.sh"
fi

if ! gh release view "${WINDOWS_SDK_RELEASE_TAG}" --repo specs-feup/clang-dumper >/dev/null 2>&1; then
  gh release create "${WINDOWS_SDK_RELEASE_TAG}" \
    --repo specs-feup/clang-dumper \
    --title "Windows builds SDKs" \
    --notes "Immutable Windows cross-compilation SDK bundles used by CI." \
    --latest=false
fi

asset_digest="sha256:$(sha256sum "${ASSET}" | awk '{print $1}')"
published_digest="$(gh release view "${WINDOWS_SDK_RELEASE_TAG}" \
  --repo specs-feup/clang-dumper \
  --json assets \
  --jq ".assets[] | select(.name == \"${WINDOWS_SDK_ASSET}\") | .digest")"

if [[ -n "${published_digest}" ]]; then
  if [[ "${published_digest}" != "${asset_digest}" ]]; then
    echo "Release asset ${WINDOWS_SDK_ASSET} already exists with a different checksum" >&2
    exit 1
  fi
  exit 0
fi

gh release upload "${WINDOWS_SDK_RELEASE_TAG}" "${ASSET}" --repo specs-feup/clang-dumper
