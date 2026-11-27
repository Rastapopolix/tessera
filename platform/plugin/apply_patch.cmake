# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Theo Dainis
# FetchContent's PATCH_COMMAND: apply PATCH in the current directory once (a source already patched, as a configure
# run again finds it, is left as it is). cmake -DPATCH=<file> -P apply_patch.cmake
execute_process(COMMAND git apply --reverse --check --ignore-whitespace "${PATCH}" RESULT_VARIABLE done
                OUTPUT_QUIET ERROR_QUIET)
if (NOT done EQUAL 0)
    execute_process(COMMAND git apply --ignore-whitespace "${PATCH}" RESULT_VARIABLE r)
    if (NOT r EQUAL 0)
        message(FATAL_ERROR "could not apply ${PATCH}")
    endif()
endif()
