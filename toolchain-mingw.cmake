set(CMAKE_SYSTEM_NAME Windows)

# Cross-compiler triplet
set(CROSS_PREFIX x86_64-w64-mingw32 CACHE STRING "GNU triplet for the target")
set(HOST_LLD_DIR "" CACHE PATH "Directory containing a Linux-host ld.lld for MinGW cross-linking")
if(NOT DEFINED CMAKE_SYSTEM_PROCESSOR)
	if(CROSS_PREFIX MATCHES "^(aarch64|arm64)-")
		set(CMAKE_SYSTEM_PROCESSOR ARM64)
	elseif(CROSS_PREFIX MATCHES "^i[3-6]86-")
		set(CMAKE_SYSTEM_PROCESSOR x86)
	else()
		set(CMAKE_SYSTEM_PROCESSOR x86_64)
	endif()
endif()

# Location of the MinGW/LLVM sysroot (bin/lib/include). Defaults to /usr/<triplet>
set(MINGW_SYSROOT "" CACHE PATH "Root directory of the MinGW toolchain")
if(NOT MINGW_SYSROOT)
	if(DEFINED ENV{MINGW_BASEDIR} AND EXISTS "$ENV{MINGW_BASEDIR}")
		set(MINGW_SYSROOT "$ENV{MINGW_BASEDIR}")
	else()
		set(MINGW_SYSROOT "/usr/${CROSS_PREFIX}")
	endif()
endif()

set(_MINGW_BIN "${MINGW_SYSROOT}/bin")

find_program(MINGW_C_COMPILER
	NAMES ${CROSS_PREFIX}-clang ${CROSS_PREFIX}-gcc
	HINTS "${_MINGW_BIN}"
)
if(NOT MINGW_C_COMPILER)
	find_program(MINGW_C_COMPILER NAMES clang)
endif()
set(CMAKE_C_COMPILER ${MINGW_C_COMPILER})

find_program(MINGW_CXX_COMPILER
	NAMES ${CROSS_PREFIX}-clang++ ${CROSS_PREFIX}-clang ${CROSS_PREFIX}-g++
	HINTS "${_MINGW_BIN}"
)
if(NOT MINGW_CXX_COMPILER)
	find_program(MINGW_CXX_COMPILER NAMES clang++)
endif()
set(CMAKE_CXX_COMPILER ${MINGW_CXX_COMPILER})

if(NOT CMAKE_C_COMPILER OR NOT CMAKE_CXX_COMPILER)
	message(FATAL_ERROR "Could not find a MinGW compiler wrapper or host clang/clang++ for ${CROSS_PREFIX}")
endif()

if(MINGW_C_COMPILER MATCHES "/clang[^/]*$" OR MINGW_CXX_COMPILER MATCHES "/clang[^/]*$")
	set(CMAKE_C_COMPILER_TARGET "${CROSS_PREFIX}" CACHE STRING "" FORCE)
	set(CMAKE_CXX_COMPILER_TARGET "${CROSS_PREFIX}" CACHE STRING "" FORCE)
	set(_CLANG_MINGW_LINKER_FLAGS "")
	if(HOST_LLD_DIR)
		set(_CLANG_MINGW_LINKER_FLAGS "-B${HOST_LLD_DIR} -fuse-ld=lld")
	endif()
	set(_CLANG_MINGW_COMPILE_FLAGS "-resource-dir=${MINGW_SYSROOT}/lib/clang/18")
	set(_CLANG_MINGW_RUNTIME_FLAGS "-rtlib=compiler-rt -unwindlib=libunwind")
	set(CMAKE_C_FLAGS_INIT "--target=${CROSS_PREFIX} --sysroot=${MINGW_SYSROOT} ${_CLANG_MINGW_COMPILE_FLAGS}")
	set(CMAKE_CXX_FLAGS_INIT "--target=${CROSS_PREFIX} --sysroot=${MINGW_SYSROOT} ${_CLANG_MINGW_COMPILE_FLAGS} -stdlib=libc++")
	set(CMAKE_EXE_LINKER_FLAGS_INIT "--target=${CROSS_PREFIX} --sysroot=${MINGW_SYSROOT} ${_CLANG_MINGW_LINKER_FLAGS} ${_CLANG_MINGW_COMPILE_FLAGS} ${_CLANG_MINGW_RUNTIME_FLAGS} -stdlib=libc++")
	set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT}")
	set(CMAKE_MODULE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT}")
endif()

find_program(MINGW_RC_COMPILER
	NAMES llvm-rc ${CROSS_PREFIX}-windres
	HINTS "${_MINGW_BIN}"
)
if(MINGW_RC_COMPILER)
	set(CMAKE_RC_COMPILER ${MINGW_RC_COMPILER})
endif()

# Tell CMake how to locate headers/libraries for the target sysroot
set(CMAKE_FIND_ROOT_PATH
	"${MINGW_SYSROOT}/${CROSS_PREFIX}"
	"${MINGW_SYSROOT}/${CROSS_PREFIX}/lib"
	"${MINGW_SYSROOT}"
)

# Prefer cross-compiled libraries over system libraries
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
