#!/usr/bin/env python3
import argparse
import os
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
                try:
                    target = path.resolve(strict=False)
                except (OSError, RuntimeError):
                    continue
                if not target.exists() or target.is_dir():
                    continue
            if not target.exists():
                continue
            archive.write(target, path.relative_to(source))
        archive_framework_directory_aliases(archive, source)


def copy_ignore_excluded(excluded_dirs):
    def ignore(directory, names):
        ignored = set()
        parent = Path(directory).resolve()
        for name in names:
            candidate = (parent / name).resolve(strict=False)
            if any(
                candidate == excluded or is_relative_to(candidate, excluded)
                for excluded in excluded_dirs
            ):
                ignored.add(name)
        return ignored

    return ignore


def archive_framework_directory_aliases(archive, source):
    """Materialize known framework directory aliases without following arbitrary links."""
    source_root = source.resolve()
    pending = [
        (path, path.relative_to(source))
        for path in sorted(source.rglob("*.framework"))
        if path.is_dir() and not path.is_symlink()
    ]
    processed = set()
    while pending:
        framework, archive_framework = pending.pop()
        framework_root = framework.resolve()
        archive_framework = Path(archive_framework)
        framework_key = (framework_root, archive_framework.as_posix())
        if framework_key in processed:
            continue
        processed.add(framework_key)
        if not is_relative_to(framework_root, source_root):
            continue

        for alias_name in ("Headers", "PrivateHeaders", "Modules", "Frameworks"):
            alias = framework / alias_name
            if not alias.is_symlink():
                continue
            try:
                target_root = alias.resolve(strict=False)
            except (OSError, RuntimeError):
                continue
            if (
                not target_root.is_dir()
                or not is_relative_to(target_root, source_root)
                or not is_relative_to(target_root, framework_root)
                or target_root == framework_root
            ):
                continue

            archive_alias = archive_framework / alias_name
            for directory, child_dirs, filenames in os.walk(target_root, followlinks=False):
                directory_path = Path(directory)
                child_dirs[:] = sorted(
                    child for child in child_dirs
                    if not (directory_path / child).is_symlink()
                )
                if alias_name == "Frameworks":
                    for child in child_dirs:
                        child_framework = directory_path / child
                        if child.endswith(".framework") and child_framework.is_dir():
                            pending.append(
                                (
                                    child_framework,
                                    archive_alias / child_framework.relative_to(target_root),
                                )
                            )
                for filename in sorted(filenames):
                    file_path = directory_path / filename
                    try:
                        target_file = file_path.resolve(strict=False)
                    except (OSError, RuntimeError):
                        continue
                    if (
                        not target_file.is_file()
                        or not is_relative_to(target_file, source_root)
                    ):
                        continue
                    archive_name = archive_alias / file_path.relative_to(target_root)
                    archive.write(target_file, archive_name.as_posix())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", required=True, choices=["linux", "macos", "windows"])
    parser.add_argument("--staging", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument(
        "--extra-include-dir",
        action="append",
        default=[],
        type=Path,
        help="Additional include root to copy into the archive and append to entrypoints.txt",
    )
    parser.add_argument(
        "--exclude-include-dir",
        action="append",
        default=[],
        type=Path,
        help="Exclude an include directory subtree from copied roots and entrypoints.txt",
    )
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()

    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    result = subprocess.run(command, input="", text=True, capture_output=True)
    output = result.stdout + result.stderr
    if result.returncode != 0:
        sys.stderr.write(output)
        return result.returncode

    include_dirs = parse_search_dirs(output)
    for extra_dir in args.extra_include_dir:
        extra_dir = extra_dir.resolve()
        if not extra_dir.is_dir():
            print(f"warning: extra include dir does not exist, skipping: {extra_dir}", file=sys.stderr)
            continue
        include_dirs.append(extra_dir)

    include_dirs = [path for path in dict.fromkeys(include_dirs)]
    roots = minimal_roots(include_dirs)
    excluded_dirs = [path.resolve() for path in args.exclude_include_dir]
    for excluded in excluded_dirs:
        if not any(is_relative_to(excluded, root) for root in roots):
            print(
                f"excluded include dir is outside selected include roots: {excluded}",
                file=sys.stderr,
            )
            return 2
    include_dirs = [
        path
        for path in include_dirs
        if not any(path == excluded or is_relative_to(path, excluded) for excluded in excluded_dirs)
    ]
    roots = minimal_roots(include_dirs)
    names = unique_names(roots, args.platform)
    entrypoints = make_entrypoints(include_dirs, roots, names)

    shutil.rmtree(args.staging, ignore_errors=True)
    args.staging.mkdir(parents=True)
    for root in roots:
        shutil.copytree(
            root,
            args.staging / names[root],
            symlinks=True,
            ignore=copy_ignore_excluded(excluded_dirs),
        )
    (args.staging / "entrypoints.txt").write_text("\n".join(entrypoints) + "\n")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.output.exists():
        args.output.unlink()
    zip_dir(args.staging, args.output)


if __name__ == "__main__":
    raise SystemExit(main())
