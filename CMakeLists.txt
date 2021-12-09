cmake_minimum_required(VERSION 3.8)

if(DEFINED PROJECT_NAME)
    set(SUBPROJECT ON)
endif()

project(qtpromise VERSION 0.6.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Qt5 5.6.0 REQUIRED COMPONENTS Core)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(qtpromise INTERFACE)
add_library(qtpromise::qtpromise ALIAS qtpromise)

target_link_libraries(qtpromise INTERFACE Qt5::Core)
target_include_directories(qtpromise INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include")

add_definitions(
    -DQT_DEPRECATED_WARNINGS
    -DQT_NO_KEYWORDS
)

# https://github.com/simonbrunel/qtpromise/issues/10
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
    add_compile_options(
        -Werror
        -Wpedantic
        -Wall
        -Wextra
        -Wconversion
        -Wsign-conversion
        -Wold-style-cast
        -Wunused-local-typedefs
        -pedantic-errors
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # https://docs.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level
    add_compile_options(
        /WX
    )
endif()

if(NOT SUBPROJECT)
    enable_testing()
    add_subdirectory(tests)
endif()
