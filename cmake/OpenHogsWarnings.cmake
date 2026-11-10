# Helper that applies the project's warning level to a target.
#
# Usage: openhogs_set_warnings(<target>)
#
# Warnings are kept high on purpose: they catch many bugs in binary-parsing
# code (sign conversions, truncations) before they turn into crashes.

function(openhogs_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
    endif()
endfunction()
