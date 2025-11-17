# Auto-generated helper (do not edit)
# Scans src/enums_cpp for cpp files produced by the Node script
file(GLOB ENUM_CPP_FILES "src/enums_cpp/*.cpp")
file(WRITE "/home/runner/work/clang-dumper/clang-dumper/_codeql_build_dir/generated/clang_enums_aggregated.cpp" "// Aggregated enum sources - generated at build time\n\n")
foreach(f IN LISTS ENUM_CPP_FILES)
  file(APPEND "/home/runner/work/clang-dumper/clang-dumper/_codeql_build_dir/generated/clang_enums_aggregated.cpp" "#include \"${f}\"\n")
endforeach()
