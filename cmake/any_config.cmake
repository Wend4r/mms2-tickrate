# {project}
# Copyright (C) {year} {name of author}
# Licensed under the GPLv3 license. See LICENSE file in the project root for details.

if(NOT ANY_CONFIG_DIR)
	message(FATAL_ERROR "ANY_CONFIG_DIR is empty")
endif()

set(ANY_CONFIG_BINARY_DIR "s2u-any_config")

set(ANY_CONFIG_INCLUDE_DIRS
	${ANY_CONFIG_INCLUDE_DIRS}

	${ANY_CONFIG_DIR}/include
)

add_subdirectory(${ANY_CONFIG_DIR} ${ANY_CONFIG_BINARY_DIR})
