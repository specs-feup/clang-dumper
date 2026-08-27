#!/usr/bin/env python3
"""
Corpus smoke-test runner for clang-dumper.

Runs the dumper over Clang's own test-suite corpus (checked out at the LLVM
version pinned in llvm-version.env) and classifies every file:

    SKIP_*            not runnable here (no cc1 RUN line, needs lit setup,
                      non-C/C++ language, or UNSUPPORTED for this host)
    HARNESS_NO_JOB    lit-only flag combination the driver cannot turn into a
                      single compile job (not a dumper defect)
    ENV_SKIP_CUDA     needs a CUDA installation this host does not have
    PARSE_FAIL        Clang itself rejects the file under its own flags
    EXPECTED_ERR      file uses -verify; its parse errors are intentional
    TIMEOUT           exceeded the time limit in either stage
    CRASH             dumper died on a signal (real dumper bug)
    DUMP_FAIL         parses, but the dumper exited non-zero
    CLEAN             parsed and dumped successfully

The effective language standard is decoded from the dump stream itself: the
<Compiler Instance Data> header serializes LangOpts in a fixed order
(LineComment, GNUInline, C99, C11, CPlusPlus..CPlusPlus26, Digraphs, GNUMode,
HexFloats, OpenCL, OpenCLVersion, NativeHalfType, CUDA, Bool, Half, WChar),
so files compiled without an explicit -std= are reported under the standard
Clang actually inferred (e.g. gnu++17), not "(default)".

Aggregates handler encounters (-handler-coverage-all) across the corpus and
reports node kinds from the pinned LLVM's TableGen inventories that were never
encountered, split into in-scope (C/C++ mission) and out-of-scope
(ObjC/HLSL/SYCL) kinds.

Usage:
    python3 test/corpus_runner.py --tool build/tool \
        --corpus /path/to/llvm-project/clang/test --out /tmp/corpus-results

Corpus fetch:
    git clone --filter=blob:none --no-checkout --depth 1 \
        --branch llvmorg-18.1.8 https://github.com/llvm/llvm-project.git
    cd llvm-project && git sparse-checkout set clang/test clang/include && git checkout
"""

import argparse
import json
import re
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path

SOURCE_EXTS = {".c", ".cpp", ".cc", ".cxx", ".cu", ".cl"}
CLANG_LANGS = {"c", "c++", "c++11", "c++14", "c++17", "c++20", "c++23",
               "c++26", "cl", "cl1.2", "cl2.0", "clcpp"}
HOST_UNSUPPORTED_MARKERS = {"windows", "darwin", "macos", "aix",
                            "system-windows"}

RUN_RE = re.compile(r"//\s*RUN:\s*(.*)")
UNSUPPORTED_RE = re.compile(r"^\s*UNSUPPORTED\s*:(.*)", re.MULTILINE)

# Tokens from cc1 RUN lines that are safe/useful for plain parsing.
FLAG_ALLOWLIST_PREFIXES = (
    "-std=", "-D", "-U", "-I", "-F", "-isystem", "-iquote", "-idirafter",
    "-include", "-imacros", "-fopenmp", "-fopenmp-simd",
    "-fgnu89-inline", "-fgnuc-version=", "-fms-extensions",
    "-fms-compatibility", "-fms-compatibility-version=", "-fmsc-version=",
    "-fdelayed-template-parsing", "-fexceptions", "-fno-exceptions",
    "-fcxx-exceptions", "-fno-cxx-exceptions", "-fms-dialect",
    "-fblocks", "-fpascal-strings", "-fborland-extensions",
    "-fstrict-return", "-fms-inline-asm", "-fasm-blocks", "-fshort-enums",
    "-fshort-wchar", "-ffixed-point", "-fnative-half-type",
    "-fallow-half-arguments-and-returns", "-fcuda-is-device",
    "-fdeclare-opencl-builtins", "-fprebuilt-module-path=", "-fmodule-file=",
    "-fmodule-map-file=", "-fno-builtin", "-fbuiltin", "-ffreestanding",
    "-ffake-address-space-map", "-finclude-default-header",
    "-fdouble-square-bracket-attributes", "-fcomplete-member-pointers",
    "-faligned-allocation", "-fnew-alignment=", "-fcoroutines-ts",
    "-fexperimental-library", "-fpreserve-vec3-type",
    "-faligned-alloc-unavailable", "-cl-std=", "-cl-fast-relaxed-math",
    "-cl-fp32-correctly-rounded-divide-sqrt",
    "-cl-single-precision-constant", "-cl-mad-enable", "-cl-no-signed-zeros",
    "-cl-unsafe-math-optimizations", "-cl-denorms-", "-cl-no-stdinc",
)
FLAG_ALLOWLIST_EXACT = {"-w", "-nostdinc", "-nobuiltininc", "-nostdsysteminc"}
PAIRED_FLAGS = {
    "-triple", "-target", "-isystem", "-iquote", "-idirafter", "-include",
    "-imacros", "-std", "-x", "-mcpu", "-march", "-mfpu", "-mfloat-abi",
    "-fmodule-file", "-fmodule-map-file", "-fprebuilt-module-path",
    "-internal-isystem", "-internal-externc-isystem", "-resource-dir",
    "-aux-triple", "-main-file-name", "-target-feature", "-target-cpu",
    "-target-abi", "-mlink-builtin-bitcode", "-fdebug-default-version",
    "-I", "-F", "-mfpmath",
}
TARGET_FLAG_ALLOWLIST_PREFIXES = (
    "-mcpu=", "-march=", "-mfpu=", "-mfloat-abi=", "-mvsx", "-maltivec",
    "-mmmx", "-msse", "-msse2", "-msse3", "-mssse3", "-msse4", "-msse4a",
    "-mavx", "-maes", "-mpclmul", "-mbmi", "-mfma", "-mlong-double-64",
    "-mlong-double-80", "-mlong-double-128", "-msoft-float", "-mhard-float",
    "-mthumb", "-marm", "-mcrc", "-mcumode", "-mwavefrontsize64",
    "-mcmse", "-mrestrict", "-mxnack", "-msram-ecc", "-mno-dxp",
)

# Flags that only add declarations/search paths and are safe to merge in from
# later %clang_cc1 RUN lines of the same test file.
ADDITIVE_PREFIXES = ("-D", "-U")
ADDITIVE_PAIRED = {"-include", "-imacros", "-idirafter", "-iquote",
                   "-isystem", "-I", "-F"}

OUT_OF_SCOPE_PREFIXES = ("ObjC", "HLSL", "SYCL")


def node_in_scope(classname: str) -> bool:
    return not classname.startswith(OUT_OF_SCOPE_PREFIXES)


@dataclass(frozen=True)
class CorpusJob:
    flags: list[str]
    uses_verify: bool


def tokenize_cc1(cmd: str):
    return cmd.split()


def extract_jobs(text: str):
    """Extract one independent job from each usable %clang_cc1 RUN line."""
    cc1_cmds = []
    for m in RUN_RE.finditer(text):
        if "%clang_cc1" in m.group(1):
            cc1_cmds.append(m.group(1))
    if not cc1_cmds:
        return [], "SKIP_NO_CC1_RUN"

    def usable(cmd):
        return not any(s in cmd for s in ("%t", "%T", "%@", "%python"))

    usable_cmds = [cmd for cmd in cc1_cmds if usable(cmd)]
    if not usable_cmds:
        return [], "SKIP_NEEDS_LIT_SETUP"

    def to_flags(tokens):
        flags, lang, i = [], None, 0
        while i < len(tokens):
            tok = tokens[i]
            i += 1
            if tok.startswith("%") or tok == "-cc1":
                continue
            if tok == "-verify" or tok.startswith("-verify=") \
                    or tok == "-verify-ignore-unexpected":
                continue
            if tok == "-x":
                if i < len(tokens):
                    lang = tokens[i]
                    flags += ["-x", tokens[i]]
                    i += 1
                continue
            # Normalize `=`-joined target/triple flags so driver-mode
            # translation applies (-std= is valid in both dialects as-is)
            for p in ("-triple", "-target"):
                if tok.startswith(p + "="):
                    flags += [p, tok[len(p) + 1:]]
                    break
            else:
                paired = next(
                    (p for p in PAIRED_FLAGS if tok.startswith(p + "=")),
                    None,
                )
                if paired is not None:
                    flags += [paired, tok[len(paired) + 1:]]
                    continue
                if tok in PAIRED_FLAGS:
                    if i < len(tokens):
                        flags += [tok, tokens[i]]
                        i += 1
                    continue
                if tok.startswith(FLAG_ALLOWLIST_PREFIXES) or tok in FLAG_ALLOWLIST_EXACT:
                    flags.append(tok)
                    continue
                if tok.startswith(TARGET_FLAG_ALLOWLIST_PREFIXES):
                    flags.append(tok)
                    continue
        return flags, lang

    jobs = []
    skipped_languages = []
    for cmd in usable_cmds:
        tokens = tokenize_cc1(cmd)
        flags, lang = to_flags(tokens)
        if lang is not None and lang not in CLANG_LANGS:
            skipped_languages.append(lang)
            continue
        uses_verify = any(tok == "-verify" or tok.startswith("-verify=")
                          for tok in tokens)
        jobs.append(CorpusJob(flags, uses_verify))

    if not jobs:
        lang = skipped_languages[0] if skipped_languages else "unknown"
        return [], f"SKIP_LANG_{lang}"
    return jobs, None


CC1_ONLY_PAIRED_FLAGS = {
    "-mfpmath", "-target-feature", "-target-cpu", "-target-abi",
}


def to_driver_flags(cc1_flags: list[str]) -> tuple[list[str], list[str]]:
    """Translate cc1-style flags to the driver-style dialect the dumper tool
    accepts after `--`. The corpus RUN lines are written for %clang_cc1.
    Returns (driver_flags, cc1_only_flags_that_were_dropped)."""
    out, dropped = [], []
    i = 0
    while i < len(cc1_flags):
        f = cc1_flags[i]
        if f == "-triple" and i + 1 < len(cc1_flags):
            out += ["-target", cc1_flags[i + 1]]
            i += 2
            continue
        if f in CC1_ONLY_PAIRED_FLAGS:
            if i + 1 < len(cc1_flags):
                out += ["-Xclang", f, "-Xclang", cc1_flags[i + 1]]
                i += 2
            else:
                dropped.append(f)
                i += 1
            continue
        if f in ("-aux-triple", "-main-file-name", "-internal-isystem",
                 "-internal-externc-isystem"):
            i += 2 if i + 1 < len(cc1_flags) else 1
            continue
        out.append(f)
        i += 1
    return out, dropped


def infer_std_from_dump(stderr_text: str) -> str | None:
    """Decode the effective language standard from the <Compiler Instance
    Data> header, which dumps LangOpts in a fixed order."""
    lines = stderr_text.splitlines()
    try:
        start = lines.index("<Compiler Instance Data>")
    except ValueError:
        return None
    vals = lines[start + 2: start + 23]
    if len(vals) < 21:
        return None
    (line_comment, _gnu_inline, c99, c11, cxx, cxx11, cxx14, cxx17,
     cxx20, cxx23, cxx26, _digraphs, gnu_mode, _hex_floats, opencl,
     opencl_version, _native_half, cuda, *_rest) = vals

    def b(s):
        return s == "1"

    if b(cuda):
        return "cuda"
    if b(opencl):
        return f"opencl{opencl_version}"
    if b(cxx):
        level = ("c++26" if b(cxx26) else "c++23" if b(cxx23) else
                 "c++20" if b(cxx20) else "c++17" if b(cxx17) else
                 "c++14" if b(cxx14) else "c++11" if b(cxx11) else "c++98")
        return f"gnu{level[1:]}" if b(gnu_mode) else level
    if b(c11):
        return "gnu11" if b(gnu_mode) else "c11"
    if b(c99):
        return "gnu99" if b(gnu_mode) else "c99"
    return "gnu89" if b(gnu_mode) else "c89"


def parse_unsupported(text: str) -> set[str]:
    m = UNSUPPORTED_RE.search(text)
    if not m:
        return set()
    return {p.strip() for p in m.group(1).split(",") if p.strip()}


def run_stage1(clang: str, source: Path, flags: list[str]):
    """Clang must accept its own file before we bother dumping it."""
    cmd = [clang, "-cc1", "-fsyntax-only"] + flags + [str(source)]
    try:
        proc = subprocess.run(
            cmd, cwd=str(source.parent), stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE, text=True, timeout=20,
        )
    except subprocess.TimeoutExpired:
        return "TIMEOUT", ""
    if proc.returncode == 0:
        return "PARSE_OK", ""
    err = proc.stderr
    idx = err.find("error:")
    reason = err[idx:idx + 160].strip() if idx >= 0 else err[-160:].strip()
    return "PARSE_FAIL", reason.replace("\n", " ").replace("\r", "")


def run_stage2(tool: str, source: Path, flags: list[str], resource_dir: str):
    driver_flags, dropped = to_driver_flags(flags)
    if resource_dir:
        driver_flags += ["-resource-dir", resource_dir]
    cmd = [tool, "-id=0", "-handler-coverage-report", "-handler-coverage-all"]
    cmd += [str(source), "--"] + driver_flags
    try:
        proc = subprocess.run(
            cmd, cwd=str(source.parent), stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, timeout=30,
        )
    except subprocess.TimeoutExpired:
        return "TIMEOUT", {}, None, dropped
    stderr = proc.stderr.decode(errors="replace")
    eff_std = infer_std_from_dump(stderr)

    if "expected exactly one compiler job" in stderr:
        return "HARNESS_NO_JOB", {}, eff_std, dropped
    if "cannot find CUDA installation" in stderr or "--cuda-path" in stderr:
        return "ENV_SKIP_CUDA", {}, eff_std, dropped
    if "amdgpu-arch" in stderr or "--rocm-path" in stderr \
            or "ROCm device library" in stderr:
        return "ENV_SKIP_GPU", {}, eff_std, dropped
    if proc.returncode == 0:
        result = "CLEAN"
    elif proc.returncode < 0:
        result = "CRASH"
    else:
        # The corpus is written for %clang_cc1, whose defaults differ from
        # the driver-mode invocation the dumper uses:
        #  - the driver enables -fexceptions for C++, turning some warnings
        #    into errors
        #  - the driver auto-includes opencl-c.h for OpenCL sources (tests
        #    that declare their own built-in types then conflict)
        #  - triples whose backend archive the build filters out (non-host
        #    architectures) cannot construct a TargetInfo
        if "when exceptions are enabled" in stderr \
                or "missing exception specification" in stderr \
                or "unhandled_exception()" in stderr:
            return "HARNESS_CC1_DEFAULTS", {}, eff_std, dropped
        if source.suffix.lower() == ".cl" and (
                "opencl-c-base.h" in stderr or "redefinition" in stderr):
            return "HARNESS_CC1_DEFAULTS", {}, eff_std, dropped
        if "unknown target triple 'unknown-" in stderr \
                or "MS-style inline assembly is not available" in stderr:
            return "HARNESS_TARGET_BACKEND", {}, eff_std, dropped
        result = "DUMP_FAIL"

    coverage = {}
    in_encounters = False
    for line in proc.stdout.decode(errors="replace").splitlines():
        if line.startswith("[clang-dumper] handler encounters:"):
            in_encounters = True
            continue
        if in_encounters:
            if not line.startswith("  ") or line.startswith("[clang-dumper]"):
                break
            stripped = line.strip()
            if stripped.endswith(":") and ": " not in stripped:
                continue
            family, _, rest = stripped.partition(": ")
            if rest:
                coverage[family] = rest.split()
    return result, coverage, eff_std, dropped


def aggregate_bucket(buckets: list[str]) -> str:
    """Keep file-level reporting useful while preserving every job result."""
    if not buckets:
        return "SKIP_NO_JOBS"
    for bucket in ("CRASH", "TIMEOUT", "DUMP_FAIL"):
        if bucket in buckets:
            return bucket
    if "CLEAN" in buckets:
        if all(bucket in {"CLEAN", "EXPECTED_ERR"} for bucket in buckets):
            return "CLEAN"
        return "PARTIAL"
    if "HARNESS_TARGET_FLAGS" in buckets:
        return "HARNESS_TARGET_FLAGS"
    if "HARNESS_TARGET_BACKEND" in buckets:
        return "HARNESS_TARGET_BACKEND"
    if "HARNESS_CC1_DEFAULTS" in buckets:
        return "HARNESS_CC1_DEFAULTS"
    if "HARNESS_NO_JOB" in buckets:
        return "HARNESS_NO_JOB"
    if "EXPECTED_ERR" in buckets:
        return "EXPECTED_ERR"
    return buckets[0]


def process_file(args):
    tool, clang, path_str, resource_dir = args
    source = Path(path_str)
    try:
        text = source.read_text(errors="replace")
    except OSError:
        return path_str, "SKIP_READ_ERROR", "", {}, {}

    unsupported = parse_unsupported(text)

    jobs, skip_reason = extract_jobs(text)
    if not jobs:
        return path_str, skip_reason, "", {}, {}
    if unsupported & HOST_UNSUPPORTED_MARKERS:
        return path_str, "SKIP_UNSUPPORTED_TARGET", "", {}, {}

    job_results = []
    all_coverage = {}
    for job_index, job in enumerate(jobs):
        stage1, reason = run_stage1(clang, source, job.flags)
        if stage1 != "PARSE_OK":
            bucket = "EXPECTED_ERR" if (job.uses_verify
                                         and stage1 == "PARSE_FAIL") \
                else stage1
            job_results.append({
                "index": job_index,
                "bucket": bucket,
                "reason": reason,
                "flags": job.flags,
                "std": None,
                "effective_std": None,
                "coverage": {},
            })
            continue

        result, coverage, eff_std, dropped = run_stage2(
            tool, source, job.flags, resource_dir)
        if result == "DUMP_FAIL" and dropped:
            result = "HARNESS_TARGET_FLAGS"
        std_key = None
        for j, f in enumerate(job.flags):
            if f == "-std" and j + 1 < len(job.flags):
                std_key = job.flags[j + 1]
            elif f.startswith("-std="):
                std_key = f[5:]
            elif f.startswith("-cl-std="):
                std_key = f[8:]
        job_results.append({
            "index": job_index,
            "bucket": result,
            "reason": "",
            "flags": job.flags,
            "std": std_key,
            "effective_std": eff_std,
            "coverage": coverage,
        })
        for family, classes in coverage.items():
            all_coverage.setdefault(family, set()).update(classes)

    for job in job_results:
        job["coverage"] = {
            family: sorted(classes)
            for family, classes in job["coverage"].items()
        }
    buckets = [job["bucket"] for job in job_results]
    reasons = sorted({job["reason"] for job in job_results if job["reason"]})
    first = job_results[0]
    return path_str, aggregate_bucket(buckets), "; ".join(reasons), {
        family: sorted(classes) for family, classes in all_coverage.items()
    }, {
        "jobs": job_results,
        "flags": first["flags"],
        "std": first["std"],
        "effective_std": first["effective_std"],
    }


def _parse_node_td(path: Path):
    """Parse a TableGen ASTNodes.td into {name: base}, plus abstract set."""
    nodes, abstract = {}, set()
    def_re = re.compile(
        r"^\s*def\s+([A-Za-z0-9_]+)\s*:\s*\w*Node<\s*(\??)([A-Za-z0-9_]*)\s*(?:,\s*\d)?")
    abs_re = re.compile(r"<[^<>]*,\s*1\s*>")
    for line in path.read_text(errors="replace").splitlines():
        m = def_re.match(line)
        if m:
            name, _, base = m.groups()
            nodes[name] = base or None
            if abs_re.search(line):
                abstract.add(name)
    return nodes, abstract


def _descendants(nodes: dict, root: str) -> tuple[set, set]:
    members = {n for n, b in nodes.items() if b == root or n == root}
    changed = True
    while changed:
        changed = False
        for n, b in nodes.items():
            if b in members and n not in members:
                members.add(n)
                changed = True
    return members - {root}, members


def load_inventory(corpus_root: Path):
    basic = corpus_root.parent / "include" / "clang" / "Basic"
    inv = {}

    stmt_nodes, stmt_abstract = _parse_node_td(basic / "StmtNodes.td")
    _, stmt_all = _descendants(stmt_nodes, "Stmt")
    _, expr_all = _descendants(stmt_nodes, "Expr")
    inv["stmt data"] = sorted((stmt_all - expr_all) - stmt_abstract)
    inv["expr data"] = sorted(expr_all - stmt_abstract)

    decl_nodes, decl_abs = _parse_node_td(basic / "DeclNodes.td")
    _, decl_all = _descendants(decl_nodes, "Decl")
    inv["decl data"] = sorted(
        n + "Decl" for n in decl_all - {"Decl"} - decl_abs
    )

    type_nodes, type_abs = _parse_node_td(basic / "TypeNodes.td")
    _, type_all = _descendants(type_nodes, "Type")
    inv["type data"] = sorted(type_all - {"Type"} - type_abs)

    attrs = []
    attr_p = basic / "Attr.td"
    if attr_p.exists():
        for line in attr_p.read_text(errors="replace").splitlines():
            m = re.match(r"^def\s+([A-Za-z0-9_]+)\s*:\s*Attr\b", line)
            if m:
                attrs.append(m.group(1) + "Attr")
    inv["attr data"] = attrs
    return inv


FAMILY_TO_INVENTORY = {
    "decl data": "decl data",
    "stmt data": "stmt data",
    "expr data": "expr data",
    "type data": "type data",
    "attr data": "attr data",
}


def main():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--tool", required=True)
    parser.add_argument("--clang", default="clang-18")
    parser.add_argument("--corpus", required=True, type=Path,
                        help="Path to llvm-project/clang/test")
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--jobs", type=int, default=8)
    parser.add_argument("--limit", type=int, default=0,
                        help="Only process the first N files (debugging)")
    parser.add_argument("--no-resource-dir", action="store_true",
                        help="Do not append -resource-dir to tool runs")
    args = parser.parse_args()
    args.tool = str(Path(args.tool).resolve())

    resource_dir = ""
    if not args.no_resource_dir:
        try:
            resource_dir = subprocess.run(
                [args.clang, "-print-resource-dir"], capture_output=True,
                text=True, timeout=30,
            ).stdout.strip()
        except Exception:
            resource_dir = ""

    files = sorted(
        str(p) for p in args.corpus.rglob("*")
        if p.suffix.lower() in SOURCE_EXTS and p.is_file()
    )
    if args.limit:
        files = files[:args.limit]
    print(f"Corpus: {len(files)} candidate files "
          f"(resource-dir: {resource_dir or 'none'})", file=sys.stderr)

    results = {}
    with ProcessPoolExecutor(max_workers=args.jobs) as pool:
        futures = [
            pool.submit(process_file, (args.tool, args.clang, f, resource_dir))
            for f in files
        ]
        done = 0
        for fut in as_completed(futures):
            path, bucket, reason, coverage, meta = fut.result()
            results[path] = {"bucket": bucket, "reason": reason,
                             "coverage": coverage, **meta}
            done += 1
            if done % 500 == 0:
                print(f"  {done}/{len(files)}", file=sys.stderr)

    args.out.mkdir(parents=True, exist_ok=True)

    buckets = {}
    job_buckets = {}
    for r in results.values():
        buckets[r["bucket"]] = buckets.get(r["bucket"], 0) + 1
        for job in r.get("jobs", []):
            job_bucket = job["bucket"]
            job_buckets[job_bucket] = job_buckets.get(job_bucket, 0) + 1

    encounters: dict[str, set] = {}
    std_matrix = {}
    default_std_counts = {}
    for path, r in results.items():
        for family, classes in r["coverage"].items():
            encounters.setdefault(family, set()).update(classes)
        for job in r.get("jobs", []):
            if job["bucket"] not in ("CLEAN", "DUMP_FAIL"):
                continue
            std = job.get("std") or job.get("effective_std") or "(unknown)"
            entry = std_matrix.setdefault(std, {})
            entry[job["bucket"]] = entry.get(job["bucket"], 0) + 1
            if not job.get("std"):
                default_std_counts[std] = default_std_counts.get(std, 0) + 1

    inventory = load_inventory(args.corpus)
    uncovered_in, uncovered_out = {}, {}
    for family, inv_key in FAMILY_TO_INVENTORY.items():
        seen = encounters.get(family, set())
        classes = sorted(set(inventory.get(inv_key, [])) - seen)
        uncovered_in[family] = [c for c in classes if node_in_scope(c)]
        out = [c for c in classes if not node_in_scope(c)]
        if out:
            uncovered_out[family] = out

    catalog = {
        "totals": buckets,
        "job_totals": job_buckets,
        "files_processed": len(files),
        "driver_default_effective_std": dict(sorted(default_std_counts.items())),
        "per_std_matrix": dict(sorted(std_matrix.items())),
        "uncovered_nodes_in_scope": uncovered_in,
        "uncovered_nodes_out_of_scope": uncovered_out,
        "encountered_nodes": {k: sorted(v) for k, v in encounters.items()},
        "crashes": [
            {"file": p} for p, r in sorted(results.items())
            if r["bucket"] == "CRASH"
        ],
        "dump_failures": [
            {"file": p} for p, r in sorted(results.items())
            if r["bucket"] == "DUMP_FAIL"
        ],
        "timeouts": [
            {"file": p} for p, r in sorted(results.items())
            if r["bucket"] == "TIMEOUT"
        ],
    }

    (args.out / "results.json").write_text(json.dumps(results, indent=1))
    (args.out / "catalog.json").write_text(json.dumps(catalog, indent=1))

    print("\n=== Summary ===")
    for k, v in sorted(buckets.items()):
        print(f"  {k}: {v}")
    print("\n=== Effective std for files without explicit -std ===")
    for k, v in sorted(default_std_counts.items()):
        print(f"  {k}: {v}")
    print("\n=== Uncovered node kinds (in scope) ===")
    for family, classes in uncovered_in.items():
        total_inv = len(inventory.get(FAMILY_TO_INVENTORY[family], []))
        n_out = len(uncovered_out.get(family, []))
        print(f"  {family}: {len(classes)} uncovered of "
              f"{total_inv - n_out} in-scope ({n_out} out-of-scope)")
    print(f"\nResults written to {args.out}")


if __name__ == "__main__":
    main()
