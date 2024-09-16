# {project}
# Copyright (C) {year} {name of author}
# Licensed under the GPLv3 license. See LICENSE file in the project root for details.

if(NOT GAMEDATA_DIR)
	message(FATAL_ERROR "GAMEDATA_DIR is empty")
endif()

set(GAMEDATA_BINARY_DIR "s2u-gamedata")

set(GAMEDATA_INCLUDE_DIRS
	${GAMEDATA_INCLUDE_DIRS}

	${GAMEDATA_DIR}/include
)

add_subdirectory(${GAMEDATA_DIR} ${GAMEDATA_BINARY_DIR})
