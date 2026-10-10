# AST wire format

The dumper writes each translation unit as a FlatBuffers stream. AST output always requires a file path
(`-o <path>` for the tool, `-plugin-arg-DumpAst -output=<path>` for the plugin).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target tool plugin verify_flatbuffers --parallel
build/tool -c source.cpp -o source.clv2 -- -std=c++17
build/verify_flatbuffers source.clv2
```

The build fetches the FlatBuffers commit pinned in [`flatbuffers-version.env`](../flatbuffers-version.env) and
builds its own `flatc`; cross builds supply a host `flatc` from the same commit.
`scripts/generate_complete_wire.py` runs `flatc` and writes the schema hash and enum lookup headers into the
build directory.

## Stream

A stream is a sequence of size-prefixed `Block`s (file identifier `CLV2`) of about 64 KiB, each holding
`Record`s. The first record is a `Header` with the schema hash, the last is `End`. Paths are interned as `File`
records. Positive node ids are dense within a translation unit; -1 to -6 are typed null references.

The entry point is `wire/v2/complete.fbs`. Its schema hash is SHA-256 over the `wire/v2/*.fbs` files sorted by
path, hashing for each file its UTF-8 path (`wire/v2/<name>`), a NUL byte, its bytes and a NUL byte.

## Naming contract with Clava

Clava binds payload tables by name at run time, so names are the contract:

- A node payload table is named after its Clava class plus `Data` (`FunctionDeclData` fills `FunctionDecl`); a
  `base` field holds the parent class's payload.
- A field is the snake case of the Clava `DataKey` name (`is_cxx_class_member` fills `isCxxClassMember`).
- Scalars are declared `= null` so absence is visible. Mark a field `(wire_optional)` only when absence is a valid
  value; Clava rejects any other absent field.
- Enums are read by ordinal; their value names must equal the Java enum constants.

## Adding or changing a node or field

1. Edit the schema under `wire/v2/`. A new node table also goes into `NodePayload` in `complete.fbs`.
2. Build the payload in the matching `src/Clava/Flat*.cpp` file. Each file ends with its family's `makeNode`
   dispatch; add a `FLAT_PAYLOAD(ClangClass, makeXData)` line there for a new Clang class.
3. Add Clang child visiting in `src/ChildrenVisitor/` for a new Clang node kind.
4. Run `test/run_tests.py` for the tool and the plugin, then the Clava parser tests against the local build.
5. Publish a release candidate tag (such as `v18.1.8_5-rc4`); the release job packages the schema and writes
   `clang-dumper-release-manifest.json`. In Clava, select the tag and refresh its schema resource (see Clava's
   `ClangAstParser/WIRE_FORMAT.md`).
