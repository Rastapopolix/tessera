# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis
#
# Tessera as a plugin (phase 0 spike): CLAP, VST3 and a standalone app from one CLAP implementation, through
# clap-wrapper (MIT). The CLAP SDK (MIT), clap-wrapper and the VST3 SDK (MIT since 3.8) are downloaded at configure
# time. The core is Tessera's core API (platform/core/tess_core.h) as the web simulator builds it (web/sim/sim.c, for
# its demos, over platform/core/core_unit.c).
#
#   cmake -S platform/plugin -B build/plugin -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/plugin --config Release
#
# Windows: the firmware sources need clang, so configure with  -T ClangCL  (Visual Studio's LLVM toolset).
cmake_minimum_required(VERSION 3.21)
project(tessera-plugin VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")   # static, as clap-wrapper builds its parts
set(TESS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
option(TESS_PLUGIN_TESTS "Build the render test (a minimal CLAP host; not on Windows)" ON)
# several instances: the plugin carries this many private copies of the core (core_copies.h), one per instance
set(TESS_PLUGIN_COPIES 8 CACHE STRING "Tesseras at once in one DAW project (1..16): each a copy of the core")

if (NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Tessera's firmware sources build with clang (on Windows: -T ClangCL)")
endif()

# the generated headers (build/gen): fonts, tables, the simulator's demos
# TESS_GEN OFF: build/gen is already there (on Windows, Pillow has no Raqm for the UI font: run gen.py in WSL, or
# take build/gen from the Linux CI job, as plugin.yml does)
option(TESS_GEN "Run platform/plugin/gen.py at configure time" ON)
if (TESS_GEN)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gen.py RESULT_VARIABLE TESS_GEN_RC)
    if (NOT TESS_GEN_RC EQUAL 0)
        message(FATAL_ERROR "platform/plugin/gen.py failed (needs Python 3 with Pillow (with Raqm) and fontTools)")
    endif()
elseif (NOT EXISTS ${TESS_ROOT}/build/gen/sim_demos.h OR NOT EXISTS ${TESS_ROOT}/build/gen/core_syms.h)
    message(FATAL_ERROR "TESS_GEN is OFF but build/gen has no generated headers: run platform/plugin/gen.py first")
endif()

include(FetchContent)
FetchContent_Declare(clap GIT_REPOSITORY https://github.com/free-audio/clap GIT_TAG 1.2.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(clap)
set(CLAP_WRAPPER_DOWNLOAD_DEPENDENCIES ON CACHE BOOL "" FORCE)
FetchContent_Declare(clap-wrapper GIT_REPOSITORY https://github.com/free-audio/clap-wrapper GIT_TAG v0.17.0
                     GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(clap-wrapper)

# ---- the core: the firmware, as web/sim/build.sh builds it for the browser, with the plugin's options (core_defs.txt,
# which gen.py reads too). tessera-core: one copy under its own names (the panel and sync tests); tessera-copies: the
# plugin's TESS_PLUGIN_COPIES private copies (core_copy.c, each with the prefix tc<k>_), one per instance
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/core_defs.txt TESS_CORE_DEFS)
function(tess_core_options tgt)
    target_include_directories(${tgt} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${TESS_ROOT}/platform/core
        PRIVATE ${TESS_ROOT}/build/gen ${TESS_ROOT}/firmware/src)
    if (WIN32)
        target_include_directories(${tgt} BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/compat)
    endif()
    target_compile_definitions(${tgt} PRIVATE ${TESS_CORE_DEFS})
    set_target_properties(${tgt} PROPERTIES POSITION_INDEPENDENT_CODE ON C_VISIBILITY_PRESET hidden)
    if (MSVC)
        target_compile_options(${tgt} PRIVATE -w /fp:precise -clang:-ffp-contract=off)
    else()
        target_compile_options(${tgt} PRIVATE -w -ffp-contract=off)
    endif()
endfunction()
add_library(tessera-core STATIC ${TESS_ROOT}/web/sim/sim.c)
tess_core_options(tessera-core)

if (TESS_PLUGIN_COPIES LESS 1 OR TESS_PLUGIN_COPIES GREATER 16)
    message(FATAL_ERROR "TESS_PLUGIN_COPIES: 1 to 16")
endif()
set(TESS_COPY_OBJECTS "")
math(EXPR TESS_COPY_LAST "${TESS_PLUGIN_COPIES} - 1")
foreach (k RANGE ${TESS_COPY_LAST})
    add_library(tessera-core-copy${k} OBJECT core_copy.c)
    tess_core_options(tessera-core-copy${k})
    target_compile_definitions(tessera-core-copy${k} PRIVATE TESS_PFX=tc${k}_)
    list(APPEND TESS_COPY_OBJECTS $<TARGET_OBJECTS:tessera-core-copy${k}>)
endforeach()
add_library(tessera-copies STATIC core_copies.c ${TESS_COPY_OBJECTS})
target_include_directories(tessera-copies PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${TESS_ROOT}/platform/core)
target_compile_definitions(tessera-copies PRIVATE TESS_PLUGIN_COPIES=${TESS_PLUGIN_COPIES})
set_target_properties(tessera-copies PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- the CLAP implementation, then every format from it
# the window: the panel drawn in software, in a native child window (Win32, X11; none on macOS yet)
add_library(tessera-panel STATIC panel.cpp)   # (the core's types only: it calls no core)
target_include_directories(tessera-panel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${TESS_ROOT}/platform/core
    PRIVATE ${TESS_ROOT}/firmware/src)

if (WIN32)
    set(TESS_GUI gui_win32.cpp)
elseif (APPLE)
    set(TESS_GUI gui_none.cpp)
else()
    set(TESS_GUI gui_x11.cpp)
endif()
add_library(tessera-impl STATIC tessera_clap.cpp ${TESS_GUI})
target_link_libraries(tessera-impl PUBLIC clap tessera-copies tessera-panel)
if (UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(tessera-impl PUBLIC X11::X11)
endif()

if (WIN32)
    set(TESS_FORMATS CLAP VST3)
    set(TESS_STANDALONE standalone "Tessera" "io.github.rastapopolix.tessera")
elseif (APPLE)
    set(TESS_FORMATS CLAP VST3 AUV2)
    set(TESS_STANDALONE standalone "Tessera" "io.github.rastapopolix.tessera")
else()
    set(TESS_FORMATS CLAP VST3)   # (a Linux standalone needs ALSA / JACK headers: not in the spike)
    set(TESS_STANDALONE "")
endif()

make_clapfirst_plugins(
    TARGET_NAME tessera
    IMPL_TARGET tessera-impl
    OUTPUT_NAME "Tessera"
    ENTRY_SOURCE tessera_entry.cpp
    BUNDLE_IDENTIFIER "io.github.rastapopolix.tessera"
    BUNDLE_VERSION ${PROJECT_VERSION}
    COPY_AFTER_BUILD FALSE
    PLUGIN_FORMATS ${TESS_FORMATS}
    ASSET_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out
    AUV2_MANUFACTURER_NAME "Tessera"
    AUV2_MANUFACTURER_CODE "Tess"
    AUV2_SUBTYPE_CODE "Tsra"
    AUV2_INSTRUMENT_TYPE "aumu"
    STANDALONE_CONFIGURATIONS ${TESS_STANDALONE}
)

# ---- the render test: loads the built .clap as a host would
if (TESS_PLUGIN_TESTS AND NOT WIN32)
    add_executable(tessera-render-test render_test.cpp)
    target_link_libraries(tessera-render-test PRIVATE clap ${CMAKE_DL_LIBS})
    if (NOT APPLE)
        target_link_libraries(tessera-render-test PRIVATE X11::X11)
    endif()
    add_dependencies(tessera-render-test tessera_clap)
endif()

# ---- the window's test: the core and the panel, no host (every platform)
if (TESS_PLUGIN_TESTS)
    add_executable(tessera-panel-test panel_test.cpp)
    target_link_libraries(tessera-panel-test PRIVATE tessera-panel tessera-core)
    find_package(Threads REQUIRED)
    add_executable(tessera-copies-test copies_test.cpp)
    target_link_libraries(tessera-copies-test PRIVATE tessera-copies Threads::Threads)
    add_executable(tessera-sync-test sync_test.cpp)
    target_link_libraries(tessera-sync-test PRIVATE tessera-core)
endif()
