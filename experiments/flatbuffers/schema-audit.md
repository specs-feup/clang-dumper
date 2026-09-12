# v2 schema and native audit

Audit date: 2026-09-11. The comparison covers the current `wire/v2` schemas,
the Java `DataKey` declarations under
`Clava: ClavaAst/src`,
the parsers and wire readers under
`Clava: ClangAstParser`,
and the old native dumpers.

## Mappings that agree

- Every schema `java_key` resolves to a real Java `DataKey`; there are no
  schema annotations pointing at missing classes or constants. Keys for Clava
  context state, comments, translation-unit bookkeeping, transforms, and
  other non-dumper state are outside this wire scope.
- Primitive widths match the declared Java types. Java `Integer` fields use
  FlatBuffers `int`, Java `Long` fields use `long`, Java `Double` fields use
  `double`, and booleans use `bool`. `CharacterLiteral.VALUE` and
  `StringLiteral.LENGTH` are correctly `long`; `StringLiteral.CHAR_BYTE_WIDTH`
  and all parameter/depth/index/count fields are correctly `int`.
- All `java_ref` annotations match the parser queues: `node`, `optional`,
  `nullable`, and `list`. This includes the intentionally different
  `VariableArrayType.SIZE_EXPR` (`nullable`, `wire/v2/types.fbs:71`) and
  `DependentSizedArrayType.SIZE_EXPR` (`optional`, `wire/v2/types.fbs:76`).
- Every annotated enum field has the Java enum type named by its annotation,
  including list enums. The constants and ordinals in `wire/v2/enums.fbs` also
  match the Java enum sources. This explicitly includes the legacy numeric
  `NameKind` mapping and the new `EnumScopeType` values.
- `TypeOfExprType.IS_SUGARED` at `wire/v2/types.fbs:146` uses
  `DecltypeType.IS_SUGARED` intentionally. The current parser writes that key
  for `TypeOfExprType` at
  `Clava: ClangAstParser/src/pt/up/fe/specs/clang/parsers/data/TypeDataParser.java:345-350`.
  Likewise, `CXXMethodDecl.RECORD_ID` is represented as the `java_ref_alias`
  on `wire/v2/decls.fbs:77`; the old parser reads the record ID and then queues
  `RECORD` at `DeclDataParser.java:319-326`.

## Native findings rechecked

The three native hazards from the previous audit are now fixed in the current
checkout:

- `src/Clava/FlatExpressions.cpp:401-407` guards
  `getPartialArguments()` with `isPartiallySubstituted()`, matching Clang's
  assertion at `/usr/lib/llvm-18/include/clang/AST/ExprCXX.h:4311-4314` and the
  old dumper's guarded branch at
  `src/ClavaDataDumper/ClavaDataDumperStmts.cpp:556-570`.
- `src/Clava/FlatExpressions.cpp:423-428` filters synthetic anonymous-field
  designators using the same predicate as
  `src/ClavaDataDumper/ClavaDataDumperStmts.cpp:584-604`.
- `src/Clava/FlatExpressions.cpp:468-471` maps
  `PseudoObjectExpr::NoResult` explicitly to `-1`, matching
  `src/ClavaDataDumper/ClavaDataDumperStmts.cpp:627-637` and the `int` field at
  `wire/v2/expressions.fbs:255`.

No remaining native payload mismatch was found in this pass.

## Importer and compound mappings rechecked

- **MSProperty optional strings are correct.** The builder's absent value is an
  empty `std::string` at `src/Clava/FlatDecls.cpp:401-409`. Object API packing
  turns that into a null string offset at
  `build/wire-v2/decls_generated.h:5638-5649`, and the generated Java accessor
  returns `null` when the offset is absent
  (`Clava: ClangAstParser/build/generated/sources/wire/astwire/v2/MSPropertyDeclData.java:31-35`).
  The generated binding uses `Optional.ofNullable` in
  `build/wire-v2-java/pt/up/fe/specs/clang/wire/GeneratedNodes.java:39`, so
  the Java value is `Optional.empty()`. The earlier concern was false.
- **Language side records are converted correctly.**
  `Clava: ClangAstParser/src/pt/up/fe/specs/clang/wire/CompleteReader.java:248-294`
  maps raw OpenCL values `0/100/110/120/200` to `OpenCLVersion`, rejects other
  values, and uses `Math.toIntExact` for all Java `Integer` width keys. The
  `uint` fields in `wire/v2/records.fbs:30,36-44` therefore do not leak as Java
  `long` values.
- **Template integral compounds are converted correctly.**
  `CompoundReader.java:60-63` parses the native decimal string with
  `BigInteger.intValueExact()` before setting Java's
  `TemplateArgumentIntegral.INTEGRAL`. This preserves the Java `Integer`
  contract while rejecting values outside its range, as the legacy
  `LineStreamParsers.integer` parser did.
- **Aligned attributes are converted correctly.** The combined wire fields in
  `wire/v2/attributes.fbs:11` are expanded into the Java kind and subtype keys
  by `SchemaRuntime.java:126-133`, including optional expression references.

The source paths above are all from the current `ast-flatbuffers` checkout,
not the older `dumper-v3` sibling checkout. The old parser's custom MSProperty
sentinel behavior remains documented at
`Clava: ClangAstParser/src/pt/up/fe/specs/clang/parsers/data/ClavaDataParsers.java:507-510`,
but it is not the path used by the v2 reader for absent names.

## Verification

The schema-key, parser-reference, primitive-width, enum-type, and enum-ordinal
checks were rerun against `the ast-flatbuffers workspace`
and found no additional annotated-field mismatches. After the native fixes and
the importer checks above, this audit has no unresolved schema/native mapping
issue.
