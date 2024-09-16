# {project}
# Copyright (C) {year} {name of author}
# Licensed under the GPLv3 license. See LICENSE file in the project root for details.

if(NOT LOGGER_DIR)
	message(FATAL_ERROR "LOGGER_DIR is empty")
endif()

set(LOGGER_BINARY_DIR "s2u-logger")

set(LOGGER_INCLUDE_DIRS
	${LOGGER_INCLUDE_DIRS}

	${LOGGER_DIR}/include
)

add_subdirectory(${LOGGER_DIR} ${LOGGER_BINARY_DIR})
