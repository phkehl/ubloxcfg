########################################################################################################################
# Some variables
set(FF_CMAKE_CMD_HELP "cmake -B build -DCMAKE_INSTALL_PREFIX=~/ubloxcfg")


########################################################################################################################
# (Somewhat) prevent in-source builds
get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)
message(STATUS "ff: srcdir=${srcdir}")
message(STATUS "ff: bindir=${bindir}")
if("${srcdir}" STREQUAL "${bindir}")
    message(FATAL_ERROR "\n\n ==> Aborting attempt to do a in-source build. Use '${FF_CMAKE_CMD_HELP}'\n\n")
endif()


########################################################################################################################
# Add install prefix to cmake path. Other libs may be available there.
if(NOT "${CMAKE_INSTALL_PREFIX}" STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})
endif()


########################################################################################################################
# Get version info from GIT, unless explicitly given by the user (cmake command line)
# TODO: This is executed only once on configuration of the project. That's of course wrong. We'd have to execute this
# always, write/update some.hpp that can be included and properly dependency-managed. There are things like
# https://github.com/andrew-hardin/cmake-git-version-tracking/tree/master, but they're quite involved, too.

if (NOT FF_VERSION_IS_SET) # Do this only once. E.g. when running the top-level CMakeLists.txt
    # - Version supplied on cmake command line (-DVERSION_STRING=x.x.x, -DVERSION_STRING=x.x.x-gggggg)
    if (FF_VERSION_STRING)
        string(REGEX MATCH "^([0-9]+\.[0-9]+\.[0-9]+).*$" _ ${FF_VERSION_STRING})
        if ("${CMAKE_MATCH_1}" STREQUAL "")
            message(FATAL_ERROR "ff: bad FF_VERSION_STRING=${FF_VERSION_STRING}, must be x.x.x or x.x.x-ggggggg")
        endif()
        set(FF_VERSION_NUMBER "${CMAKE_MATCH_1}")
        set(FF_VERSION_STRING "${FF_VERSION_STRING}")
    # - Version supplied from VERSION file (release tarballs), but ignore file if is a git repo
    elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../../FF_VERSION_NUMBER AND NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/../../.git)
        file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/../../FF_VERSION_NUMBER FF_VERSION_NUMBER LIMIT_COUNT 1)
        set(FF_VERSION_NUMBER "${FF_VERSION_NUMBER}")
        file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/../../FF_VERSION_STRING FF_VERSION_STRING LIMIT_COUNT 1)
        set(FF_VERSION_STRING "${FF_VERSION_STRING}")
    # - Version from git
    else()
        execute_process(
            COMMAND git -C ${CMAKE_CURRENT_LIST_DIR} describe --dirty --tags --always --exact-match --all --long
            OUTPUT_VARIABLE CMD_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # tags/fp_9.16.0-0-g9db8f03                 at tag, clean
        # tags/fp_9.16.0-0-g9db8f03-dirty           at tag, dirty
        # heads/integ/master-0-geec9255             at commit, clean
        # heads/integ/master-0-geec9255-dirty       at commit, dirty

        # Anything dirty: version number 0.0.0 and string "0.0.0-dirty"
        if("${CMD_OUTPUT}" MATCHES "-dirty")
            set(FF_VERSION_NUMBER 0.0.0)
            set(FF_VERSION_STRING "${FF_VERSION_NUMBER}-${CMD_OUTPUT}")
        # Tags that look like a version: version number x.y.z and string "x.y.z"
        elseif("${CMD_OUTPUT}" MATCHES "^tags/([a-z0-9]+_|v|)(0|[1-9][0-9]*).(0|[1-9][0-9]*).(0|[1-9][0-9]*)")
            set(FF_VERSION_NUMBER "${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.${CMAKE_MATCH_4}")
            set(FF_VERSION_STRING "${FF_VERSION_NUMBER}")
        # Anything else: version number 0.0.0 and string "0.0.0-whatevergitsaid"
        elseif(NOT "${CMD_OUTPUT}" STREQUAL "")
            set(FF_VERSION_NUMBER 0.0.0)
            set(FF_VERSION_STRING "${FF_VERSION_NUMBER}-${CMD_OUTPUT}")
        # Git failed: version number 0.0.0 and string "0.0.0-dev"
        else()
            set(FF_VERSION_NUMBER 0.0.0)
            set(FF_VERSION_STRING "${FF_VERSION_NUMBER}-dev")
        endif()
    endif()

    add_compile_definitions(FF_VERSION_NUMBER="${FF_VERSION_NUMBER}")
    add_compile_definitions(FF_VERSION_STRING="${FF_VERSION_STRING}")

    # write version info to file
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/FF_VERSION_NUMBER "${FF_VERSION_NUMBER}")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/FF_VERSION_STRING "${FF_VERSION_STRING}")

    set(FF_VERSION_IS_SET ON)
endif()


########################################################################################################################
# Some debugging
message(STATUS "ff: CMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}")
message(STATUS "ff: CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
message(STATUS "ff: CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message(STATUS "ff: FF_VERSION_NUMBER=${FF_VERSION_NUMBER}")
message(STATUS "ff: FF_VERSION_STRING=${FF_VERSION_STRING}")


########################################################################################################################
