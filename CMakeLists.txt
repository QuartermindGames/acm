cmake_minimum_required(VERSION 3.23)

project(acm
        DESCRIPTION "ACM (Ape Config Markup)."
        LANGUAGES C
)

option(ACM_ENABLE_TESTS "Enable tests" ON)
if (ACM_ENABLE_TESTS)
    enable_testing()
endif ()

set(CMAKE_C_STANDARD 11)

file(GLOB ACM_SOURCE_FILES
        src/acm.c
        src/acm_lexer.c
        src/acm_parser.c
)

add_library(acm STATIC ${ACM_SOURCE_FILES})
target_include_directories(acm PUBLIC include)

#############################################
# Tests
#############################################
if (CMAKE_TESTING_ENABLED)
    add_executable(acm-tests ${ACM_SOURCE_FILES})
    target_compile_definitions(acm-tests PRIVATE ACM_TEST)
    target_include_directories(acm-tests PRIVATE include)
endif()
