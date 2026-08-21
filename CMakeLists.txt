cmake_minimum_required(VERSION 3.20)

project(linux-shell VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(READLINE REQUIRED IMPORTED_TARGET readline)

file(GLOB SHELL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(linux-shell ${SHELL_SOURCES})
target_link_libraries(linux-shell PRIVATE PkgConfig::READLINE)
target_compile_options(linux-shell PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
if(BUILD_TESTING)
    add_executable(command-parser-tests
        tests/command_parser_tests.cpp
        src/command_parser.cpp
    )
    target_include_directories(command-parser-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_compile_options(command-parser-tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME command-parser-tests COMMAND command-parser-tests)
endif()
