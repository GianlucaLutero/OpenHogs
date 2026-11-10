# Helper that applies the project's warning level to a target.
#
# Usage: openhogs_set_warnings(<target>)
#
# Warnings are kept high on purpose: they catch many bugs in binary-parsing
# code (sign conversions, truncations) before they turn into crashes.
# Only OpenHogs targets use this, so third-party code (SDL, glad, Catch2)
# keeps its own settings and cannot break the build with its warnings.

option(OPENHOGS_WARNINGS_AS_ERRORS "Treat warnings in OpenHogs code as errors (used by CI)" OFF)

function(openhogs_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    endif()
    if(OPENHOGS_WARNINGS_AS_ERRORS)
        set_target_properties(${target} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
    endif()
endfunction()
