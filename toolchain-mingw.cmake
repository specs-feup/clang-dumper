# toolchain-mingw.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)  # Change to x86 if targeting 32-bit

# Cross-compiler triplet
set(CROSS_PREFIX x86_64-w64-mingw32 CACHE STRING "GNU triplet for the target")  # Change to i686-w64-mingw32 for 32-bit

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
	message(FATAL_ERROR "Could not find ${CROSS_PREFIX}-clang or -gcc under ${_MINGW_BIN}")
endif()
set(CMAKE_C_COMPILER ${MINGW_C_COMPILER})

find_program(MINGW_CXX_COMPILER
	NAMES ${CROSS_PREFIX}-clang++ ${CROSS_PREFIX}-clang ${CROSS_PREFIX}-g++
	HINTS "${_MINGW_BIN}"
)
if(NOT MINGW_CXX_COMPILER)
	message(FATAL_ERROR "Could not find ${CROSS_PREFIX}-clang++/clang/g++ under ${_MINGW_BIN}")
endif()
set(CMAKE_CXX_COMPILER ${MINGW_CXX_COMPILER})

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
