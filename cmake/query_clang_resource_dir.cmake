function(clang_dumper_query_clang_resource_dir clang_path expected_major output_variable)
    if(NOT clang_path)
        message(FATAL_ERROR "A host Clang executable is required to query resource headers")
    endif()

    execute_process(
        COMMAND "${clang_path}" -dumpversion
        RESULT_VARIABLE _clang_version_result
        OUTPUT_VARIABLE _clang_version
        ERROR_VARIABLE _clang_version_error
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _clang_version_result EQUAL 0)
        message(FATAL_ERROR "Cannot query host Clang version from ${clang_path}: ${_clang_version_error}")
    endif()
    string(REGEX MATCH "^[0-9]+" _clang_major "${_clang_version}")
    if(NOT "${_clang_major}" STREQUAL "${expected_major}")
        message(FATAL_ERROR
            "Host Clang ${clang_path} reports version '${_clang_version}', expected LLVM ${expected_major}")
    endif()

    execute_process(
        COMMAND "${clang_path}" -print-resource-dir
        RESULT_VARIABLE _resource_dir_result
        OUTPUT_VARIABLE _resource_dir
        ERROR_VARIABLE _resource_dir_error
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _resource_dir_result EQUAL 0 OR NOT IS_DIRECTORY "${_resource_dir}")
        message(FATAL_ERROR
            "Cannot query host Clang resource directory from ${clang_path}: ${_resource_dir_error}")
    endif()
    get_filename_component(_resource_version "${_resource_dir}" NAME)
    string(REGEX MATCH "^[0-9]+" _resource_major "${_resource_version}")
    if(NOT "${_resource_major}" STREQUAL "${expected_major}")
        message(FATAL_ERROR
            "Host Clang ${clang_path} reports resource directory ${_resource_dir}, expected LLVM ${expected_major}")
    endif()
    if(NOT EXISTS "${_resource_dir}/include/stdarg.h")
        message(FATAL_ERROR "Host Clang resource headers are missing from ${_resource_dir}")
    endif()

    set(${output_variable} "${_resource_dir}" PARENT_SCOPE)
endfunction()
