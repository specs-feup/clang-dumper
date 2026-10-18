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
build directory, together with the binary schema `complete.bfbs`. Releases and local build manifests publish it as
`clang-dumper-wire-schema-v2.bfbs`; Clava's build generates its node data classes from it.

## Stream

A stream is a sequence of size-prefixed `Block`s (file identifier `CLV2`) of about 64 KiB, each holding
`Record`s. The first record is a `Header` with the schema hash, the last is `End`. Paths are interned as `File`
records. Positive node ids are dense within a translation unit; -1 to -6 are typed null references.

The entry point is `wire/v2/complete.fbs`. Its schema hash is SHA-256 over the `wire/v2/*.fbs` files sorted by
path, hashing for each file its UTF-8 path (`wire/v2/<name>`), a NUL byte, its bytes and a NUL byte.

## The schema drives both sides

Node payload tables carry the attributes that generate the dumper's payload builders
(`scripts/generate_builders.py`, into `<family>_builders.inc`) and Clava's Java data classes, so a field is added
or changed in the schema only.

- A node payload table is named after its Clava class plus `Data` (`FunctionDeclData` is the data of
  `FunctionDecl`), and its `base` field holds the parent class's payload.
- A field's Clang value is, by convention, `node->isBar()` for `is_bar`, `node->hasBar()` for `has_bar` and
  `node->getBar()` otherwise, converted by the field's type. `clang: "<expr>"` gives another Clang value and
  `clang_value: "<expr>"` the final field value (helpers live in the family's `src/Clava/Flat*.cpp`). Enums go
  through the `clava::<ENUM_NAME>` name table; `clang_enum` names another.
- Node references are `long` ids typed by `ref: "<ClavaClass>"`; `optional` makes the Java value an `Optional`
  (for strings too, where empty means absent) and `nullable` turns a null reference into a typed null node.
- A table's Clang class is its name without `Data`; `clang_class` names another. `clang_manual` keeps a
  hand-written builder in the family's `.cpp` file.
- `clang_root` marks a family's root table, which also gets the family's `makeNode`: nodes get the table of their
  Clang class, or of a table listing their class in `clang_nodes`, and otherwise the root's payload.
- Document fields with `///` comments; they become the Javadoc of the generated Java getters.
- Scalars are declared `= null` so absence is visible. Mark a field `(wire_optional)` only when absence is a valid
  value; Clava rejects any other absent field.
- Enum values are matched to Java enum constants by name.

## Adding or changing a node or field

1. Edit the schema under `wire/v2/`. A new node table also goes into `NodePayload` in `complete.fbs`; a new
   family file goes into `GENERATED_FAMILIES` in `scripts/generate_complete_wire.py`.
2. Add `clang`/`clang_value` attributes where the convention does not hold, and helpers for values that need code.
3. Add Clang child visiting in `src/ChildrenVisitor/` for a new Clang node kind.
4. Run `test/run_tests.py` for the tool and the plugin, then the Clava parser tests against the local build.
5. Publish a release candidate tag (such as `v18.1.8_7-rc4`); the release job packages the schema and writes
   `clang-dumper-release-manifest.json`. In Clava, select the tag in `clang-dumper-release.tag`; its build
   regenerates the data classes, and code using a changed field no longer compiles until it is updated.
