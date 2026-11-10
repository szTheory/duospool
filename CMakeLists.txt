# SPDX-License-Identifier: MIT OR Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 The DuoSpool Authors
#
# DuoSpool build (D-40, D-41). Three targets: ds_core, the freestanding machine core (src/core), ds_text, the canonical record
# printer (src/text, links ds_core, prints and never parses) and ds_runner, the conformance runner (tests/conformance, never
# installed). Dependencies point one way: ds_runner -> ds_text -> ds_core. There is no public header and no install rule (D-05):
# the runner reaches the core and the printer through the internal include directories src/core and src/text only.
# Compiler flags come from the presets in CMakePresets.json through two cache variables, so each preset adds to the shared
# warning set instead of replacing it: DS_WARNING_FLAGS (the warning set, in the hidden base preset) and DS_EXTRA_FLAGS
# (per preset: -fwrapv on the gate presets, the sanitizer flags on a later preset). A plain cmake call without a preset
# builds with the compiler defaults and no warning gate; the gate is the preset.
cmake_minimum_required(VERSION 3.25)
project(duospool C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(DS_WARNING_FLAGS "" CACHE STRING "Warning flags shared by every preset")
set(DS_EXTRA_FLAGS "" CACHE STRING "Per-preset flags (wrap semantics, sanitizers)")
separate_arguments(DS_FLAG_LIST NATIVE_COMMAND "${DS_WARNING_FLAGS} ${DS_EXTRA_FLAGS}")
add_compile_options(${DS_FLAG_LIST})

# the build id printed in the [core] table of a record; a build that does not set it prints the literal "unset" (D-53)
set(DS_BUILD_ID "unset" CACHE STRING "Build id printed in the run record")

file(GLOB DS_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c)
add_library(ds_core STATIC ${DS_CORE_SOURCES})
target_include_directories(ds_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/core)

file(GLOB DS_TEXT_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/text/*.c)
add_library(ds_text STATIC ${DS_TEXT_SOURCES})
target_include_directories(ds_text PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/text)
target_link_libraries(ds_text PUBLIC ds_core)
target_compile_definitions(ds_text PRIVATE "DS_BUILD_ID=\"${DS_BUILD_ID}\"")

add_executable(ds_runner tests/conformance/runner.c tests/conformance/profkinds.c tests/conformance/synth.c)
target_link_libraries(ds_runner PRIVATE ds_text ds_core)
