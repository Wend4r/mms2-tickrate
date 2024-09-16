# {project}
# Copyright (C) {year} {name of author}
# Licensed under the GPLv3 license. See LICENSE file in the project root for details.

if(NOT TRNALSTIONS_DIR)
	message(FATAL_ERROR "TRNALSTIONS_DIR is empty")
endif()

set(TRNALSTIONS_BINARY_DIR "s2u-translations")

set(TRNALSTIONS_INCLUDE_DIRS
	${TRNALSTIONS_INCLUDE_DIRS}

	${TRNALSTIONS_DIR}/include
)

add_subdirectory(${TRNALSTIONS_DIR} ${TRNALSTIONS_BINARY_DIR})
