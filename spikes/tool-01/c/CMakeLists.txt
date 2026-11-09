# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
cmake_minimum_required(VERSION 3.20)
project(duospool C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(duospool STATIC src/core.c)
target_include_directories(duospool PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_options(duospool PRIVATE -Wall -Wextra -Wconversion -Werror)

# The manifest runner links the library and uses only the public duospool.h API. It is a test tool and is not installed.
add_executable(duospool-runner src/runner.c)
target_link_libraries(duospool-runner PRIVATE duospool)
target_compile_options(duospool-runner PRIVATE -Wall -Wextra -Wconversion -Werror)

install(TARGETS duospool ARCHIVE DESTINATION lib)
install(FILES include/duospool.h DESTINATION include)
configure_file(duospool.pc.in duospool.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/duospool.pc DESTINATION lib/pkgconfig)
