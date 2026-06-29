#!/usr/bin/env python3
import argparse
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path


def parse_search_dirs(output):
    dirs = []
    in_search = False
    for line in output.splitlines():
        if line.strip() == "#include <...> search starts here:":
            in_search = True
            continue
        if line.strip() == "End of search list.":
            break
        if not in_search:
            continue
        path = line.replace("(framework directory)", "").strip()
        if path:
            dirs.append(Path(path).resolve())
    return [path for path in dict.fromkeys(dirs) if path.is_dir()]


def is_relative_to(path, parent):
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def minimal_roots(paths):
    roots = []
    for path in sorted(paths, key=lambda item: (len(item.parts), str(item))):
        if not any(is_relative_to(path, root) for root in roots):
            roots.append(path)
    return roots


def clean_name(value):
    return "".join(char if char.isalnum() else "-" for char in value).strip("-")


def root_name(path, platform):
    parts = path.parts
    if len(parts) >= 4 and path.name == "include" and parts[-3] == "clang":
        return "clang"
    if platform == "linux" and path == Path("/usr/include"):
        return "usr"
    if platform == "windows" and path.name == "include":
        return "mingw"
    if platform == "macos":
        if len(parts) >= 3 and parts[-3:] == ("include", "c++", "v1"):
            return "libcxx"
        if "SDKs" in parts and len(parts) >= 2 and parts[-2:] == ("usr", "include"):
            return "sdk"
    return clean_name(path.name)


def unique_names(roots, platform):
    names = {}
    used = set()
    for root in roots:
        base = root_name(root, platform)
        name = base
        index = 2
        while name in used:
            name = f"{base}-{index}"
            index += 1
        used.add(name)
        names[root] = name
    return names


def make_entrypoints(paths, roots, names):
    lines = []
    for path in paths:
        root = next(root for root in roots if is_relative_to(path, root))
        relative = path.relative_to(root)
        entry = names[root] if str(relative) == "." else f"{names[root]}/{relative.as_posix()}"
        lines.append(entry)
    return lines


def zip_dir(source, output):
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(source.rglob("*")):
            target = path
            if path.is_symlink():
                target = path.resolve(strict=False)
                if not target.exists() or target.is_dir():
                    continue
            if not target.exists():
                continue
            archive.write(target, path.relative_to(source))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", required=True, choices=["linux", "macos", "windows"])
    parser.add_argument("--staging", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()

    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    result = subprocess.run(command, input="", text=True, capture_output=True)
    output = result.stdout + result.stderr
    if result.returncode != 0:
        sys.stderr.write(output)
        return result.returncode

    include_dirs = parse_search_dirs(output)
    roots = minimal_roots(include_dirs)
    names = unique_names(roots, args.platform)
    entrypoints = make_entrypoints(include_dirs, roots, names)

    shutil.rmtree(args.staging, ignore_errors=True)
    args.staging.mkdir(parents=True)
    for root in roots:
        shutil.copytree(root, args.staging / names[root], symlinks=True)
    (args.staging / "entrypoints.txt").write_text("\n".join(entrypoints) + "\n")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.output.exists():
        args.output.unlink()
    zip_dir(args.staging, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
