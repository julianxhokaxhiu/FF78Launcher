#*****************************************************************************#
#    Copyright (C) 2024 Julian Xhokaxhiu                                      #
#                                                                             #
#    This file is part of FF78Launcher                                        #
#                                                                             #
#    FF78Launcher is free software: you can redistribute it and/or modify     #
#    it under the terms of the GNU General Public License as published by     #
#    the Free Software Foundation, either version 3 of the License            #
#                                                                             #
#    FF78Launcher is distributed in the hope that it will be useful,          #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of           #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            #
#    GNU General Public License for more details.                             #
#*****************************************************************************#

cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0091 NEW)

if(NOT DEFINED CMAKE_BUILD_TYPE)
  message(FATAL_ERROR "CMAKE_BUILD_TYPE must be set to continue building with cmake. \nExample: Add -DCMAKE_BUILD_TYPE=Release to your cmake command line.")
endif()
if (NOT DEFINED _EXE_VERSION OR NOT _EXE_VERSION)
  message(FATAL_ERROR "_EXE_VERSION must be set to continue building with cmake. \nExample: Add -D_EXE_VERSION=devel to your cmake command line.")
endif ()

set(VCPKG_INSTALL_OPTIONS "--clean-after-build" "--x-use-aria2")

set(CMAKE_EXE_LINKER_FLAGS
  "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT /NODEFAULTLIB:MSVCRTD /DEBUG:FULL /FORCE:MULTIPLE /IGNORE:4006,4075,4099,4217"
)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebug")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT")
else()
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMTD")
endif()

project(FF78Launcher)

find_package(plog CONFIG REQUIRED)
find_package(STACKWALKER REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

set(RELEASE_NAME "FF78Launcher")

if(_EXE_VERSION STREQUAL "devel" OR _EXE_VERSION MATCHES "-")
  set(_EXE_RCVERSION "0,0,0,0")
  set(_EXE_RCSTRVERSION "0.0.0.0")
else()
  string(REPLACE "." "," _EXE_RCVERSION ${_EXE_VERSION})
  set(_EXE_RCSTRVERSION ${_EXE_VERSION})
endif()

# Include all the source code files
file(GLOB_RECURSE source_files "${CMAKE_SOURCE_DIR}/src/*.cpp")

configure_file(${CMAKE_SOURCE_DIR}/src/version.rc ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)

add_executable(
  ${RELEASE_NAME}
  WIN32
  ${source_files}
  ${CMAKE_CURRENT_BINARY_DIR}/version.rc
)
target_include_directories(
  ${RELEASE_NAME}
  PRIVATE ${CMAKE_SOURCE_DIR}/src
  PRIVATE ${STACKWALKER_INCLUDE_DIRS}
)
target_link_libraries(
  ${RELEASE_NAME}
  shlwapi
  plog::plog
  tomlplusplus::tomlplusplus
  ${STACKWALKER_LIBRARIES}
)
target_compile_options(
  ${RELEASE_NAME}
  PRIVATE /D_WIN32_WINNT=0x0601
  PRIVATE /D_CRT_SECURE_NO_WARNINGS
  PRIVATE /DNOMINMAX
  PRIVATE /Zc:strictStrings-
  PRIVATE /Qpar
  # App Defines
  PRIVATE /DAPP_CONFIG_FILE="${RELEASE_NAME}.toml"
  PRIVATE /DAPP_RELEASE_NAME="${RELEASE_NAME}"
  PRIVATE /DAPP_RELEASE_VERSION="${_EXE_RCSTRVERSION}"
)
target_compile_features(
  ${RELEASE_NAME}
  PRIVATE cxx_std_20
)
target_link_options(
  ${RELEASE_NAME}
  PRIVATE /PDBALTPATH:${RELEASE_NAME}.pdb
)

# INSTALL
add_custom_command(
  TARGET ${RELEASE_NAME}
  POST_BUILD
  # Add dependencies directly into the release folder in order to allow debugging from VSCode/VStudio
  # ...
  # ensure bin directory exists
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/bin
  # copy everything from the release folder to the final destination
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}
          ${CMAKE_BINARY_DIR}/bin
  # copy License
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
          ${CMAKE_BINARY_DIR}/bin/COPYING.TXT
  # .toml
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_CURRENT_SOURCE_DIR}/misc/${RELEASE_NAME}.toml
          ${CMAKE_BINARY_DIR}/bin
)
