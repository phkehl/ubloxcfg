########################################################################################################################
#
# u-blox 9 positioning receivers configuration library and tool
#
# Copyright (c) Philippe Kehl (flipflip at oinkzwurgl dot org),
# https://oinkzwurgl.org/hacking/ubloxcfg
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

cmake_minimum_required(VERSION 3.9)

# ----------------------------------------------------------------------------------------------------------------------

# Sets PROJECT_NAME, PROJECT_VERSION and PROJECT_DESCRIPTION
project(ubloxcfg
    VERSION       0.0
    DESCRIPTION   "u-blox 9 positioning receivers configuration and control library"
    LANGUAGES     "C"
    HOMEPAGE_URL  "https://oinkzwurgl.org/projaeggd/ubloxcfg/")

add_library(${PROJECT_NAME} SHARED
    ../ubloxcfg/ubloxcfg.c
    ../ubloxcfg/ubloxcfg_gen.c
    ../ff/ff_crc.c
    ../ff/ff_debug.c
    ../ff/ff_epoch.c
    ../ff/ff_nmea.c
    ../ff/ff_novatel.c
    ../ff/ff_parser.c
    ../ff/ff_port.c
    ../ff/ff_rtcm3.c
    ../ff/ff_rx.c
    ../ff/ff_spartn.c
    ../ff/ff_stuff.c
    ../ff/ff_time.c
    ../ff/ff_trafo.c
    ../ff/ff_ubx.c
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION              ${PROJECT_VERSION}
    SOVERSION            ${PROJECT_VERSION_MAJOR}
    C_STANDARD           99 # apparently that does mean gnu99, which is what we want
    C_STANDARD_REQUIRED  TRUE
)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC m)
target_include_directories(${PROJECT_NAME} PRIVATE ../ubloxcfg ../ff ../3rdparty/stuff)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -Wshadow)
target_compile_definitions(${PROJECT_NAME} PRIVATE CONFIG_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
target_compile_definitions(${PROJECT_NAME} PRIVATE CONFIG_VERSION_MINOR=${PROJECT_VERSION_MINOR})

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(${PROJECT_NAME} PRIVATE FF_BUILD_RELEASE)
    target_compile_options(${PROJECT_NAME} PRIVATE -O3)
endif()
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE FF_BUILD_DEBUG)
    target_compile_options(${PROJECT_NAME} PRIVATE -Og)
endif()

# ----------------------------------------------------------------------------------------------------------------------

include(GNUInstallDirs)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "\
../ubloxcfg/ubloxcfg.h;\
../ubloxcfg/ubloxcfg_gen.h;\
../ff/ff_crc.h;\
../ff/ff_debug.h;\
../ff/ff_epoch.h;\
../ff/ff_nmea.h;\
../ff/ff_novatel.h;\
../ff/ff_parser.h;\
../ff/ff_port.h;\
../ff/ff_rtcm3.h;\
../ff/ff_rx.h;\
../ff/ff_spartn.h;\
../ff/ff_stuff.h;\
../ff/ff_time.h;\
../ff/ff_trafo.h;\
../ff/ff_ubx.h;\
")

install(TARGETS ${PROJECT_NAME}
    LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

configure_file(${PROJECT_NAME}.pc.in ${PROJECT_NAME}.pc @ONLY)

install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

# ----------------------------------------------------------------------------------------------------------------------
