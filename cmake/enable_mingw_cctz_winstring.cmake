function(clang_dumper_enable_mingw_cctz_winstring source_path target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "CCTZ winstring include target does not exist: ${target}")
    endif()
    if(NOT EXISTS "${source_path}")
        message(FATAL_ERROR "CCTZ source requiring winstring.h was not found: ${source_path}")
    endif()

    set_property(SOURCE "${source_path}" TARGET_DIRECTORY "${target}" APPEND
        PROPERTY COMPILE_OPTIONS "-include;winstring.h")
endfunction()
