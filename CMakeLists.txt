#
# Copyright 2017 Confluent Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra -Wno-deprecated -Wno-deprecated-declarations -O2 -fno-strict-aliasing")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_REENTRANT -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFUSE_USE_VERSION=26")

enable_testing()

# Define "make check" as an alias for "make test."
add_custom_target(check COMMAND ctest)

function(FLATTEN_LIST INPUT SEPARATOR OUTPUT)
    string (REPLACE ";" "${SEPARATOR}" _TMPS "${INPUT}")
    set (${OUTPUT} "${_TMPS}" PARENT_SCOPE)
endfunction()

# Find Linux FUSE
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE fuse)
IF(FUSE_FOUND)
    FLATTEN_LIST("${FUSE_CFLAGS}" " " FUSE_CFLAGS)
    FLATTEN_LIST("${FUSE_LDFLAGS}" " " FUSE_LDFLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FUSE_CFLAGS}")
    set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} ${FUSE_LDFLAGS}")
    MESSAGE(STATUS "Building Linux FUSE client.")
    include_directories(${FUSE_INCLUDE_DIRS})
ELSE(FUSE_FOUND)
    MESSAGE(FATAL_ERROR "Failed to find Linux FUSE libraries or include files.")
ENDIF(FUSE_FOUND)

find_program(VALGRIND_BINARY valgrind)
set(VALGRIND_OPTIONS --tool=memcheck --leak-check=full --trace-children=yes --error-exitcode=99)

add_executable(kibosh
    fault.c
    file.c
    fs.c
    io.c
    json.c
    log.c
    main.c
    meta.c
    pid.c
    test.c
    util.c
)
target_link_libraries(kibosh
    ${FUSE_LIBRARIES}
    m
    pthread
    rt
)

macro(add_utest utest)
    add_test(${utest} ${CMAKE_CURRENT_BINARY_DIR}/${utest} ${utest})
    if (VALGRIND_BINARY)
        add_test(valgrind_${utest} ${VALGRIND_BINARY} ${VALGRIND_OPTIONS} ${CMAKE_CURRENT_BINARY_DIR}/${utest} valgrind_${utest})
    endif (VALGRIND_BINARY)
endmacro(add_utest)

add_library(utest
    test.c
)
target_link_libraries(utest pthread)

add_executable(fault_unit
    fault.c
    fault_unit.c
    json.c
    log.c
    test.c
    util.c
)
target_link_libraries(fault_unit utest m)
add_utest(fault_unit)

add_executable(fs_test
    fs_test.c
    io.c
    log.c
    test.c
    util.c
)
target_link_libraries(fs_test utest)

add_executable(pid_unit
    io.c
    log.c
    pid.c
    pid_unit.c
    test.c
)
target_link_libraries(pid_unit utest m)
add_utest(pid_unit)

add_executable(util_unit
    log.c
    util_unit.c
    util.c
    test.c
)
target_link_libraries(util_unit utest)
add_utest(util_unit)

add_custom_target(
    copy_test_script ALL
    DEPENDS systest.sh
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/systest.sh" "${CMAKE_BINARY_DIR}/systest.sh"
)

find_program (BASH_PROGRAM bash)
if (BASH_PROGRAM)
    add_test(systest.sh ${BASH_PROGRAM} ${CMAKE_CURRENT_BINARY_DIR}/systest.sh)
endif (BASH_PROGRAM)
