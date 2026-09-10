# FlatBuffers AST wire generation

The schema is [`wire.fbs`](wire.fbs). The generated C++ header is a build
artifact under `build/generated/wire`; it is intentionally not committed.

The native pilot uses FlatBuffers commit
`7e163021e59cca4f8e1e35a7c828b5c6b7915953` from the upstream `v25.12.19` tag. The Clava experiment's `bootstrap.py` fetches and verifies it.

Configure and build with the pinned SDK as follows:

```sh
export FLATBUFFERS_ROOT=$HOME/.cache/ast-flatbuffers-planning/flatbuffers
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DSKIP_ENUM_GENERATION=OFF \
  -DAST_WIRE_FLATC="$FLATBUFFERS_ROOT/build-make/flatc" \
  -DAST_WIRE_FLATBUFFERS_INCLUDE_DIR="$FLATBUFFERS_ROOT/include"
cmake --build build --target tool verify_flat_wire --parallel 4
```

This invokes `flatc --cpp --scoped-enums` during the build and places
`wire_generated.h` in `build/generated/wire`.

## Running the pilot

With `AST_WIRE_FLAT=1`, `tool -c source.c -o dump.flat -- -std=c11`
produces the hybrid binary stream. Without the environment variable the tool
keeps its existing text behavior. `AST_WIRE_DENSE_TEXT=1` selects a text control
with dense IDs. Use a separate cache namespace and include the selected format
in ccache's extra-file key, as the Clava experiment driver does: ccache does not
otherwise know about these environment switches.

Run `build/verify_flat_wire dump.flat` to check generated record buffers and
stream counts. This verifier is test-only and reads the whole input; the actual
writer emits bounded records and the Java reader maps windows incrementally.

This is a single-TU experiment. IDs restart per translation unit and need a TU
namespace before a multi-TU application can use them. The schema types ten
expression/statement payloads; unsupported payloads remain legacy text in Raw
records. Do not treat this schema as a complete or production-stable AST protocol.
