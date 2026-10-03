# Eager FlatBuffers AST wire format

The production dumper emits the complete eager FlatBuffers v2 stream. AST output
always requires a file path; format and compression switches are not supported.
The plugin uses the same protocol and requires `-output=<path>`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target tool plugin verify_flatbuffers --parallel
build/tool -c source.cpp -o source.clv2 -- -std=c++17
build/verify_flatbuffers source.clv2
```

The native build fetches the FlatBuffers source commit in
[`flatbuffers-version.env`](../flatbuffers-version.env), builds its own `flatc`,
and verifies that the compiler and runtime versions match. Cross builds build a
host `flatc` from that same commit. No adjacent checkout is used.

## Schema and consumer naming

The bundle entrypoint is `wire/v2/complete.fbs`. It includes the record envelope
and all declaration, type, expression, statement, attribute, and common payloads.
The schema contains wire-format definitions only; Clava owns its Java reader,
key and enum mapping, and generated consumer bindings.

Consumer table names follow one rule: remove the single `Data` suffix to find
the Java node class (`FunctionDeclData` maps to `FunctionDecl`, and
`ClavaNodeData` maps to `ClavaNode`). Wire fields use the snake-case form of
their consumer key names. Do not add aliases or consumer-specific schema
annotations. Optional-scalar presence is part of the wire contract; mark a field
`wire_optional` only when absence is semantically valid. Keep structural fields
that have no consumer key named for their protocol role.

## Adding or changing a node or field

1. Edit the relevant schema under `wire/v2/`. Add a payload alternative to
   `NodePayload` in `complete.fbs` when introducing a node table. Follow the
   `*Data` naming rule and use strict field names.
2. Implement the producer mapping in the matching `src/Clava/Flat*.cpp` file.
   Add Clang visitor coverage in `src/ClavaDataDumper/` when introducing a new
   Clang node kind.
3. Regenerate the C++ schema bindings, dispatch, enum support, and declaration
   includes with the pinned compiler, then update the generated-output
   inventory:

   ```sh
   cmake --build build --target flatc
   python3 scripts/generate_complete_wire.py \
     --flatc build/_deps/flatbuffers-build/flatc \
     --out build/wire-v2 \
     --manifest wire/generated.sha256 \
     --update-manifest
   ```

   Normal builds omit `--update-manifest`; they fail when committed generated
   output inventory differs. The Clava consumer independently generates its
   bindings from the published schema using the naming contract above.
4. Build and run `verify_flatbuffers`, native tool/plugin corpus validation, and
   the Clava consumer tests. Check stream fidelity before changing the schema
   version for an incompatible wire change.
5. Publish a compatible dumper release. The release job packages every
   `wire/v2/*.fbs` file, writes `clang-dumper-release-manifest.json`, and uploads
   the schema bundle with the tools. Consumers select the exact release using
   `clang-dumper-release.tag` and verify the manifest hashes before compiling
   their reader.

The canonical schema hash is SHA-256 over files sorted by POSIX relative path.
For each file, hash the UTF-8 path (`wire/v2/<name>`), one NUL byte, exact file
bytes, and one NUL byte. The manifest also records the archive's own SHA-256 so
the downloaded bundle can be checked independently. `schema_version` and the
FlatBuffers version and source commit are recorded beside the v2 entrypoint.

## Stream framing

Each size-prefixed `CLV2` block contains typed records. Blocks target 64 KiB so
records can share vtables without buffering a whole translation unit; a single
large record may exceed that size. The first record carries the schema hash and
the terminal End record carries stream, node, file, and dense-ID counts. Paths
are interned. Positive node IDs are dense within one translation unit; negative
IDs identify typed null references. Every AST reference is eagerly encoded.
