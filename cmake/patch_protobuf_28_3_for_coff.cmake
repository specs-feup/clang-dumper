if(NOT DEFINED PROTOBUF_SOURCE_DIR OR NOT DEFINED PROTOBUF_SOURCE_VERSION)
    message(FATAL_ERROR "Protobuf COFF patch requires PROTOBUF_SOURCE_DIR and PROTOBUF_SOURCE_VERSION")
endif()
if(NOT "${PROTOBUF_SOURCE_VERSION}" VERSION_EQUAL "28.3")
    message(FATAL_ERROR
        "The Windows COFF workaround is audited only for Protobuf 28.3; review it before changing the pinned version")
endif()

set(_port_def "${PROTOBUF_SOURCE_DIR}/src/google/protobuf/port_def.inc")
if(NOT EXISTS "${_port_def}")
    message(FATAL_ERROR "Pinned Protobuf port_def.inc was not found: ${_port_def}")
endif()
file(READ "${_port_def}" _port_def_contents)

set(_unpatched_guard [=[#if defined(__GNUC__) && defined(__clang__) && !defined(__APPLE__) && \
    !defined(_MSC_VER)
#define PROTOBUF_DESCRIPTOR_WEAK_MESSAGES_ALLOWED
]=])
set(_patched_guard [=[#if defined(__GNUC__) && defined(__clang__) && !defined(__APPLE__) && \
    !defined(_MSC_VER) && !defined(_WIN32)
#define PROTOBUF_DESCRIPTOR_WEAK_MESSAGES_ALLOWED
]=])

string(FIND "${_port_def_contents}" "${_unpatched_guard}" _unpatched_index)
if(NOT _unpatched_index EQUAL -1)
    string(REPLACE "${_unpatched_guard}" "${_patched_guard}"
        _patched_port_def_contents "${_port_def_contents}")
    file(WRITE "${_port_def}" "${_patched_port_def_contents}")
    message(STATUS "Disabled Protobuf 28.3 ELF weak-descriptor sentinels for Windows COFF")
    return()
endif()

string(FIND "${_port_def_contents}" "${_patched_guard}" _patched_index)
if(NOT _patched_index EQUAL -1)
    message(STATUS "Protobuf 28.3 Windows COFF weak-descriptor patch is already applied")
    return()
endif()

message(FATAL_ERROR
    "The Protobuf 28.3 weak-descriptor feature guard changed; review the Windows COFF workaround before continuing")
