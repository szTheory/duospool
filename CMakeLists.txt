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

# The sanitize preset (D-41, D-83): the -fsanitize flags must reach the link line as well as the compile line, or the runtime is
# not linked. Only those flags are repeated at link time (a warning flag there would only draw unused-argument noise).
# DS_SANITIZE says that sanitizers are on: it adds the canary executable below, which exists to prove the sanitizers report.
set(DS_SANITIZE OFF CACHE BOOL "Sanitizers are on: build the planted canaries (ds_canary)")
separate_arguments(DS_EXTRA_LIST NATIVE_COMMAND "${DS_EXTRA_FLAGS}")
foreach(flag IN LISTS DS_EXTRA_LIST)
  if(flag MATCHES "^-fsanitize")
    add_link_options("${flag}")
  endif()
endforeach()

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

# Toy control libraries (D-85, D-55): one runner per tests/controls/NAME/RULE, each built from the ds_core sources with the one file
# named by the rule's file= line replaced by a generated variant of the live file. scripts/control-mutate writes that variant to
# ${CMAKE_BINARY_DIR}/controls/NAME/ before the build, so the source is marked GENERATED and need not exist at configure time. A
# toy is its own executable target (ds_toy_NAME), excluded from the default build, and never touches ds_core: it compiles the core
# sources again into its own objects. The headers still resolve through the private include directories of the toy.
file(GLOB DS_CONTROL_RULES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/controls/*/RULE)
foreach(rule IN LISTS DS_CONTROL_RULES)
  get_filename_component(rule_dir "${rule}" DIRECTORY)
  get_filename_component(name "${rule_dir}" NAME)
  # an edit of a rule (the file= line may change) reconfigures; the glob alone only sees a rule added or removed
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${rule}")
  file(STRINGS "${rule}" rule_file_lines REGEX "^file=")
  list(LENGTH rule_file_lines rule_file_count)
  if(NOT rule_file_count EQUAL 1)
    message(FATAL_ERROR "tests/controls/${name}/RULE must have exactly one file= line, found ${rule_file_count}")
  endif()
  string(REGEX REPLACE "^file=" "" rule_file "${rule_file_lines}")
  set(live_source "${CMAKE_CURRENT_SOURCE_DIR}/${rule_file}")
  if(NOT live_source IN_LIST DS_CORE_SOURCES)
    message(FATAL_ERROR "tests/controls/${name}/RULE names ${rule_file}, which is not a source of ds_core")
  endif()
  get_filename_component(rule_base "${rule_file}" NAME)
  set(toy_source "${CMAKE_BINARY_DIR}/controls/${name}/${rule_base}")
  set_source_files_properties("${toy_source}" PROPERTIES GENERATED TRUE)
  set(toy_core_sources ${DS_CORE_SOURCES})
  list(REMOVE_ITEM toy_core_sources "${live_source}")
  add_executable(ds_toy_${name} EXCLUDE_FROM_ALL
    tests/conformance/runner.c tests/conformance/profkinds.c tests/conformance/synth.c
    ${DS_TEXT_SOURCES} ${toy_core_sources} "${toy_source}")
  target_include_directories(ds_toy_${name} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core ${CMAKE_CURRENT_SOURCE_DIR}/src/text)
  target_compile_definitions(ds_toy_${name} PRIVATE "DS_BUILD_ID=\"${DS_BUILD_ID}\"")
endforeach()

# Planted canaries (D-55, D-83): ds_canary has three modes, each a defect that a live sanitizer must report (a heap overflow, a signed
# overflow, a signed left shift). It is built only when DS_SANITIZE is on, never links the core, and is run by conformance-run
# sanitize, which requires a non-zero exit and the sanitizer's own message from each mode.
if(DS_SANITIZE)
  add_executable(ds_canary tests/conformance/canary.c)
endif()
