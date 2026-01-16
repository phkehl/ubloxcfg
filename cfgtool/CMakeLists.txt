########################################################################################################################
#
# cfgtool -- u-blox positioning receivers configuration tool
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

project(cfgtool
    LANGUAGES C
    VERSION ${FF_VERSION_NUMBER}
    DESCRIPTION "ubloxcfg tool"
)


# COMPILER SETUP =======================================================================================================

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror \
    -Wshadow -Wunused-parameter -Wformat -Wpointer-arith -Wundef")
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
if(NOT TARGET ff)
    find_package(ff REQUIRED)
endif()


# EXECUTABLES ==========================================================================================================

file(GLOB C_FILES *.c)
file(GLOB H_FILES *.h)
message(STATUS "ff: C_FILES=${C_FILES}")
message(STATUS "ff: H_FILES=${H_FILES}")

list(APPEND CMAKE_INSTALL_RPATH $ORIGIN)
list(APPEND CMAKE_INSTALL_RPATH $ORIGIN/../lib)

add_executable(${PROJECT_NAME} ${C_FILES})
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ubloxcfg
        ff
        m
)


# GENERATED HELP =======================================================================================================

if(NOT NO_CODEGEN)
    add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/cfgtool.txt
        COMMAND ${PROJECT_NAME} -H > ${PROJECT_SOURCE_DIR}/cfgtool.txt
        DEPENDS ${PROJECT_NAME} ${C_FILES} ${H_FILES}
    )

    add_custom_target(${PROJECT_NAME}-help ALL DEPENDS ${PROJECT_SOURCE_DIR}/cfgtool.txt)
endif()


# INSTALL ==============================================================================================================

include(GNUInstallDirs) # Provides nice relative paths wrt CMAKE_INSTALL_PREFIX
set(PROJECT_RUNTIME_DIR ${CMAKE_INSTALL_FULL_BINDIR})
set(PROJECT_LIBRARY_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
set(PROJECT_INCLUDE_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR}/${PROJECT_NAME})
set(PROJECT_DATA_DIR    ${CMAKE_INSTALL_FULL_DATAROOTDIR}/${PROJECT_NAME})
set(PROJECT_DOC_DIR     ${CMAKE_INSTALL_FULL_DOCDIR})

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    RUNTIME DESTINATION ${PROJECT_RUNTIME_DIR}
)

install(
    FILES
        ${PROJECT_SOURCE_DIR}/README.md
        ${PROJECT_SOURCE_DIR}/LICENSE
        ${PROJECT_SOURCE_DIR}/../tools/99-ftdi-ublox.rules
        ${PROJECT_SOURCE_DIR}/../tools/evk-f9p-base.cfg
    DESTINATION ${PROJECT_DOC_DIR}
)

if(NOT NO_CODEGEN)
    install(
        FILES
            ${PROJECT_SOURCE_DIR}/cfgtool.txt
        DESTINATION ${PROJECT_DOC_DIR}
    )
endif()


########################################################################################################################
