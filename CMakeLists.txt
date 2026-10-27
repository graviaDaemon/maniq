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
        src/codegen.cpp
        src/codegen.h
        src/diagnostic.h
        src/parser.cpp
        src/parser.h
)
target_compile_definitions(maniq PRIVATE MANIQ_VERSION="${PROJECT_VERSION}")

enable_testing()
set(stages lex parse err types c run)
set(extensions tokens ast err types c out)

file(GLOB samples CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/samples/*.mq)
foreach (sample ${samples})
    get_filename_component(name ${sample} NAME_WE)
    foreach (stage extension IN ZIP_LISTS stages extensions)
        if (NOT name MATCHES "^v0-" AND NOT stage STREQUAL "lex")
            continue()
        endif ()
        add_test(NAME ${stage}/${name} COMMAND ${CMAKE_COMMAND}
                -DMANIQ=$<TARGET_FILE:maniq>
                -DSTAGE=${stage}
                -DSOURCE=${sample}
                -DGOLDEN=${CMAKE_SOURCE_DIR}/tests/${stage}/${name}.${extension}
                -DWORK=${CMAKE_BINARY_DIR}/tests/${stage}/${name}
                -P ${CMAKE_SOURCE_DIR}/tests/golden.cmake)
        set_tests_properties(${stage}/${name} PROPERTIES LABELS ${stage})
    endforeach ()
endforeach ()

file(GLOB goldens CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/*/*)
set(orphans "")
foreach (golden ${goldens})
    get_filename_component(name ${golden} NAME_WE)
    if (NOT EXISTS ${CMAKE_SOURCE_DIR}/samples/${name}.mq)
        list(APPEND orphans ${golden})
    endif ()
endforeach ()
string(REPLACE ";" "|" orphans "${orphans}")
add_test(NAME suite/orphans COMMAND ${CMAKE_COMMAND} -DSTAGE=orphans "-DORPHANS=${orphans}" -P ${CMAKE_SOURCE_DIR}/tests/golden.cmake)
