cmake_minimum_required(VERSION 3.28)
project(Maniq VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(maniq src/main.cpp
        src/lexer.cpp
        src/lexer.h
        src/ast.h)
target_compile_definitions(maniq PRIVATE MANIQ_VERSION="${PROJECT_VERSION}")

enable_testing()
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/lex)
file(GLOB LEX_GOLDEN CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/lex/*.tokens)
foreach (golden ${LEX_GOLDEN})
    get_filename_component(name ${golden} NAME_WE)
    add_test(NAME lex/${name} COMMAND ${CMAKE_COMMAND}
            -DMANIQ=$<TARGET_FILE:maniq>
            -DSOURCE=${CMAKE_SOURCE_DIR}/samples/${name}.mq
            -DGOLDEN=${golden}
            -DACTUAL=${CMAKE_BINARY_DIR}/lex/${name}.tokens
            -P ${CMAKE_SOURCE_DIR}/tests/lex.cmake)
endforeach ()
