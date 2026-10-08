#!/usr/bin/env python3
"""Generate the native one-record protobuf adapter headers.

The protobuf compiler owns the schema.  This generator consumes protoc's
FileDescriptorSet, so the C++ temporary model and its encoder cannot silently
drift when a field or oneof is added to the .proto file.
"""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Optional
try:
    from google.protobuf import descriptor_pb2
except ImportError as exc:  # pragma: no cover - exercised by build configuration
    raise SystemExit(
        "generate_proto_adapters.py requires the standard Python protobuf "
        "runtime (protobuf 4.28.3 or a compatible distribution)"
    ) from exc


TRANSPORT_MESSAGES = {"Header", "End", "Envelope", "Chunk", "Record"}
REQUIRED_HEADER_FIELDS = {
    "protocol_major",
    "protocol_minor",
    "schema_id",
    "producer_version",
    "llvm_major",
    "schema_sha256",
}
UNION_MEMBERS = {
    "TemplateArgument": "value",
    "TemplateName": "value",
    "CXXCtorInitializer": "target",
    "ExceptionSpecification": "details",
    "OffsetOfComponent": "value",
    "Designator": "value",
    "NestedNameSpecifier": "value",
    "Node": "payload",
}
ALIASES = {
    "TemplateValue": "UnionValue",
    "TemplateNameValue": "UnionValue",
    "InitializerTarget": "UnionValue",
    "ExceptionDetails": "UnionValue",
    "OffsetValue": "UnionValue",
    "DesignatorValue": "UnionValue",
    "NestedNameValue": "UnionValue",
    "DeclPayload": "UnionValue",
    "TypePayload": "UnionValue",
    "NodePayload": "UnionValue",
}
CPP_KEYWORDS = {
    "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel",
    "atomic_commit", "atomic_noexcept", "auto", "bitand", "bitor", "bool",
    "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t",
    "class", "compl", "concept", "const", "consteval", "constexpr",
    "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield",
    "decltype", "default", "delete", "do", "double", "dynamic_cast", "else",
    "enum", "explicit", "export", "extern", "false", "float", "for", "friend",
    "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
    "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private",
    "protected", "public", "register", "reinterpret_cast", "requires", "return",
    "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
    "switch", "synchronized", "template", "this", "thread_local", "throw", "true",
    "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual",
    "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
}
SCALAR_TYPES = {
    descriptor_pb2.FieldDescriptorProto.TYPE_DOUBLE: "double",
    descriptor_pb2.FieldDescriptorProto.TYPE_FLOAT: "float",
    descriptor_pb2.FieldDescriptorProto.TYPE_INT64: "int64_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_SINT64: "int64_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_SFIXED64: "int64_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_UINT64: "uint64_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_FIXED64: "uint64_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_INT32: "int32_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_SINT32: "int32_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_SFIXED32: "int32_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_UINT32: "uint32_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_FIXED32: "uint32_t",
    descriptor_pb2.FieldDescriptorProto.TYPE_BOOL: "bool",
    descriptor_pb2.FieldDescriptorProto.TYPE_STRING: "std::string",
    descriptor_pb2.FieldDescriptorProto.TYPE_BYTES: "std::vector<uint8_t>",
}


def short_name(type_name: str) -> str:
    return type_name.rsplit(".", 1)[-1]


def cpp_field(name: str) -> str:
    return name + "_" if name in CPP_KEYWORDS else name


def is_real_oneof(message: descriptor_pb2.DescriptorProto,
                  field: descriptor_pb2.FieldDescriptorProto) -> bool:
    return field.HasField("oneof_index") and not message.oneof_decl[
        field.oneof_index
    ].name.startswith("_")


def enum_type(field: descriptor_pb2.FieldDescriptorProto) -> str:
    return short_name(field.type_name)


def field_type(field: descriptor_pb2.FieldDescriptorProto,
               for_object: bool = True) -> str:
    if field.type == field.TYPE_MESSAGE:
        return short_name(field.type_name) + "T" if for_object else short_name(field.type_name)
    if field.type == field.TYPE_ENUM:
        return enum_type(field)
    try:
        return SCALAR_TYPES[field.type]
    except KeyError as exc:
        raise ValueError(f"unsupported protobuf field type {field.type} for {field.name}") from exc


def object_field_type(field: descriptor_pb2.FieldDescriptorProto) -> str:
    value_type = field_type(field)
    if field.label == field.LABEL_REPEATED:
        if field.type == field.TYPE_MESSAGE:
            value_type = f"std::unique_ptr<{value_type}>"
        return f"std::vector<{value_type}>"
    if field.type == field.TYPE_MESSAGE:
        return f"std::unique_ptr<{value_type}>"
    return value_type


def messages(file_desc: descriptor_pb2.FileDescriptorProto) -> list[descriptor_pb2.DescriptorProto]:
    return [m for m in file_desc.message_type if m.name not in TRANSPORT_MESSAGES]


def enum_declarations(file_desc: descriptor_pb2.FileDescriptorProto) -> list[str]:
    output: list[str] = []
    for enum in file_desc.enum_type:
        values = [v for v in enum.value if not (v.number == 0 and v.name.endswith("_UNSPECIFIED"))]
        expected = list(range(1, len(values) + 1))
        actual = [v.number for v in values]
        if actual != expected:
            raise ValueError(f"{enum.name} values must be contiguous after UNSPECIFIED: {actual}")
        members = ", ".join(f"{v.name} = {index}" for index, v in enumerate(values))
        output.append(f"enum class {enum.name} : int32_t {{ {members}, }};")
    return output


def object_header(file_desc: descriptor_pb2.FileDescriptorProto) -> str:
    lines = [
        "#pragma once",
        "// Generated from protoc's FileDescriptorSet; do not edit by hand.",
        "#include <cstdint>",
        "#include <memory>",
        "#include <string>",
        "#include <string_view>",
        "#include <type_traits>",
        "#include <typeindex>",
        "#include <utility>",
        "#include <vector>",
        "",
        "namespace astwire::v1obj {",
        "",
    ]
    msg = messages(file_desc)
    for index, item in enumerate(msg):
        lines.append(f"struct {item.name}T;")
        if index + 1 == len(msg):
            lines.append("")
    lines.extend([
        "// Lightweight per-record object model used only while converting one native",
        "// record to generated protobuf classes. It is deliberately not an AST store",
        "// and is never retained as a second protobuf AST.",
        "struct UnionValue {",
        "  std::shared_ptr<void> object;",
        "  std::type_index type = typeid(void);",
        "  template <class T> void Set(T &&value) {",
        "    using U = std::decay_t<T>;",
        "    object = std::make_shared<U>(std::forward<T>(value));",
        "    type = typeid(U);",
        "  }",
        "  template <class T> const T *get() const {",
        "    return type == typeid(T) ? static_cast<const T *>(object.get()) : nullptr;",
        "  }",
        "};",
        "",
    ])
    lines.extend(enum_declarations(file_desc))
    lines.append("")
    for item in msg:
        lines.append(f"struct {item.name}T {{")
        for field in item.field:
            if not is_real_oneof(item, field):
                lines.append(f"  {object_field_type(field)} {cpp_field(field.name)}{{}};")
        real_oneofs = [
            oneof.name for oneof in item.oneof_decl if not oneof.name.startswith("_")
        ]
        if real_oneofs:
            if len(real_oneofs) != 1:
                raise ValueError(f"{item.name} has multiple real oneofs")
            member = UNION_MEMBERS.get(item.name, "value")
            lines.append(f"  UnionValue {member};")
        lines.extend(["};", ""])
    for alias, target in ALIASES.items():
        lines.append(f"using {alias} = {target};")
    lines.extend(["", "} // namespace astwire::v1obj", ""])
    return "\n".join(lines)


def setter(field: descriptor_pb2.FieldDescriptorProto, source: str, destination: str) -> str:
    name = cpp_field(field.name)
    if field.type == field.TYPE_ENUM:
        value = f"static_cast<pb::{enum_type(field)}>(static_cast<int>({source}) + 1)"
    else:
        value = source
    if field.type == field.TYPE_BYTES:
        value = f"{source}.data(), {source}.size()"
    return f"{destination}->set_{name}({value});"


def add_repeated(field: descriptor_pb2.FieldDescriptorProto, source: str,
                 destination: str) -> str:
    if field.type == field.TYPE_ENUM:
        source = f"static_cast<pb::{enum_type(field)}>(static_cast<int>({source}) + 1)"
    if field.type == field.TYPE_BYTES:
        source = f"{source}.data(), {source}.size()"
    return f"{destination}->add_{cpp_field(field.name)}({source});"


def encode_body(message: descriptor_pb2.DescriptorProto) -> list[str]:
    lines: list[str] = []
    real_oneof = {
        field.name: message.oneof_decl[field.oneof_index].name
        for field in message.field
        if is_real_oneof(message, field)
    }
    for field in message.field:
        if field.name in real_oneof:
            continue
        source = f"src.{cpp_field(field.name)}"
        if field.label == field.LABEL_REPEATED:
            lines.append(f"  for (const auto &value : {source}) {{")
            if field.type == field.TYPE_MESSAGE:
                lines.append(f"    if (value) encode(*value, dst->add_{cpp_field(field.name)}());")
            else:
                lines.append(f"    {add_repeated(field, 'value', 'dst')}")
            lines.append("  }")
        elif field.type == field.TYPE_MESSAGE:
            lines.append(f"  if ({source}) encode(*{source}, dst->mutable_{cpp_field(field.name)}());")
        else:
            lines.append(f"  {setter(field, source, 'dst')}")
    if real_oneof:
        member = UNION_MEMBERS.get(message.name, "value")
        for field in message.field:
            if field.name not in real_oneof:
                continue
            if field.type == field.TYPE_MESSAGE:
                value_type = f"obj::{short_name(field.type_name)}T"
                action = f"encode(*value, dst->mutable_{cpp_field(field.name)}());"
            else:
                value_type = f"obj::{field_type(field)}"
                if field.type == field.TYPE_ENUM:
                    action = f"dst->set_{cpp_field(field.name)}(static_cast<pb::{enum_type(field)}>(static_cast<int>(*value) + 1));"
                elif field.type == field.TYPE_BYTES:
                    action = f"dst->set_{cpp_field(field.name)}(value->data(), value->size());"
                else:
                    action = f"dst->set_{cpp_field(field.name)}(*value);"
            lines.append(
                f"  if (const auto *value = src.{member}.get<{value_type}>()) {action}"
            )
    return lines


def destination_method(field: descriptor_pb2.FieldDescriptorProto) -> str:
    """Return the generated protobuf mutator used for a descriptor field."""
    name = cpp_field(field.name)
    if field.label == field.LABEL_REPEATED:
        return f"add_{name}"
    if field.type == field.TYPE_MESSAGE:
        return f"mutable_{name}"
    return f"set_{name}"


def validate_encoder_coverage(file_desc: descriptor_pb2.FileDescriptorProto) -> None:
    """Ensure every descriptor field has a generated destination mutator."""
    for message in messages(file_desc):
        body = "\n".join(encode_body(message))
        missing = [
            field.name
            for field in message.field
            if f"{destination_method(field)}(" not in body
        ]
        if missing:
            raise ValueError(
                f"encoder has no destination mutator for {message.name}: "
                + ", ".join(missing)
            )

    record = next(message for message in file_desc.message_type if message.name == "Record")
    envelope = encoder_header(file_desc)
    missing_record = [
        field.name
        for field in record.field
        if f"mutable_{cpp_field(field.name)}(" not in envelope
    ]
    if missing_record:
        raise ValueError(
            "record envelope has no adapter for: " + ", ".join(missing_record)
        )


def encoder_header(file_desc: descriptor_pb2.FileDescriptorProto) -> str:
    msg = messages(file_desc)
    lines = [
        "#pragma once",
        "// Generated from protoc's FileDescriptorSet; do not edit by hand.",
        '#include "clava_ast_wire.pb.h"',
        '#include "ProtoObjects.h"',
        "#include <stdexcept>",
        "#include <string>",
        "",
        "namespace clava::proto {",
        "namespace pb = ::astwire::v1;",
        "namespace obj = ::astwire::v1obj;",
        "",
    ]
    for item in msg:
        lines.append(f"inline void encode(const obj::{item.name}T &src, pb::{item.name} *dst);")
    lines.append("")
    for item in msg:
        lines.append(f"inline void encode(const obj::{item.name}T &src, pb::{item.name} *dst) {{")
        lines.extend(encode_body(item))
        lines.extend(["}", ""])
    lines.extend([
        "template <typename T> inline void setRecord(const T &, pb::Record *) {",
        "  static_assert(sizeof(T) == 0, \"No protobuf record mapping for this object\");",
        "}",
    ])
    record = next(item for item in file_desc.message_type if item.name == "Record")
    for field in record.field:
        target = short_name(field.type_name)
        lines.extend([
            f"inline void setRecord(const obj::{target}T &src, pb::Record *dst) {{",
            f"  encode(src, dst->mutable_{cpp_field(field.name)}());",
            "}",
        ])
    lines.extend(["", "} // namespace clava::proto", ""])
    return "\n".join(lines)


def read_descriptor(path: Path) -> descriptor_pb2.FileDescriptorProto:
    descriptor_set = descriptor_pb2.FileDescriptorSet()
    descriptor_set.ParseFromString(path.read_bytes())
    if len(descriptor_set.file) != 1:
        raise ValueError(f"expected one file descriptor, got {len(descriptor_set.file)}")
    return descriptor_set.file[0]


def validate_descriptor(file_desc: descriptor_pb2.FileDescriptorProto,
                        native_source: Optional[Path]) -> None:
    by_name = {message.name: message for message in file_desc.message_type}
    missing = TRANSPORT_MESSAGES - by_name.keys()
    if missing:
        raise ValueError(f"missing transport messages: {sorted(missing)}")
    header = by_name["Header"]
    header_fields = {field.name: field for field in header.field}
    missing_header = REQUIRED_HEADER_FIELDS - header_fields.keys()
    if missing_header:
        raise ValueError(f"missing mandatory header fields: {sorted(missing_header)}")
    for name in REQUIRED_HEADER_FIELDS:
        field = header_fields[name]
        if not field.HasField("oneof_index"):
            raise ValueError(f"mandatory header field lacks explicit presence: {name}")

    for message in messages(file_desc):
        real_oneofs = [
            oneof for oneof in message.oneof_decl if not oneof.name.startswith("_")
        ]
        if len(real_oneofs) > 1:
            raise ValueError(f"{message.name} has multiple real oneofs")
        for field in message.field:
            if is_real_oneof(message, field) and field.label == field.LABEL_REPEATED:
                raise ValueError(f"repeated field in oneof: {message.name}.{field.name}")
            field_type(field)
    record = by_name["Record"]
    record_types = {short_name(field.type_name) for field in record.field}
    if record_types - {message.name for message in messages(file_desc)}:
        raise ValueError("Record contains an adapter-less message")

    # These are the fields that make a stream self-describing.  Keep this
    # check next to the descriptor validation so a new header cannot be added
    # without updating the native writer and its focused decoder test.
    if native_source is not None:
        source = native_source.read_text(encoding="utf-8")
        missing_native = {
            field for field in REQUIRED_HEADER_FIELDS
            if f"header->set_{cpp_field(field)}(" not in source
        }
        if missing_native:
            raise ValueError(
                "native writer does not set mandatory header fields: "
                + ", ".join(sorted(missing_native))
            )

        # Header and End are the only non-record messages constructed by the
        # native stream writer.  Every field in both messages is intentional
        # protocol metadata, so adding one without an extraction assignment is
        # a schema change that must fail the build.
        for message_name, prefix in (("Header", "header"), ("End", "end")):
            message = by_name[message_name]
            missing = {
                field.name
                for field in message.field
                if f"{prefix}->set_{cpp_field(field.name)}(" not in source
            }
            if missing:
                raise ValueError(
                    f"native writer does not set {message_name} fields: "
                    + ", ".join(sorted(missing))
                )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--descriptor", type=Path, required=True)
    parser.add_argument("--objects", type=Path, required=True)
    parser.add_argument("--encode", type=Path, required=True)
    parser.add_argument("--native-source", type=Path)
    args = parser.parse_args()
    file_desc = read_descriptor(args.descriptor)
    validate_descriptor(file_desc, args.native_source)
    validate_encoder_coverage(file_desc)
    args.objects.parent.mkdir(parents=True, exist_ok=True)
    args.encode.parent.mkdir(parents=True, exist_ok=True)
    args.objects.write_text(object_header(file_desc), encoding="utf-8")
    args.encode.write_text(encoder_header(file_desc), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
