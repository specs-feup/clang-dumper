# Complete FlatBuffers AST protocol

The current experiment uses [`v2/complete.fbs`](v2/complete.fbs), selected with
`-ast-dump-format=flatbuffers-v2`. Text remains the default. The format argument
participates in ccache invocation identity.

```sh
export FLATBUFFERS_ROOT=$HOME/.cache/ast-flatbuffers-planning/flatbuffers
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DSKIP_ENUM_GENERATION=OFF \
  -DAST_WIRE_FLATC="$FLATBUFFERS_ROOT/build-make/flatc" \
  -DAST_WIRE_FLATBUFFERS_INCLUDE_DIR="$FLATBUFFERS_ROOT/include"
cmake --build build --target tool --parallel 3
build/tool -c source.cpp -o source.flat -ast-dump-format=flatbuffers-v2 -- -std=c++17
```

Use upstream FlatBuffers v25.12.19, commit
`7e163021e59cca4f8e1e35a7c828b5c6b7915953`. Clava's experiment `bootstrap.py`
fetches and verifies this SDK. Python 3 runs the build-time adapter generator.

## Schema organization

The schema defines 112 node payload alternatives, corresponding to the existing
specialized or generic dumper handlers. It does not claim support for every Clang
AST class. Declarations, types, expressions, statements and attributes have separate
schema files. Required `base` tables compose the existing field hierarchy. Compound
values such as template arguments and constructor targets use tagged unions.
There is no text or Raw fallback in v2; unexpected legacy output is an error.

The schema is ordinary FlatBuffers. Standard custom attributes attach Clava DataKey,
Java enum and reference mappings. `scripts/generate_complete_wire.py` reads official
FlatBuffers reflection data, then generates native dispatch and Java field bindings.
`flatc` generates the C++ builders and Java binary accessors. Clang getters and the
adapters for compound Clava values remain handwritten.

Mandatory strings, vectors and tables use `(required)`. Scalars use explicit presence,
with generated Java checks rejecting omission unless marked `wire_optional`.
Reference properties are eager, including those nested inside compound values.
Java resolves them using existing typed Clava node queues. The generator checks
presence and enum/union alternatives; this is not a complete hostile-input verifier.

## File layout and ownership

Each size-prefixed `CLV2` Block contains typed records. The builder flushes at a
64 KiB target so records share vtables without buffering a whole translation unit.
One oversized record may exceed that target. Header carries a SHA-256 of the schema;
End carries counts. Paths are interned. Positive IDs are dense within one TU;
negative IDs distinguish null type, declaration, expression, statement and attribute
references. Java adds the parsing scope to IDs before multi-TU normalization.

The Java reader maps 64 MiB windows and retains the relevant buffers for lazy fields.
Mapped artifacts must be uncompressed and immutable. Clava enables ccache compression
for persistent binary entries, which ccache restores to an uncompressed file.
The cache path still waits for native completion before Java import begins.

The companion Clava worktree consumes this schema during its build. Publishing the
schema and hash through the clang-dumper release manifest is not implemented yet.

## Historical v1 pilot

The original `wire.fbs`, `AST_WIRE_FLAT=1` and `verify_flat_wire` target remain for
reproducing the earlier ten-payload hybrid experiment. They are not the complete
protocol and must not be combined with `-ast-dump-format=flatbuffers-v2`.
