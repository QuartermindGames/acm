set(CMAKE_C_STANDARD 11)

file(GLOB ACM_SOURCE_FILES
        src/acm.c
        src/acm_lexer.c
        src/acm_parser.c
)

add_library(acm STATIC ${ACM_SOURCE_FILES})
target_include_directories(acm PUBLIC include)
target_link_libraries(acm plcore)

#[[TODO: this needs a proper test suite!
add_executable(acm_test ${ACM_SOURCE_FILES})
target_compile_definitions(acm_test PRIVATE ACM_TEST)
target_link_libraries(acm_test acm)
]]
