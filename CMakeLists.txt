cmake_minimum_required(VERSION 3.16)
project(wkt_parser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Library
add_library(wkt_parser_lib STATIC
    src/lexer.cpp
    src/ast.cpp
    src/parser.cpp
    src/document.cpp
)

target_include_directories(wkt_parser_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Executable with tests
add_executable(wkt_parser src/main.cpp)
target_link_libraries(wkt_parser PRIVATE wkt_parser_lib)

# Single-header option for easy distribution
# Generate single header (optional, for convenience)
add_custom_target(single_header
    COMMAND ${CMAKE_COMMAND} -E echo "// WKT Parser - Single Header"
    COMMENT "Generating single header (not implemented, use include/wkt_parser.hpp)"
)

# Installation
install(TARGETS wkt_parser_lib
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(FILES include/wkt_parser.hpp
    DESTINATION include
)

install(TARGETS wkt_parser
    RUNTIME DESTINATION bin
)
