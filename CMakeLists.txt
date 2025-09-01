cmake_minimum_required(VERSION 3.16)
include(cmake/setup.cmake)

project(everything
    LANGUAGES C CXX
    VERSION ${FF_VERSION_NUMBER}
    DESCRIPTION "everything"
)

add_subdirectory(ubloxcfg)
add_subdirectory(ff)
add_subdirectory(cfgtool)
