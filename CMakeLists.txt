cmake_minimum_required(VERSION 3.20)

project(parser)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Werror

    -Wall
    -Wextra
    -Wpedantic

    -Wcast-align
    -Wcast-qual
    -Wconversion
    -Wctor-dtor-privacy
    -Wenum-compare
    -Wfloat-equal
    -Wnon-virtual-dtor
    # -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo
)

add_subdirectory(ssl_certs)

add_subdirectory(src)

# target_include_directories(data-parcer PRIVATE ssl_certs)
