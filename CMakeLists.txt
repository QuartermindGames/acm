cmake_minimum_required(VERSION 3.23)

project(acm
        DESCRIPTION "ACM (Ape Config Markup)."
        LANGUAGES C
)

set(CMAKE_C_STANDARD 11)

file(GLOB ACM_SOURCE_FILES
        src/acm.c
        src/acm_lexer.c
        src/acm_parser.c
)

add_library(acm STATIC ${ACM_SOURCE_FILES})
target_include_directories(acm PUBLIC include)
