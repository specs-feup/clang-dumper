cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/read_llvm_version.cmake")
clang_dumper_load_llvm_version("${CMAKE_CURRENT_LIST_DIR}/llvm-version.env")
set(LLVM_WINDOWS_ROOT "" CACHE PATH "Windows-target LLVM/Clang SDK root for cross-compilation")

# CMake chooses compilers during project(), so default compiler discovery must
# happen before that call. Users and CI can still override this with CC/CXX or
# -DCMAKE_*_COMPILER.
if(NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED ENV{CC} AND UNIX AND NOT APPLE AND NOT CMAKE_TOOLCHAIN_FILE)
    find_program(_CLANG_DUMPER_DEFAULT_CC NAMES clang-${CLANG_VERSION} clang)
    if(_CLANG_DUMPER_DEFAULT_CC)
        set(CMAKE_C_COMPILER "${_CLANG_DUMPER_DEFAULT_CC}" CACHE FILEPATH "" FORCE)
    endif()
endif()
if(NOT DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX} AND UNIX AND NOT APPLE AND NOT CMAKE_TOOLCHAIN_FILE)
    find_program(_CLANG_DUMPER_DEFAULT_CXX NAMES clang++-${CLANG_VERSION} clang++)
    if(_CLANG_DUMPER_DEFAULT_CXX)
        set(CMAKE_CXX_COMPILER "${_CLANG_DUMPER_DEFAULT_CXX}" CACHE FILEPATH "" FORCE)
    endif()
endif()

project(ClangAstDumper LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32 AND CMAKE_CROSSCOMPILING)
    if(NOT LLVM_WINDOWS_ROOT)
        message(FATAL_ERROR "LLVM_WINDOWS_ROOT is required for Windows cross-compilation")
    endif()
elseif(WIN32)
    message(FATAL_ERROR "Native Windows builds are no longer supported. Build Windows tools from Linux with scripts/build_windows_cross.sh.")
endif()

message(STATUS "Using C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Using C++ compiler: ${CMAKE_CXX_COMPILER}")

# Set libraries
if(WIN32 AND CMAKE_CROSSCOMPILING)
    set(LLVM_DIR "${LLVM_WINDOWS_ROOT}/lib/cmake/llvm")
    set(Clang_DIR "${LLVM_WINDOWS_ROOT}/lib/cmake/clang")
elseif(APPLE)
    if(NOT DEFINED LLVM_PREFIX AND DEFINED ENV{LLVM_PREFIX})
        set(LLVM_PREFIX "$ENV{LLVM_PREFIX}")
    endif()
    if(NOT DEFINED LLVM_PREFIX)
        find_program(BREW_EXE brew)
        if(BREW_EXE)
            execute_process(
                COMMAND "${BREW_EXE}" --prefix "llvm@${CLANG_VERSION}"
                OUTPUT_VARIABLE LLVM_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
        endif()
    endif()
    if(LLVM_PREFIX)
        set(Clang_DIR "${LLVM_PREFIX}/lib/cmake/clang" CACHE PATH "Path to ClangConfig.cmake")
        set(LLVM_DIR "${LLVM_PREFIX}/lib/cmake/llvm" CACHE PATH "Path to LLVMConfig.cmake")
    endif()
else()
    if(NOT DEFINED Clang_DIR)
        set(Clang_DIR "/usr/lib/cmake/clang-${CLANG_VERSION}" CACHE PATH "Path to ClangConfig.cmake")
    endif()
    if(NOT DEFINED LLVM_DIR)
        set(LLVM_DIR "/usr/lib/llvm-${CLANG_VERSION}/lib/cmake/llvm" CACHE PATH "Path to LLVMConfig.cmake")
    endif()
endif()

message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

add_compile_options(-Wsuggest-override)

if(WIN32 AND CMAKE_CROSSCOMPILING)
    if(NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB INTERFACE IMPORTED)
        message(STATUS "Windows cross build: skipping host ZLIB discovery")
    endif()
else()
    find_package(ZLIB REQUIRED)
endif()
# Some distributions ship LLVM with a LibEdit dependency (LLVMLineEditor target)
# but do not provide a LibEdit::LibEdit CMake target unless the user installs
# an extra FindLibEdit module. Create an imported interface target if missing
# so that LLVM's exported targets referencing LibEdit::LibEdit do not break
# configuration. We try to locate the actual 'edit' library; if not found we
# still create an empty interface target (rare case) which is usually fine if
# we never link LLVMLineEditor into our final binaries.
if(NOT TARGET LibEdit::LibEdit)
    find_library(LIBEDIT_LIB NAMES edit)
    if(LIBEDIT_LIB)
        add_library(LibEdit::LibEdit INTERFACE IMPORTED)
        set_target_properties(LibEdit::LibEdit PROPERTIES INTERFACE_LINK_LIBRARIES "${LIBEDIT_LIB}")
        message(STATUS "Found libedit: ${LIBEDIT_LIB}; created LibEdit::LibEdit target")
    else()
        add_library(LibEdit::LibEdit INTERFACE IMPORTED)
        message(WARNING "libedit not found; created empty LibEdit::LibEdit target. If link errors referencing edit/readline occur, install libedit-dev.")
    endif()
endif()
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# Set the LLVM and Clang include directories as SYSTEM to suppress warnings
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
include_directories(SYSTEM ${CLANG_INCLUDE_DIRS})

# Add definitions needed for LLVM and Clang
add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})

# -----------------------------------------------------------------------------
# Generated Clang Enum Sources (build-time, not configure-time)
# -----------------------------------------------------------------------------
# We previously globbed the generated files at configure time and injected all
# of them as separate translation units. This forced us to know their names
# during configuration and caused repeated re-configurations / stale lists.
#
# New approach:
# 1. A custom command (build step) runs the Python script that (re)generates the
#    enum sources under src/enums_cpp/.
# 2. Immediately after, a small CMake script (executed with -P at build time)
#    scans the directory and writes a single aggregator translation unit
#    (clang_enums_aggregated.cpp) which simply #includes every generated
#    enums_*.cpp file. This file has a stable name so it can be listed in the
#    targets without hardcoding any of the dynamic filenames.
# 3. Targets only compile this single aggregator TU; individual generated
#    .cpp files are included via the preprocessor and do not need to be
#    enumerated in CMake.
#
# This satisfies:
#  - No hardcoding of the generated enum file names.
#  - Generation happens strictly at build time (not configuration).
#  - Adding/removing enum files requires no CMake reconfigure.
#
# When SKIP_ENUM_GENERATION is ON (e.g., in CI where enums are pre-generated),
# the Python script is skipped and only the aggregator runs.
# -----------------------------------------------------------------------------

# Find Python3 for enum generation
find_package(Python3 COMPONENTS Interpreter REQUIRED)

option(SKIP_ENUM_GENERATION "Skip Python enum generation (use pre-existing files)" OFF)
set(CLANG_ENUMS_HOST_CLANG "" CACHE FILEPATH "Host clang++ executable for enum preprocessing")
if(NOT CLANG_ENUMS_HOST_CLANG)
    find_program(CLANG_ENUMS_HOST_CLANG_AUTO NAMES clang++-${CLANG_VERSION} clang++)
    if(CLANG_ENUMS_HOST_CLANG_AUTO)
        set(CLANG_ENUMS_HOST_CLANG "${CLANG_ENUMS_HOST_CLANG_AUTO}" CACHE FILEPATH "Host clang++ executable for enum preprocessing" FORCE)
    endif()
endif()
set(CLANG_ENUMS_HOST_CLANG_ARG "")
if(CLANG_ENUMS_HOST_CLANG)
    set(CLANG_ENUMS_HOST_CLANG_ARG --clang-executable "${CLANG_ENUMS_HOST_CLANG}")
endif()
set(CLANG_ENUMS_EXTRA_CLANG_ARGS "")
if(WIN32 AND CMAKE_CROSSCOMPILING)
    list(APPEND CLANG_ENUMS_EXTRA_CLANG_ARGS
        "--clang-arg=--target=${CROSS_PREFIX}"
        "--clang-arg=--sysroot=${LLVM_WINDOWS_ROOT}"
        "--clang-arg=-resource-dir=${LLVM_WINDOWS_ROOT}/lib/clang/${CLANG_VERSION}"
        "--clang-arg=-stdlib=libc++"
        "--clang-arg=-nostdinc++"
        "--clang-arg=-isystem"
        "--clang-arg=${LLVM_WINDOWS_ROOT}/include/c++/v1"
    )
endif()

# Directory for build-time helper + aggregated source
set(ENUMS_GEN_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${ENUMS_GEN_DIR}")
set(ENUMS_AGGREGATED_SRC "${ENUMS_GEN_DIR}/clang_enums_aggregated.cpp")
set(GENERATE_ENUMS_AGG_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/generate_enum_aggregator.cmake")

# Collect Python generation script files as (re)build dependencies (best-effort).
file(GLOB PYTHON_ENUM_SCRIPTS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/clang_enums/*.py")
file(GLOB ENUM_CPP_INPUTS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/enums_cpp/*.cpp")

if(SKIP_ENUM_GENERATION)
    # Skip Python generation, only run aggregator on pre-existing enum files
    message(STATUS "SKIP_ENUM_GENERATION is ON: using pre-existing enum files in src/enums_cpp/")
    add_custom_command(
        OUTPUT "${ENUMS_AGGREGATED_SRC}"
        COMMAND ${CMAKE_COMMAND} -E echo "Aggregating pre-existing enum sources..."
        COMMAND ${CMAKE_COMMAND}
            -DENUMS_SOURCE_DIR=${CMAKE_SOURCE_DIR}/src
            -DENUMS_AGGREGATED_SRC=${ENUMS_AGGREGATED_SRC}
            -P "${GENERATE_ENUMS_AGG_SCRIPT}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS "${GENERATE_ENUMS_AGG_SCRIPT}" ${ENUM_CPP_INPUTS}
        COMMENT "Aggregate pre-existing enum sources"
        VERBATIM
    )
else()
    # Full generation: run Python script then aggregator
    add_custom_command(
        OUTPUT "${ENUMS_AGGREGATED_SRC}"
        COMMAND ${CMAKE_COMMAND} -E echo "Generating enums (Python) into src/ ..."
        COMMAND ${Python3_EXECUTABLE} -m clang_enums ${LLVM_DIR} src/ ${CLANG_ENUMS_HOST_CLANG_ARG} ${CLANG_ENUMS_EXTRA_CLANG_ARGS}
        COMMAND ${CMAKE_COMMAND}
            -DENUMS_SOURCE_DIR=${CMAKE_SOURCE_DIR}/src
            -DENUMS_AGGREGATED_SRC=${ENUMS_AGGREGATED_SRC}
            -P "${GENERATE_ENUMS_AGG_SCRIPT}"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        DEPENDS "${GENERATE_ENUMS_AGG_SCRIPT}" ${PYTHON_ENUM_SCRIPTS} ${ENUM_CPP_INPUTS}
        COMMENT "Run enum generator and aggregate sources"
        VERBATIM
    )
endif()

add_custom_target(clang_enums DEPENDS "${ENUMS_AGGREGATED_SRC}")

set(SOURCE_FILES
    src/Clang/ClangAst.cpp
    src/Clang/ClangNodes.cpp
    src/ClangEnums/ClangEnums.cpp
    src/TypeMarker/TypeMarker.cpp
    src/Clava/DumpStream.cpp
    src/Clava/HandlerCoverage.cpp
    src/ClavaDataDumper/ClavaDataDumper.cpp
    src/ClavaDataDumper/ClavaDataDumperAttrs.cpp
    src/ClavaDataDumper/ClavaDataDumperDecls.cpp
    src/ClavaDataDumper/ClavaDataDumperStmts.cpp
    src/ClavaDataDumper/ClavaDataDumperTypes.cpp
    src/ChildrenVisitor/ChildrenVisitorAttrs.cpp
    src/ChildrenVisitor/ChildrenVisitorDecls.cpp
    src/ChildrenVisitor/ChildrenVisitorExtras.cpp
    src/ChildrenVisitor/ChildrenVisitorStmts.cpp
    src/ChildrenVisitor/ChildrenVisitorTypes.cpp
    src/ClangAstDumper/ClangAstDumper.cpp
    src/ClangAstDumper/ClangAstDumperAttr.cpp
    src/ClangAstDumper/ClangAstDumperDecls.cpp
    src/ClangAstDumper/ClangAstDumperExprs.cpp
    src/ClangAstDumper/ClangAstDumperExtra.cpp
    src/ClangAstDumper/ClangAstDumperStmts.cpp
    src/ClangAstDumper/ClangAstDumperTypes.cpp
    "${ENUMS_AGGREGATED_SRC}"
    )

if(NOT (WIN32 AND CMAKE_CROSSCOMPILING))
    add_library(plugin MODULE ${SOURCE_FILES} src/plugin.cpp)
    target_compile_features(plugin PRIVATE cxx_std_20)
    add_dependencies(plugin clang_enums)
endif()

add_executable(tool ${SOURCE_FILES} src/tool.cpp)
target_compile_features(tool PRIVATE cxx_std_20)
add_dependencies(tool clang_enums)

target_compile_options(tool PRIVATE -ffunction-sections -fdata-sections)

function(clang_dumper_find_clang_archives out_var llvm_libdir)
    set(CLANG_STATIC_COMPONENTS
        clangTooling
        clangToolingCore
        clangFrontend
        clangFrontendTool
        clangSerialization
        clangDriver
        clangParse
        clangSema
        clangAnalysis
        clangASTMatchers
        clangAST
        clangLex
        clangBasic
        clangEdit
        clangRewrite
        clangRewriteFrontend
        clangAPINotes
        clangSupport
    )
    set(CLANG_ARCHIVES "")
    foreach(component IN LISTS CLANG_STATIC_COMPONENTS)
        unset(CLANG_ARCHIVE)
        unset(CLANG_ARCHIVE CACHE)
        find_library(CLANG_ARCHIVE
            NAMES "${component}"
            PATHS "${llvm_libdir}"
            NO_DEFAULT_PATH
        )
        if(NOT CLANG_ARCHIVE)
            message(FATAL_ERROR "Required static Clang archive ${component} was not found in ${llvm_libdir}")
        endif()
        list(APPEND CLANG_ARCHIVES "${CLANG_ARCHIVE}")
    endforeach()
    set(${out_var} "${CLANG_ARCHIVES}" PARENT_SCOPE)
endfunction()

function(clang_dumper_backend_regex out_var target_name)
    if(target_name MATCHES "(x86_64|amd64|X86|x86)")
        set(regex "X86")
    elseif(target_name MATCHES "(aarch64|arm64|AArch64|ARM64)")
        set(regex "AArch64")
    elseif(target_name MATCHES "arm")
        set(regex "ARM")
    elseif(target_name MATCHES "riscv")
        set(regex "RISCV")
    else()
        set(regex "")
    endif()
    set(${out_var} "${regex}" PARENT_SCOPE)
endfunction()

function(clang_dumper_filter_target_archives out_var backend_regex)
    set(FILTERED_ARCHIVES "")
    foreach(lib IN LISTS ARGN)
        get_filename_component(libname "${lib}" NAME_WE)
        if(libname MATCHES "LLVM[A-Za-z0-9]+(CodeGen|AsmParser|Disassembler|Desc|Info|TargetMCA)$")
            # Keep the generic MC disassembler: target Desc archives reference
            # llvm::createMCRelocationInfo() defined in it, pulled in as soon
            # as the native backend is initialized/registered by the tool.
            # NOTE: NAME_WE yields "libLLVMMCDisassembler".
            if(NOT libname MATCHES "${backend_regex}" AND NOT libname MATCHES "^lib?LLVMMCDisassembler$")
                continue()
            endif()
        endif()
        list(APPEND FILTERED_ARCHIVES "${lib}")
    endforeach()
    set(${out_var} "${FILTERED_ARCHIVES}" PARENT_SCOPE)
endfunction()

# --- Tool Linking ---------------------------------------------------------------
if(WIN32 AND CMAKE_CROSSCOMPILING)
    message(STATUS "Configuring 'tool' for Windows cross static linking")

    get_filename_component(LLVM_LIBDIR "${LLVM_DIR}/../.." ABSOLUTE)
    target_link_directories(tool PRIVATE "${LLVM_LIBDIR}")

    clang_dumper_find_clang_archives(CLANG_ARCHIVES "${LLVM_LIBDIR}")
    file(GLOB LLVM_ARCHIVES "${LLVM_LIBDIR}/libLLVM*.a")
    list(FILTER LLVM_ARCHIVES EXCLUDE REGEX "[.]dll[.]a$")
    clang_dumper_backend_regex(TARGET_BACKEND_REGEX "${CMAKE_SYSTEM_PROCESSOR}")
    clang_dumper_filter_target_archives(FILTERED_LLVM_ARCHIVES "${TARGET_BACKEND_REGEX}" ${LLVM_ARCHIVES})

    set(CLANG_DUMPER_WINDOWS_RUNTIME_LIBS
        c++
        c++abi
        unwind
        pthread
        ucrtbase
        m
        z
        zstd
        version
        ole32
        oleaut32
        uuid
        advapi32
        shell32
        psapi
        ntdll
        ws2_32
        bcrypt
    )

    target_link_libraries(tool PRIVATE
        -Wl,--start-group
        ${CLANG_ARCHIVES}
        ${FILTERED_LLVM_ARCHIVES}
        ${CLANG_DUMPER_WINDOWS_RUNTIME_LIBS}
        -Wl,--end-group
    )
    target_link_options(tool PRIVATE -static -Wl,--gc-sections -Wl,--stack,16777216)

else()
    message(STATUS "Configuring 'tool' for static-style linking (always enabled)")
    find_program(LLVM_CONFIG_EXE
        NAMES llvm-config-${CLANG_VERSION} llvm-config
        HINTS "${LLVM_TOOLS_BINARY_DIR}"
    )
    if(NOT LLVM_CONFIG_EXE)
        message(FATAL_ERROR "llvm-config (version ${CLANG_VERSION}) not found; cannot build statically linked 'tool'.")
    endif()
    execute_process(COMMAND ${LLVM_CONFIG_EXE} --libdir OUTPUT_VARIABLE LLVM_LIBDIR_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LLVM_LIBDIR ${LLVM_LIBDIR_RAW})
    clang_dumper_find_clang_archives(CLANG_ARCHIVES "${LLVM_LIBDIR}")
    message(STATUS "Static Clang archives: ${CLANG_ARCHIVES}")
    execute_process(COMMAND ${LLVM_CONFIG_EXE} --libs --system-libs --link-static OUTPUT_VARIABLE LLVM_LIBS_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "\n" " " LLVM_LIBS_RAW "${LLVM_LIBS_RAW}")
    string(REGEX MATCHALL "-l[^ ]+" LLVM_LIBS_WITH_PREFIX "${LLVM_LIBS_RAW}")
    set(LLVM_LIBS_CLEAN "")
    foreach(flag IN LISTS LLVM_LIBS_WITH_PREFIX)
        string(REGEX REPLACE "^-l" "" name "${flag}")
        if(name MATCHES "^(LLVM|Polly)")
            find_library("${name}_ARCHIVE"
                NAMES "${name}"
                PATHS "${LLVM_LIBDIR}"
                NO_DEFAULT_PATH
            )
            if(NOT ${name}_ARCHIVE)
                message(FATAL_ERROR "Required static LLVM archive ${name} was not found in ${LLVM_LIBDIR}")
            endif()
            list(APPEND LLVM_LIBS_CLEAN "${${name}_ARCHIVE}")
        elseif(name STREQUAL "xml2")
            message(STATUS "Skipping llvm-config system library -lxml2 for static tool link")
        else()
            find_library("${name}_SYSTEM_LIB"
                NAMES "${name}"
            )
            if(${name}_SYSTEM_LIB)
                list(APPEND LLVM_LIBS_CLEAN "${${name}_SYSTEM_LIB}")
            else()
                list(APPEND LLVM_LIBS_CLEAN "${flag}")
            endif()
        endif()
    endforeach()

    execute_process(COMMAND ${LLVM_CONFIG_EXE} --host-target OUTPUT_VARIABLE LLVM_HOST_TARGET_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LLVM_HOST_TARGET ${LLVM_HOST_TARGET_RAW})
    clang_dumper_backend_regex(HOST_BACKEND_REGEX "${LLVM_HOST_TARGET}")
    if(HOST_BACKEND_REGEX)
        clang_dumper_filter_target_archives(FILTERED_LLVM_LIBS "${HOST_BACKEND_REGEX}" ${LLVM_LIBS_CLEAN})
        set(LLVM_LIBS_CLEAN ${FILTERED_LLVM_LIBS})
        message(STATUS "Filtered LLVM target libs to host (${LLVM_HOST_TARGET} -> ${HOST_BACKEND_REGEX}).")
    else()
        message(STATUS "Unknown host target '${LLVM_HOST_TARGET}', keeping all LLVM target libs.")
    endif()
    # The native backend's Desc archive references llvm::createMCRelocationInfo()
    # (pulled in by the tool's target registration); llvm-config does not emit
    # LLVMMCDisassembler for our dependency set, so add it explicitly.
    find_library(LLVMMCDisassembler_ARCHIVE
        NAMES "LLVMMCDisassembler"
        PATHS "${LLVM_LIBDIR}"
        NO_DEFAULT_PATH
    )
    if(NOT LLVMMCDisassembler_ARCHIVE)
        message(FATAL_ERROR "LLVMMCDisassembler archive not found in ${LLVM_LIBDIR}; required for target registration.")
    endif()
    list(FIND LLVM_LIBS_CLEAN "${LLVMMCDisassembler_ARCHIVE}" _mcdis_idx)
    if(_mcdis_idx EQUAL -1)
        list(APPEND LLVM_LIBS_CLEAN "${LLVMMCDisassembler_ARCHIVE}")
    endif()
    if(CLANG_ARCHIVES STREQUAL "")
        message(FATAL_ERROR "No Clang static archives (libclang*.a) found in ${LLVM_LIBDIR}; cannot build statically linked 'tool'. Install libclang static dev packages.")
    else()
        message(STATUS "Using Clang archives: ${CLANG_ARCHIVES}")
        message(STATUS "LLVM libs: ${LLVM_LIBS_CLEAN}")
        if(APPLE)
            # On Apple, -force_load takes the archive path as an argument. Passing
            # a bare -force_load fails with ld64 and was one of the macOS breakages.
            set(APPLE_FORCE_LOAD_CLANG_FLAGS "")
            foreach(archive IN LISTS CLANG_ARCHIVES)
                list(APPEND APPLE_FORCE_LOAD_CLANG_FLAGS "-Wl,-force_load,${archive}")
            endforeach()
            target_link_options(tool PRIVATE ${APPLE_FORCE_LOAD_CLANG_FLAGS} -Wl,-dead_strip)
            target_link_libraries(tool PRIVATE ${LLVM_LIBS_CLEAN})
        else()
            # We must enclose the static archives inside a single start/end group so the linker
            # can resolve circular dependencies between Clang and LLVM components. Previously we
            # emitted -Wl,--start-group/-Wl,--end-group as link options which CMake placed before
            # the object files, producing an empty group and leaving many unresolved symbols.
            # Inject them directly in the library list to guarantee ordering after objects.
            set(GROUPED_CLANG_LLVM_LIBS -Wl,--start-group ${CLANG_ARCHIVES} ${LLVM_LIBS_CLEAN} -Wl,--end-group)
            target_link_libraries(tool PRIVATE ${GROUPED_CLANG_LLVM_LIBS})
            # Keep static libstdc++/libgcc flags. Full static system
            # linking is not generally available on glibc-based distros.
            target_link_options(tool PRIVATE -static-libstdc++ -static-libgcc -Wl,--gc-sections)
        endif()
    endif()
endif()  # end tool linking

if(TARGET plugin)
    target_link_libraries(plugin PRIVATE clang-cpp LLVM)
endif()
