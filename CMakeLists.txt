cmake_minimum_required(VERSION 3.28)
project(Maniq VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(maniq src/main.cpp
        src/lexer.cpp
        src/lexer.h
        src/ast.cpp
        src/ast.h
        src/checker.cpp
        src/checker.h
        src/diagnostic.h
        src/parser.cpp
        src/parser.h
)
target_compile_definitions(maniq PRIVATE MANIQ_VERSION="${PROJECT_VERSION}")

enable_testing()
function(golden_tests command folder extension)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${folder})
    file(GLOB GOLDEN CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/${folder}/*.${extension})
    foreach (golden ${GOLDEN})
        get_filename_component(name ${golden} NAME_WE)
        add_test(NAME ${command}/${name} COMMAND ${CMAKE_COMMAND}
                -DMANIQ=$<TARGET_FILE:maniq>
                -DCOMMAND=${command}
                -DSOURCE=${CMAKE_SOURCE_DIR}/samples/${name}.mq
                -DGOLDEN=${golden}
                -DACTUAL=${CMAKE_BINARY_DIR}/${folder}/${name}.${extension}
                -P ${CMAKE_SOURCE_DIR}/tests/golden.cmake)
    endforeach ()
endfunction()

golden_tests(lex lex tokens)
golden_tests(parse parse ast)
golden_tests(build err err)
golden_tests(types types types)
