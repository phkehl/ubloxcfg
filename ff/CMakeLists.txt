########################################################################################################################
#
# ff -- various utilities used by cfgtool
#
# Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org) and contributors
#
# This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
# warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program.
# If not, see <https://www.gnu.org/licenses/>.
#
########################################################################################################################

# GENERAL ==============================================================================================================

cmake_minimum_required(VERSION 3.16)
include(../cmake/setup.cmake)

project(ff
    LANGUAGES C
    VERSION ${FF_VERSION_NUMBER}
    DESCRIPTION "ff library"
)


# COMPILER SETUP =======================================================================================================

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror \
    -Wshadow -Wunused-parameter -Wformat -Wpointer-arith -Wundef")
include(CheckCCompilerFlag)
check_c_compiler_flag(-fzero-init-padding-bits HAS_ZERO_INIT_PADDING_BITS)
if (HAS_ZERO_INIT_PADDING_BITS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fzero-init-padding-bits=unions")
endif()
set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(NDEBUG)
endif()


# DEPENDENCIES =========================================================================================================

if(NOT TARGET ubloxcfg)
    find_package(ubloxcfg REQUIRED)
endif()


# SHARED LIBRARY =======================================================================================================

file(GLOB C_FILES *.c)
file(GLOB H_FILES *.h)
message(STATUS "ff: C_FILES=${C_FILES}")
message(STATUS "ff: H_FILES=${H_FILES}")

list(APPEND CMAKE_INSTALL_RPATH $ORIGIN)
list(APPEND CMAKE_INSTALL_RPATH $ORIGIN/../lib)

add_library(${PROJECT_NAME} SHARED ${C_FILES} ${GEN_FILES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    PRIVATE
    $<INSTALL_INTERFACE:ubloxcfg>
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
    PRIVATE
       ubloxcfg
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        VERSION ${PROJECT_VERSION}
)


# INSTALL ==============================================================================================================

include(GNUInstallDirs) # Provides nice relative paths wrt CMAKE_INSTALL_PREFIX
set(PROJECT_RUNTIME_DIR ${CMAKE_INSTALL_FULL_BINDIR})
set(PROJECT_LIBRARY_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
set(PROJECT_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/${PROJECT_NAME})
set(PROJECT_DATA_DIR    ${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME})
set(PROJECT_DOC_DIR     ${CMAKE_INSTALL_FULL_DOCDIR})

# Headers
install(FILES ${H_FILES}
    DESTINATION ${PROJECT_INCLUDE_DIR}
)

# Library
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${PROJECT_LIBRARY_DIR}
)

install(
    FILES
        ${PROJECT_SOURCE_DIR}/README.md
        ${PROJECT_SOURCE_DIR}/LICENSE
    DESTINATION ${PROJECT_DOC_DIR}
)

# CMake target config
install(EXPORT ${PROJECT_NAME}-targets
    NAMESPACE ${PROJECT_NAMESPACE_PREFIX}
    FILE ${PROJECT_NAME}-targets.cmake
    DESTINATION ${PROJECT_LIBRARY_DIR}/cmake/${PROJECT_NAME}
)

# CMake config
include(CMakePackageConfigHelpers)
set(TARGET1 ${PROJECT_NAME})
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${PROJECT_LIBRARY_DIR}/cmake/${PROJECT_NAME}
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}"
    COMPATIBILITY AnyNewerVersion
)
install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION ${PROJECT_LIBRARY_DIR}/cmake/${PROJECT_NAME}
)

# pkg-config config
include(CMakePackageConfigHelpers)
set(TARGET1 ${PROJECT_NAME})
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/config.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
    INSTALL_DESTINATION ${PROJECT_LIBRARY_DIR}/cmake/${PROJECT_NAME}
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
    DESTINATION ${PROJECT_LIBRARY_DIR}/pkgconfig
)

########################################################################################################################
