# Updating LLVM/Clang

This document is the field manual for bumping the pinned LLVM/Clang version.
It covers the version pins, the source API adaptations, the Windows SDK
bundle, and the golden baseline refresh — including how to get baselines
from CI and iterate locally without re-running CI.

The 18.1.8 → 19.1.7 bump (PR #55) is the worked example for everything below.

## 1. Where the version lives

`llvm-version.env` is the single source of truth. Do not hardcode version
digits anywhere else:

```sh
LLVM_VERSION=19                     # major; drives compilers, paths, packages
LLVM_RELEASE=19.1.7                 # full version
MSYS2_LLVM_PACKAGE_RELEASE=1        # msys2 package release suffix (-1, -2, ...)
MSYS2_MINGW_PACKAGE_RELEASE=12.0.0.r747.g1a99f8514-1
MSYS2_OPENMP_PACKAGE=llvm-openmp    # msys2 package name (renames happen!)
MSYS2_OPENMP_RELEASE=19.1.7-1
MSYS2_ZLIB_NG_RELEASE=2.3.3-2
MSYS2_ZSTD_RELEASE=1.5.7-2
WINDOWS_SDK_RELEASE_TAG=windows-build-sdks
# WINDOWS_SDK_ASSET is derived by default as
# msys2-clang-sdk-llvm-${LLVM_RELEASE}.tar (scripts/load_llvm_version.sh);
# set it in the manifest only to override the name.
WINDOWS_SDK_SHA256=<sha256 of the SDK bundle>
```

Consumers:

- `cmake/read_llvm_version.cmake` → `CMakeLists.txt` (compilers, paths,
  llvm-config, and the major-version guard that fails the configure step when
  the found LLVM doesn't match the pin).
- `scripts/load_llvm_version.sh` → every shell script under `scripts/`. All
  manifest variables must be listed in its required-variables block; a missing
  pin fails loudly instead of silently using a default.
- `.github/workflows/build.yml` → passes the values between jobs via job
  outputs. All apt/homebrew installs are `${LLVM_VERSION}`-driven.

**msys2 pin lookups** (do this manually at every bump):

- Package archive URLs follow
  `https://repo.msys2.org/mingw/<repo>/mingw-w64-clang-<triplet>-<package>-<version>-any.pkg.tar.zst`
  with `<repo>` ∈ {`clang64` (x86_64), `clangarm64` (aarch64)}. HEAD-check each
  generated URL (`scripts/windows_sdk_packages.sh` builds the list).
- Beware **package renames**: msys2 renamed `openmp` → `llvm-openmp` in the
  19.x toolchain era; the old name's last release ever was 18.1.2-1, so a
  version search "finds nothing above 18" and the pin silently stays behind.
  When a package appears frozen at an old version, check `packages.msys2.org`
  for a rename/`replaces` before assuming the pin is fine.
- `MSYS2_LLVM_PACKAGE_RELEASE` is the msys2 `-1`/`-2` package suffix for
  `LLVM_RELEASE`; check the repo index for the exact suffix.

**Host toolchain drift**: distro packages lag minors (Ubuntu 24.04's `llvm-19`
is 19.1.1 while the SDK/homebrew pin 19.1.7). Builds link against whatever the
runner has; baselines must be generated from the same runners CI uses. Also
note `find_package(LLVM <major>)` is NOT usable in CMake — LLVM's package
config rejects a major-only requested version — hence the explicit
`LLVM_PACKAGE_VERSION` major check in `CMakeLists.txt`.

## 2. Bump procedure

1. **Pins**: update `llvm-version.env` (see §1). Add any new variable to
   `scripts/load_llvm_version.sh`'s required list and to
   `scripts/windows_sdk_packages.sh` if the archive list changed.
2. **Source API adaptations**: fix `src/` for the new Clang API. Put
   version-conditional code in **one** place,
   `src/Clang/ClangVersion.h`, instead of scattering `#if`s across visitors.
   The 19 bump touched 5 files (`ChildrenVisitorDecls.cpp`,
   `ClavaDataDumperDecls.cpp/.h`, `ClangAst.h/.cpp`) — the pattern a shim
   header centralizes.
3. **Build**: local or CI. The CMake major guard catches wrong-toolchain
   configurations (stale build cache, stray `LLVM_DIR`).
4. **Windows SDK**: rebuild and republish the bundle (see §7), update
   `WINDOWS_SDK_SHA256` in the same commit.
5. **Golden baselines**: refresh from CI (see §4) — never hand-carry
   baselines from a local container; the CI environment (header versions,
   libstdc++) is the golden source.
6. **PR body**: document every observable output-format change (see §6 of
   `git log` history for the 19 bump's structure: spelling changes with
   examples, structural record changes, header-position noise, and downstream
   impact). Downstream consumers (Clava) read this to plan their updates.

## 3. Golden baselines: layout and precedence

```
test/expected/                         # shared baseline (canonical)
test/expected-platforms/<platform>/    # overrides per CI target
```

`test/run_tests.py` resolves the expected file for each test in order:

1. `expected-platforms/<exact platform>/` (e.g. `macos-x64`),
2. `expected-platforms/<os family>/` (OS prefix: `macos`, `windows`),
3. `test/expected/` (shared).

**The platform dirs are a dedup mechanism.** The OS-family dir holds baselines
that are identical across the architectures of that OS — `macos/` contains the
files where `macos-arm64` and `macos-x64` produce the exact same normalized
output, and `windows/` serves both `windows-x86_64` and `windows-arm64` (their
outputs are fully identical after normalization). Per-arch dirs hold only the
divergences. `linux/` currently doesn't exist because linux-x64, linux-arm64
and the shared baseline all agree.

**Precedence is by design**: a platform file always wins over the shared
baseline. That means a **stale** platform file (e.g. left over from a previous
LLVM version) silently shadows a freshly refreshed shared baseline and fails
the run. After a bump, every platform dir must be either regenerated or
pruned — never left as-is.

## 4. Golden refresh from CI

CI uploads, on every run and regardless of pass/fail:

- `raw-test-outputs-<platform>` — `test/raw-outputs/<platform>/{tool,plugin}/`
  containing one `<test>.stderr` per executed test plus `_manifest.json`
  (`baseline_platform`, `mode`, `enabled_features`, `inputs_dir`,
  `extra_clang_args`, `system_header_threshold`).
- `failed-test-outputs-<platform>` — normalized outputs of failing tests, in
  the same format as the expected files (see §5).

Artifacts are retained for **24 hours**. If you miss the window, re-run CI
(re-running on the branch is cheap and re-uploads everything).

### Refresh loop

```sh
# Download one platform's raw outputs (zip contains test/raw-outputs/<plat>/...)
gh api repos/<owner>/<repo>/actions/runs/<run-id>/artifacts --jq \
  '.artifacts[] | select(.name=="raw-test-outputs-macos-arm64") | .id'
gh api repos/<owner>/<repo>/actions/artifacts/<id>/zip > raw.zip
unzip -q raw.zip -d raw-macos-arm64

# Regenerate the platform baselines: writes a file only when the output
# differs from the shared baseline; --prune empties stale files first.
python3 test/replay_outputs.py \
  --raw-output-dir raw-macos-arm64/tool \
  --baseline-platform macos-arm64 \
  --write-platform-baselines macos-arm64 \
  --prune-platform-baselines

# Dedup pass: move files identical across arches into the OS-family dir.
# (replay writes per-arch dirs; family dedup is a manual step)
mkdir -p test/expected-platforms/macos
# for each identical <name>.expected in macos-arm64/ and macos-x64/:
#   copy once to test/expected-platforms/macos/, delete both copies

# Verify: check-mode replay of EVERY capture (tool AND plugin), every platform
python3 test/replay_outputs.py --raw-output-dir raw-macos-arm64/tool \
  --baseline-platform macos-arm64    # expect "N passed, 0 failed"
python3 test/replay_outputs.py --raw-output-dir raw-macos-arm64/plugin \
  --baseline-platform macos-arm64
# ... repeat for every platform/mode in the run
```

Notes:

- `--prune-platform-baselines` deletes **all** existing `*.expected` in the
  platform dir and rewrites only what the capture differs on. Tests that did
  not run in that capture (e.g. CUDA — no CI runner enables it) lose their
  platform file and fall back to the shared baseline. That is fine while they
  are skipped everywhere; re-capture if a runner gains a feature.
- The shared baseline itself is regenerated by replaying the canonical
  platform capture (currently linux-x64 tool) and rewriting
  `test/expected/<test>.expected`; replay only writes platform dirs, so use
  its `--failure-output-dir` outputs or generate the shared files from the
  normalized capture directly.
- Always finish with the check-mode replay of all captures. All-green is the
  definition of a valid refresh.

## 5. Interpreting CI failure archives

When a test job fails, `build.yml` uploads `failed-test-outputs-<platform>`
(`test/failures/<platform>/{tool,plugin}/<test>.expected`). These are the
**normalized outputs of the failing runs**, in the same shape as the expected
files — diff them 1:1 against the baseline the run reported.

The test log tells you which resolution layer was compared:

```
Expected file: .../test/expected-platforms/macos/atom.cpp.expected (macos baseline)
```

The suffix in parentheses is the resolution layer: `(macos baseline)` = family
dir, `(linux-x64 baseline)` = exact dir, `(shared baseline)` = `test/expected/`.
If a failure reports a family baseline you thought was deduplicated away, or a
stale dir you meant to delete, the file is still there and still wins.

`raw-test-outputs-<platform>` is the deeper tool: it contains the raw,
un-normalized stderr of every test, so you can re-run normalization offline,
diff raw vs normalized, and replay against any baseline version.

## 6. Local replay iteration (for normalization work)

Once you have a raw-output directory (from CI, or captured locally with
`run_tests.py --raw-output-dir`), you can iterate on `run_tests.py`
normalization changes entirely offline:

```sh
# Compare the captured raw output against the committed baselines
python3 test/replay_outputs.py \
  --raw-output-dir /tmp/raw-macos-arm64/tool \
  --baseline-platform macos-arm64 \
  --failure-output-dir /tmp/failures     # normalized outputs of mismatches
```

The loop: edit normalization in `test/run_tests.py` → replay the raw captures
→ inspect `/tmp/failures/*.expected` diffs → repeat. No rebuild, no CI. The
raw captures are byte-stable input; normalization is pure text processing, so
results are deterministic and identical to what CI would compute.

Unit tests cover the normalizer itself and run in milliseconds:

```sh
cd test && python3 -m unittest test_run_tests test_extractor
```

## 7. Windows SDK bundle

The cross-build SDK is a tar of pinned msys2 package archives, published as an
immutable asset on the `windows-build-sdks` release:

```sh
# 1. Download the archives listed by the pins (URL-encode '+' as %2B):
source scripts/load_llvm_version.sh && load_llvm_version llvm-version.env
source scripts/windows_sdk_packages.sh
# HEAD-check every URL from msys2_sdk_archives for both triplets
# (clang64 x86_64, clangarm64 aarch64), then download into
# .deps/msys2-sdk-downloads/

# 2. Build the bundle (deterministic tar + inner SHA256SUMS) and print its sha:
scripts/create_windows_cross_sdk_bundle.sh

# 3. Publish: refuses to overwrite an existing asset with a different digest.
#    To replace: delete the asset (gh api -X DELETE .../releases/assets/<id>)
#    then run:
scripts/publish_windows_cross_sdk_bundle.sh

# 4. Pin the new digest in llvm-version.env (WINDOWS_SDK_SHA256) in the same
#    commit as the pin changes, and verify the download path end-to-end:
#    download the asset, sha256sum --check against the pin, extract, and
#    sha256sum --check SHA256SUMS inside the bundle.
```

When replacing an existing bundle, diff the inner `SHA256SUMS` against the
previously published one: for a runtime-only change (e.g. the openmp bump)
every other archive must be byte-identical — that is what makes the Windows
binaries comparable across the change.

## 8. Known landmines for the next bumps

Verified against upstream sources and empirically during the 19 bump:

- **clang 20**: no API churn in the code this repo overrides —
  `PPCallbacks::InclusionDirective` (19 shape), the template-parameter
  default-argument accessors (`getArgument().getAsType()` /
  `getSourceExpression()`), and `TemplateName::print(raw_ostream&,
  PrintingPolicy, Qualified)` are all unchanged. The output's
  `template template` doubling bug (see below) is **still present in 20**.
- **clang 21**: `NestedNameSpecifier::TypeSpecWithTemplate` is **removed**
  (upstream PR #133610 moved the template keyword into
  `DependentTemplateStorage`). This repo switches on it in two places:
  `src/ChildrenVisitor/ChildrenVisitorExtras.cpp` and `src/Clang/ClangNodes.cpp`
  — a shim in `src/Clang/ClangVersion.h` will be needed. Also expect the
  golden baselines to shrink by one `template ` keyword on every dependent
  qualified template-id (upstream fixed the double-keyword printing bug in 21;
  it exists in 19 and 20 and is visible in the current baselines as
  `::template template rebind<char>::`).
- **trunk (22)**: bigger AST refactor — `NestedNameSpecifier` became a
  lightweight handle, `ElaboratedType` was removed (PR #147835), and
  `DependentTemplateSpecializationType` was folded into
  `TemplateSpecializationType` (PR #158109). Dumper code that walks or
  switches on these types needs review at that bump.
- **TemplateArgument kinds**: clang 19 added `StructuralValue`; the dumper
  throws on unknown kinds (`src/Clang/ClangNodes.cpp`) — fail-loud by design;
  a new upstream kind will surface as a runtime crash, not silent corruption.
- **Output-format churn taxonomy** (learned from 18→19; expect similar at the
  next bump): (a) printer spelling changes (qualified names, `template`
  keywords, typedef sugar in template arguments); (b) AST-shape changes
  (clang 19 resolved 48 previously-dependent member expressions into real
  `MemberExprData` records); (c) system-header line/column shifts from
  distro header drift — noise, but breaks anything downstream caching
  per-header positions. Diff the shared baselines old-vs-new and classify
  every change into these buckets before writing the PR body.
- **msys2 package renames** can silently freeze pins (see §1).
- **CI artifact retention is 24h** — do the baseline replay the same day as
  the CI run, or re-run CI.
- **Platform baseline precedence** (§3): stale platform files silently win
  over the shared baseline. Refresh or prune every platform dir, every bump.
