if(NOT DEFINED ENUMS_SOURCE_DIR)
    message(FATAL_ERROR "ENUMS_SOURCE_DIR is required")
endif()

if(NOT DEFINED ENUMS_AGGREGATED_SRC)
    message(FATAL_ERROR "ENUMS_AGGREGATED_SRC is required")
endif()

file(GLOB ENUM_CPP_FILES "${ENUMS_SOURCE_DIR}/enums_cpp/*.cpp")
list(SORT ENUM_CPP_FILES)

file(WRITE "${ENUMS_AGGREGATED_SRC}" "// Aggregated enum sources - generated at build time\n\n")
foreach(enum_cpp IN LISTS ENUM_CPP_FILES)
    file(APPEND "${ENUMS_AGGREGATED_SRC}" "#include \"${enum_cpp}\"\n")
endforeach()
