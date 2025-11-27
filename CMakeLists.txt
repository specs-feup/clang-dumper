cmake_minimum_required(VERSION 3.14)
project(ClangAstDumper)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CLANG_VERSION 16)

# --- MSVC static linking option ---------------------------------------------------
# When USE_MSVC_STATIC is ON, we use MSVC with prebuilt LLVM binaries and static linking.
# This path uses CMake config files and file globbing instead of llvm-config.
option(USE_MSVC_STATIC "Use MSVC with prebuilt LLVM static libraries" OFF)

# LLVM_INSTALL_DIR can be set to point to the prebuilt LLVM installation
if(NOT DEFINED LLVM_INSTALL_DIR AND DEFINED ENV{LLVM_INSTALL_DIR})
    set(LLVM_INSTALL_DIR "$ENV{LLVM_INSTALL_DIR}")
endif()

# --- Windows/MSYS2 auto-detection -------------------------------------------------
# Original logic assumed a manually prepared c:/mingw64-clang-${CLANG_VERSION} tree.
# In CI (GitHub Actions with MSYS2) the toolchain lives under /mingw64 instead and
# versions advance (e.g. clang 20) even if CLANG_VERSION is left at 16 for other OSes.
# We auto-detect that scenario to avoid mixing system LLVM 16 configs with clang 20.
if(WIN32 AND NOT USE_MSVC_STATIC)
    if(DEFINED ENV{MINGW_BASEDIR} AND EXISTS "$ENV{MINGW_BASEDIR}")
        set(MINGW_BASEDIR "$ENV{MINGW_BASEDIR}")
    elseif(EXISTS "c:/mingw64-clang-${CLANG_VERSION}")
        set(MINGW_BASEDIR "c:/mingw64-clang-${CLANG_VERSION}")
    elseif(EXISTS "/mingw64/bin/clang.exe")
        # Running under MSYS2 runtime (GitHub Actions, etc.)
        set(MINGW_BASEDIR "/mingw64")
        # Derive the actual major clang version if possible (updates CLANG_VERSION for downstream messages)
        execute_process(COMMAND /mingw64/bin/clang --version OUTPUT_VARIABLE _clang_ver OUTPUT_STRIP_TRAILING_WHITESPACE)
        string(REGEX MATCH "version ([0-9]+)" _match "${_clang_ver}")
        if(CMAKE_MATCH_1)
            set(CLANG_VERSION "${CMAKE_MATCH_1}")
        endif()
    else()
        message(WARNING "Could not determine MSYS2 toolchain location; falling back to c:/mingw64-clang-${CLANG_VERSION}")
        set(MINGW_BASEDIR "c:/mingw64-clang-${CLANG_VERSION}")
    endif()
endif()

# Set compiler
# Often these do not work... replace with direct invocation in command line
# Skip compiler override for MSVC builds (let CMake use the Visual Studio generator)
if(WIN32 AND NOT USE_MSVC_STATIC)
    # E.g., cmake -DCMAKE_C_COMPILER="c:/mingw64-clang-16/bin/clang.exe" -DCMAKE_CXX_COMPILER="C:/mingw64-clang-16/bin/clang++.exe" -DCMAKE_LINKER="C:/mingw64-clang-16/bin/lld-link.exe" ..
    if(NOT DEFINED CMAKE_C_COMPILER)
        SET(CMAKE_C_COMPILER "${MINGW_BASEDIR}/bin/clang.exe")
    endif()
    if(NOT DEFINED CMAKE_CXX_COMPILER)
        SET(CMAKE_CXX_COMPILER "${MINGW_BASEDIR}/bin/clang++.exe")
    endif()
    if(NOT DEFINED CMAKE_LINKER)
        SET(CMAKE_LINKER "${MINGW_BASEDIR}/bin/lld-link.exe")
    endif()
elseif(NOT WIN32)
    if(NOT DEFINED CMAKE_C_COMPILER)
        SET(CMAKE_C_COMPILER "clang-${CLANG_VERSION}")
    endif()
    if(NOT DEFINED CMAKE_CXX_COMPILER)
        SET(CMAKE_CXX_COMPILER "clang++-${CLANG_VERSION}")
    endif()
endif()

if(NOT USE_MSVC_STATIC)
    message(STATUS "Tried to set C compiler to: ${CMAKE_C_COMPILER}")
    message(STATUS "Tried to set C++ compiler to: ${CMAKE_CXX_COMPILER}")
endif()

# Set libraries
if(USE_MSVC_STATIC AND DEFINED LLVM_INSTALL_DIR)
    # MSVC with prebuilt LLVM: use provided install directory
    message(STATUS "Using MSVC static build with LLVM from: ${LLVM_INSTALL_DIR}")
    # LLVM_DIR and Clang_DIR should be passed via command line or set here
    if(NOT DEFINED LLVM_DIR)
        set(LLVM_DIR "${LLVM_INSTALL_DIR}/lib/cmake/llvm")
    endif()
    if(NOT DEFINED Clang_DIR)
        set(Clang_DIR "${LLVM_INSTALL_DIR}/lib/cmake/clang")
    endif()
elseif(WIN32 AND NOT USE_MSVC_STATIC)
    # Prefer directories under detected MINGW_BASEDIR; fall back to system if not present.
    if(EXISTS "${MINGW_BASEDIR}/lib/cmake/llvm" AND EXISTS "${MINGW_BASEDIR}/lib/cmake/clang")
        set(Clang_DIR "${MINGW_BASEDIR}/lib/cmake/clang")
        set(LLVM_DIR  "${MINGW_BASEDIR}/lib/cmake/llvm")
    else()
        message(WARNING "Expected LLVM/Clang config packages not found under ${MINGW_BASEDIR}; falling back to system locations (may cause version mismatch).")
        # System fallback matching traditional layout (still parameterized by CLANG_VERSION)
        set(Clang_DIR "/usr/lib/cmake/clang-${CLANG_VERSION}")
        set(LLVM_DIR "/usr/lib/llvm-${CLANG_VERSION}/lib/cmake/llvm")
    endif()
elseif(APPLE)
    # Point CMake to Homebrew's LLVM and Clang config packages
    set(Clang_DIR  "/usr/local/opt/llvm@${CLANG_VERSION}/lib/cmake/clang"  CACHE PATH "Path to ClangConfig.cmake")
    set(LLVM_DIR   "/usr/local/opt/llvm@${CLANG_VERSION}/lib/cmake/llvm"   CACHE PATH "Path to LLVMConfig.cmake")
else()
    SET(Clang_DIR "/usr/lib/cmake/clang-${CLANG_VERSION}")
    SET(LLVM_DIR "/usr/lib/llvm-${CLANG_VERSION}/lib/cmake/llvm")
endif()

message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

# Compiler warning flags (MSVC uses different flags)
if(MSVC)
    # MSVC equivalent warnings - /W4 is a good baseline
    add_compile_options(/W3)
else()
    add_compile_options(-Wsuggest-override)
endif()

# ZLIB handling - on Windows with MSVC and prebuilt LLVM, we may need to skip or stub ZLIB
if(USE_MSVC_STATIC)
    # The prebuilt LLVM binaries include zlib statically, so we don't need to find it separately
    # Create a stub target if ZLIB::ZLIB is referenced
    if(NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB INTERFACE IMPORTED)
        message(STATUS "MSVC static build: ZLIB assumed to be bundled with LLVM")
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


# Helpful metadata
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")



# Set the LLVM and Clang include directories
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

#include_directories("/usr/lib/llvm-${CLANG_VERSION}/include/")
#include_directories("/usr/lib/clang/${CLANG_VERSION}/include/")

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
# 1. A custom command (build step) runs the Node script that (re)generates the
#    enum sources under src/ClangEnums/.
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
# -----------------------------------------------------------------------------

# Directory for build-time helper + aggregated source
set(ENUMS_GEN_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${ENUMS_GEN_DIR}")
set(ENUMS_AGGREGATED_SRC "${ENUMS_GEN_DIR}/clang_enums_aggregated.cpp")

# Build-time script which creates the aggregator translation unit.
set(GENERATE_ENUMS_AGG_SCRIPT "${ENUMS_GEN_DIR}/generate_enum_aggregator.cmake")
file(WRITE "${GENERATE_ENUMS_AGG_SCRIPT}" "# Auto-generated helper (do not edit)\n")
file(APPEND "${GENERATE_ENUMS_AGG_SCRIPT}" "# Scans src/enums_cpp for cpp files produced by the Node script\n")
file(APPEND "${GENERATE_ENUMS_AGG_SCRIPT}" "file(GLOB ENUM_CPP_FILES \"src/enums_cpp/*.cpp\")\n")
file(APPEND "${GENERATE_ENUMS_AGG_SCRIPT}" "file(WRITE \"${ENUMS_AGGREGATED_SRC}\" \"// Aggregated enum sources - generated at build time\\n\\n\")\n")
file(APPEND "${GENERATE_ENUMS_AGG_SCRIPT}" "foreach(f IN LISTS ENUM_CPP_FILES)\n")
# Use relative include path so we don't need to escape another variable; this is evaluated when the helper script runs at build time.
file(APPEND "${GENERATE_ENUMS_AGG_SCRIPT}" "  file(APPEND \"${ENUMS_AGGREGATED_SRC}\" \"#include \\\"\${f}\\\"\\n\")\n")
file(APPEND "${GENERATE_ENUMS_AGG_SCRIPT}" "endforeach()\n")

# Collect Node generation script files as (re)build dependencies (best-effort).
file(GLOB NODE_ENUM_SCRIPTS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/clang_enums/*.mjs"
    "${CMAKE_SOURCE_DIR}/clang_enums/*.js")

add_custom_command(
    OUTPUT "${ENUMS_AGGREGATED_SRC}"
    COMMAND ${CMAKE_COMMAND} -E echo "Generating enums (Node) into src/ClangEnums ..."
    COMMAND node "${CMAKE_SOURCE_DIR}/clang_enums/Main.mjs" ${LLVM_DIR} src/
    COMMAND ${CMAKE_COMMAND} -P "${GENERATE_ENUMS_AGG_SCRIPT}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS ${NODE_ENUM_SCRIPTS}
    COMMENT "Run enum generator and aggregate sources"
    VERBATIM
)

add_custom_target(clang_enums DEPENDS "${ENUMS_AGGREGATED_SRC}")

set(SOURCE_FILES
    src/Clang/ClangAst.cpp
    src/Clang/ClangNodes.cpp
    src/ClangEnums/ClangEnums.cpp
    src/TypeMarker/TypeMarker.cpp
    src/Clava/ClavaConstants.cpp
    src/Clava/ClavaDecl.cpp
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

# Plugin target - skip for MSVC static builds (requires shared clang-cpp which isn't available)
if(NOT USE_MSVC_STATIC)
    add_library(plugin MODULE ${SOURCE_FILES} src/plugin.cpp)
    target_compile_features(plugin PRIVATE cxx_std_20)
    add_dependencies(plugin clang_enums)
endif()


add_executable(tool ${SOURCE_FILES} src/tool.cpp)
target_compile_features(tool PRIVATE cxx_std_20)
add_dependencies(tool clang_enums)

# Size optimization flags (Option 3): place each function/data in its own section
# MSVC uses different flags for this
if(MSVC)
    target_compile_options(tool PRIVATE /Gy)  # Function-level linking
else()
    target_compile_options(tool PRIVATE -ffunction-sections -fdata-sections)
endif()

# Locate binutils tools early for post-build debug splitting
if(NOT CMAKE_OBJCOPY)
    find_program(CMAKE_OBJCOPY NAMES objcopy)
endif()
if(NOT CMAKE_STRIP)
    find_program(CMAKE_STRIP NAMES strip)
endif()


message(STATUS "SYSTEM PROCESSOR: '${CMAKE_SYSTEM_PROCESSOR}'")
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm") 
    set(PROCESSOR_PREFIX "ARM")
else()
    set(PROCESSOR_PREFIX "X86")
endif()


#set_property(TARGET tool PROPERTY INTERPROCEDURAL_OPTIMIZATION OFF)


# --- MSVC Static Linking Path -----------------------------------------------------
# When using MSVC with prebuilt LLVM binaries, we use a different approach:
# - No llvm-config available, so we use CMake's find_package results and file globbing
# - Static libraries are .lib files instead of .a
# - Different linker flags
if(USE_MSVC_STATIC)
    message(STATUS "Configuring 'tool' for MSVC static linking")
    
    # Get LLVM library directory from the found package
    get_filename_component(LLVM_LIBDIR "${LLVM_DIR}/../.." ABSOLUTE)
    message(STATUS "LLVM library directory: ${LLVM_LIBDIR}")
    
    # Include directories should already be set by find_package, but ensure they're there
    target_include_directories(tool PRIVATE "${LLVM_INSTALL_DIR}/include")
    
    # Discover Clang static libraries (.lib files)
    file(GLOB CLANG_LIBS_ALL "${LLVM_LIBDIR}/clang*.lib")
    set(CLANG_LIBS ${CLANG_LIBS_ALL})
    # Exclude clangd and clang-tidy related libraries
    list(FILTER CLANG_LIBS EXCLUDE REGEX "clangd.*[.]lib$")
    list(FILTER CLANG_LIBS EXCLUDE REGEX "clangTidy.*[.]lib$")
    # Exclude other unnecessary libraries
    list(FILTER CLANG_LIBS EXCLUDE REGEX "clang(ChangeNamespace|Doc|ExtractAPI|Format|Move|ApplyReplacements|DirectoryWatcher|DependencyScanning|Daemon|IncludeFixer|Pseudo|DynamicASTMatchers)[^.]*[.]lib$")
    
    message(STATUS "Clang libraries found: ${CLANG_LIBS}")
    
    # Discover LLVM static libraries
    file(GLOB LLVM_LIBS_ALL "${LLVM_LIBDIR}/LLVM*.lib")
    set(LLVM_LIBS ${LLVM_LIBS_ALL})
    
    # Filter to keep only host target backend (X86 for Windows x64)
    set(HOST_BACKEND_REGEX "X86")
    set(FILTERED_LLVM_LIBS "")
    foreach(lib IN LISTS LLVM_LIBS)
        get_filename_component(libname "${lib}" NAME_WE)
        # If it looks like a target-specific codegen/asm/disassembler lib and does not match host, skip.
        if(libname MATCHES "LLVM[A-Za-z0-9]+(CodeGen|AsmParser|Disassembler|Desc|Info|TargetMCA)$")
            if(NOT libname MATCHES "${HOST_BACKEND_REGEX}")
                continue()
            endif()
        endif()
        list(APPEND FILTERED_LLVM_LIBS ${lib})
    endforeach()
    set(LLVM_LIBS ${FILTERED_LLVM_LIBS})
    
    message(STATUS "LLVM libraries (filtered): ${LLVM_LIBS}")
    
    # Discover additional support libraries (lld, etc.)
    file(GLOB LLD_LIBS "${LLVM_LIBDIR}/lld*.lib")
    
    # Windows system libraries needed for LLVM
    set(WINDOWS_SYSTEM_LIBS
        version
        ole32
        oleaut32
        uuid
        advapi32
        shell32
        psapi
        ntdll
    )
    
    # Link everything together
    # MSVC linker handles circular dependencies automatically, no need for --start-group/--end-group
    target_link_directories(tool PRIVATE "${LLVM_LIBDIR}")
    target_link_libraries(tool PRIVATE 
        ${CLANG_LIBS}
        ${LLVM_LIBS}
        ${LLD_LIBS}
        ${WINDOWS_SYSTEM_LIBS}
    )
    
    # Use static runtime (/MT for Release, /MTd for Debug)
    set_property(TARGET tool PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    
    # Linker optimizations
    target_link_options(tool PRIVATE /OPT:REF /OPT:ICF)

else()
    # --- Original non-MSVC static linking path ---
    message(STATUS "Configuring 'tool' for static-style linking (always enabled)")
    find_program(LLVM_CONFIG_EXE NAMES llvm-config-${CLANG_VERSION} llvm-config)
    if(NOT LLVM_CONFIG_EXE)
        message(FATAL_ERROR "llvm-config (version ${CLANG_VERSION}) not found; cannot build statically linked 'tool'.")
    else()
    execute_process(COMMAND ${LLVM_CONFIG_EXE} --libdir OUTPUT_VARIABLE LLVM_LIBDIR_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LLVM_LIBDIR ${LLVM_LIBDIR_RAW})
    if(WIN32)
        target_link_directories(tool PRIVATE "${LLVM_LIBDIR}")
    endif()
    # Automatic discovery of Clang static archives, minus known bulky/irrelevant ones.
    # We exclude clangd* (language server), clangTidy* (analysis modules), *Proto/Monitoring*
    # which pull protobuf or service code we do not use. This keeps maintenance low
    # while avoiding the massive over-linking that led to unresolved driver/sarif references.
    file(GLOB CLANG_ARCHIVES_ALL "${LLVM_LIBDIR}/libclang*.a")
    list(FILTER CLANG_ARCHIVES_ALL EXCLUDE REGEX "libclang-cpp[.]a$")
    set(CLANG_ARCHIVES ${CLANG_ARCHIVES_ALL})
    # Drop clangd and tidy related archives
    list(FILTER CLANG_ARCHIVES EXCLUDE REGEX "libclangd.*[.]a$")
    list(FILTER CLANG_ARCHIVES EXCLUDE REGEX "libclangTidy.*[.]a$")
    # Drop ancillary tooling pieces we do not rely on (may be added back if needed)
    list(FILTER CLANG_ARCHIVES EXCLUDE REGEX "libclang(ChangeNamespace|Doc|ExtractAPI|Format|Move|ApplyReplacements|DirectoryWatcher|DependencyScanning|Daemon.*|IncludeFixer.*|Pseudo.*|DynamicASTMatchers|dMonitoringServiceProto|dRemoteIndex.*|dSupport|dRemoteMarshalling)[.]a$")
    message(STATUS "Filtered Clang archives (auto): ${CLANG_ARCHIVES}")
    execute_process(COMMAND ${LLVM_CONFIG_EXE} --libs --system-libs --link-static OUTPUT_VARIABLE LLVM_LIBS_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "\n" " " LLVM_LIBS_RAW "${LLVM_LIBS_RAW}")
    string(REGEX MATCHALL "-l[^ ]+" LLVM_LIBS_WITH_PREFIX "${LLVM_LIBS_RAW}")
    set(LLVM_LIBS_CLEAN "")
    foreach(flag IN LISTS LLVM_LIBS_WITH_PREFIX)
        string(REGEX REPLACE "^-l" "" name "${flag}")
        list(APPEND LLVM_LIBS_CLEAN ${name})
    endforeach()

    # Filter (Option 4): retain only host target backends to reduce size.
    execute_process(COMMAND ${LLVM_CONFIG_EXE} --host-target OUTPUT_VARIABLE LLVM_HOST_TARGET_RAW OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(LLVM_HOST_TARGET ${LLVM_HOST_TARGET_RAW})
    # Map common triples to backend substring patterns used in library names.
    if(LLVM_HOST_TARGET MATCHES "x86_64")
        set(HOST_BACKEND_REGEX "X86")
    elseif(LLVM_HOST_TARGET MATCHES "aarch64")
        set(HOST_BACKEND_REGEX "AArch64")
    elseif(LLVM_HOST_TARGET MATCHES "arm")
        set(HOST_BACKEND_REGEX "ARM")
    elseif(LLVM_HOST_TARGET MATCHES "riscv")
        set(HOST_BACKEND_REGEX "RISCV")
    else()
        set(HOST_BACKEND_REGEX "")
    endif()
    if(HOST_BACKEND_REGEX)
        set(FILTERED_LLVM_LIBS "")
        foreach(lib IN LISTS LLVM_LIBS_CLEAN)
            # If it looks like a target-specific codegen/asm/disassembler lib and does not match host, skip.
            if(lib MATCHES "LLVM[A-Za-z0-9]+(CodeGen|AsmParser|Disassembler|Desc|Info|TargetMCA)$")
                if(NOT lib MATCHES "${HOST_BACKEND_REGEX}")
                    continue()
                endif()
            endif()
            list(APPEND FILTERED_LLVM_LIBS ${lib})
        endforeach()
        set(LLVM_LIBS_CLEAN ${FILTERED_LLVM_LIBS})
        message(STATUS "Filtered LLVM target libs to host (${LLVM_HOST_TARGET} -> ${HOST_BACKEND_REGEX}).")
    else()
        message(STATUS "Unknown host target '${LLVM_HOST_TARGET}', keeping all LLVM target libs.")
    endif()
    if(CLANG_ARCHIVES STREQUAL "")
        message(FATAL_ERROR "No Clang static archives (libclang*.a) found in ${LLVM_LIBDIR}; cannot build statically linked 'tool'. Install libclang static dev packages.")
    else()
        message(STATUS "Using Clang archives: ${CLANG_ARCHIVES}")
        message(STATUS "LLVM libs: ${LLVM_LIBS_CLEAN}")
        if(APPLE)
            # On Apple we need -force_load semantics so that object files inside the archives
            # that are only referenced via plugin-like registration get pulled in.
            target_link_options(tool PRIVATE -Wl,-force_load)
            target_link_directories(tool PRIVATE /opt/homebrew/lib)
            target_link_libraries(tool PRIVATE ${CLANG_ARCHIVES} ${LLVM_LIBS_CLEAN})
        else()
            # We must enclose the static archives inside a single start/end group so the linker
            # can resolve circular dependencies between Clang and LLVM components. Previously we
            # emitted -Wl,--start-group/-Wl,--end-group as link options which CMake placed before
            # the object files, producing an empty group and leaving many unresolved symbols.
            # Inject them directly in the library list to guarantee ordering after objects.
            set(GROUPED_CLANG_LLVM_LIBS -Wl,--start-group ${CLANG_ARCHIVES} ${LLVM_LIBS_CLEAN} -Wl,--end-group)
            target_link_libraries(tool PRIVATE ${GROUPED_CLANG_LLVM_LIBS})
            # Keep static libstdc++/libgcc flags.
            target_link_options(tool PRIVATE -static-libstdc++ -static-libgcc -Wl,--gc-sections)
        endif()
    endif()
    endif()  # end of if(NOT LLVM_CONFIG_EXE) else block
endif()  # end of if(USE_MSVC_STATIC) else block

# Plugin always dynamic (portable; small) - skip for MSVC static builds
if(NOT USE_MSVC_STATIC)
    target_link_libraries(plugin PRIVATE clang-cpp LLVM)
endif()


# If on MacOS, change the expected location of libLLVM.dylib and libzstd.1.dylib
# To check changes, use 'otool -L tool'
if(APPLE)
    add_custom_command (
        TARGET tool POST_BUILD
        COMMAND install_name_tool -change /usr/local/opt/llvm@${CLANG_VERSION}/lib/libLLVM.dylib @executable_path/libLLVM.dylib "$<TARGET_FILE:tool>"
        COMMAND install_name_tool -change /usr/local/opt/zstd/lib/libzstd.1.dylib @executable_path/libzstd.1.dylib  "$<TARGET_FILE:tool>"
    )
endif()

# Test target - runs Python test script
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_custom_target(test
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_tests.py --tool-path $<TARGET_FILE:tool>
        DEPENDS tool
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-dumper tests..."
    )

    # Test generation target - generates expected output files
    add_custom_target(test-generate
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run_tests.py --tool-path $<TARGET_FILE:tool> --generate
        DEPENDS tool
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating expected test outputs..."
    )
else()
    message(WARNING "Python3 not found. Test targets 'test' and 'test-generate' will not be available. Please install Python 3 to enable testing.")
endif()