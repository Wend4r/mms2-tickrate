# {project}
# Copyright (C) {year} {name of author}
# Licensed under the GPLv3 license. See LICENSE file in the project root for details.

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(POLICY CMP0092)
	cmake_policy(SET CMP0092 NEW) # Don't add -W3 warning level by default.
endif()

project(mms2-sample
	VERSION 1.0.0
	DESCRIPTION "Sample"
	HOMEPAGE_URL "https://github.com/Wend4r/mms2-sample"
	LANGUAGES C CXX
)

string(REGEX REPLACE "^[^-]*-" "" PROJECT_NAME_SUBSTRING "${PROJECT_NAME}")
string(TOLOWER "${PROJECT_NAME_SUBSTRING}" PROJECT_NAME_LOWER)
string(TOUPPER "${PROJECT_NAME_SUBSTRING}" PROJECT_NAME_UPPER)

set(PROJECT_OUTPUT_NAME "sample")

set(PROJECT_AUTHOR "Sample author")
set(PROJECT_DESCRIPTION_FULL "Metamod:Source sample plugin")
set(PROJECT_LICENSE "GPLv3")
string(TIMESTAMP PROJECT_BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP PROJECT_BUILD_TIME "%H:%M:%S")

set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(PUBLIC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/public")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(SOURCE_SAMPLE_DIR "${SOURCE_DIR}/sample")
set(SOURCE_SAMPLE_PROVIDER_DIR "${SOURCE_SAMPLE_DIR}/provider")
set(SOURCE_SAMPLE_PROVIDER_GAMEDATA_DIR "${SOURCE_SAMPLE_PROVIDER_DIR}/gamedata")

function(set_or_external_dir VAR_NAME DEFAULT_DIR)
	if(${VAR_NAME})
		file(TO_CMAKE_PATH "${${VAR_NAME}}" ${VAR_NAME})
	else()
		set(${VAR_NAME} "${EXTERNAL_DIR}/${DEFAULT_DIR}")
	endif()

	set(${VAR_NAME} "${${VAR_NAME}}" PARENT_SCOPE)
endfunction()

set_or_external_dir(ANY_CONFIG_DIR "any_config")
set_or_external_dir(DYNLIBUTILS_DIR "memory_utils")
set_or_external_dir(GAMEDATA_DIR "gamedata")
set_or_external_dir(LOGGER_DIR "logger")
set_or_external_dir(METAMOD_DIR "metamod-source")
set_or_external_dir(SOURCESDK_DIR "sourcesdk")
set_or_external_dir(TRNALSTIONS_DIR "translations")

include(cmake/platform/shared.cmake)

include(cmake/any_config.cmake)
include(cmake/dynlibutils.cmake)
include(cmake/gamedata.cmake)
include(cmake/logger.cmake)
include(cmake/metamod.cmake)
include(cmake/sourcesdk.cmake)
include(cmake/translations.cmake)

set(SOURCE_FILES
	${SOURCE_SAMPLE_PROVIDER_GAMEDATA_DIR}/gameresource.cpp
	${SOURCE_SAMPLE_PROVIDER_GAMEDATA_DIR}/gamesystem.cpp
	${SOURCE_SAMPLE_PROVIDER_GAMEDATA_DIR}/source2server.cpp
	${SOURCE_SAMPLE_DIR}/chat_command_system.cpp
	${SOURCE_SAMPLE_DIR}/provider.cpp
	${SOURCE_DIR}/concat.cpp
	${SOURCE_DIR}/globals.cpp
	${SOURCE_DIR}/sample_plugin.cpp
)

set(INCLUDE_DIRS
	${INCLUDE_DIR}
	${PUBLIC_DIR}
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF

	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_OUTPUT_NAME})

if(WINDOWS)
	set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(MACOS)
	set_target_properties(${PROJECT_NAME} PROPERTIES OSX_ARCHITECTURES "x86_64")
endif()

target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILER_OPTIONS} ${SOURCESDK_COMPILE_OPTIONS})
target_link_options(${PROJECT_NAME} PRIVATE ${LINK_OPTIONS} ${SOURCESDK_LINK_OPTIONS})

target_compile_definitions(${PROJECT_NAME} PRIVATE ${COMPILE_DEFINITIONS} ${METAMOD_COMPILE_DEFINITIONS} ${SOURCESDK_COMPILE_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIRS} ${ANY_CONFIG_INCLUDE_DIRS} ${DYNLIBUTILS_INCLUDE_DIRS} ${GAMEDATA_INCLUDE_DIRS} ${LOGGER_INCLUDE_DIRS} ${METAMOD_INCLUDE_DIRS} ${SOURCESDK_INCLUDE_DIRS} ${TRNALSTIONS_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${LINK_LIBRARIES} ${ANY_CONFIG_BINARY_DIR} ${DYNLIBUTILS_BINARY_DIR} ${GAMEDATA_BINARY_DIR} ${LOGGER_BINARY_DIR} ${SOURCESDK_BINARY_DIR} ${TRNALSTIONS_BINARY_DIR})
