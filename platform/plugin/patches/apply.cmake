# Apply a patch to a fetched dependency once (FetchContent's PATCH_COMMAND, run in the dependency's source folder):
# skipped when it is already in, so a reconfigure doesn't fail. cmake -DPATCH=<file> -P apply.cmake
execute_process(COMMAND git apply --reverse --check "${PATCH}" RESULT_VARIABLE already OUTPUT_QUIET ERROR_QUIET)
if (already EQUAL 0)
    message(STATUS "patch already in: ${PATCH}")
    return()
endif()
execute_process(COMMAND git apply --whitespace=nowarn "${PATCH}" RESULT_VARIABLE failed)
if (NOT failed EQUAL 0)
    message(FATAL_ERROR "patch does not apply: ${PATCH}")
endif()
message(STATUS "patched: ${PATCH}")
