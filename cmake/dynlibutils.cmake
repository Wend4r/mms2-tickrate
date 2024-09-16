# Tickrate
# Copyright (C) 2024 Wend4r
# Licensed under the GPLv3 license. See LICENSE file in the project root for details.

if(NOT DYNLIBUTILS_DIR)
	message(FATAL_ERROR "DYNLIBUTILS_DIR is empty")
endif()

# GameData already have DynLibUtils library!

set(DYNLIBUTILS_BINARY_DIR "cpp-memory_utils")

set(DYNLIBUTILS_INCLUDE_DIRS
	${DYNLIBUTILS_INCLUDE_DIRS}

	${DYNLIBUTILS_DIR}/include
)

add_subdirectory(${DYNLIBUTILS_DIR} ${DYNLIBUTILS_BINARY_DIR})
