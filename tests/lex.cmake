execute_process(COMMAND ${MANIQ} lex ${SOURCE} OUTPUT_FILE ${ACTUAL} RESULT_VARIABLE exit_code)
if (exit_code)
    message(FATAL_ERROR "maniq lex ${SOURCE} exited with ${exit_code}")
endif ()

execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol ${ACTUAL} ${GOLDEN} RESULT_VARIABLE different)
if (different)
    message(FATAL_ERROR "Token dump differs.\n  expected: ${GOLDEN}\n  actual:   ${ACTUAL}")
endif ()
