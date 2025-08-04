cmake_minimum_required(VERSION 3.15)
project(dzip)

add_library(dzip_endian INTERFACE)
if(CMAKE_C_BYTE_ORDER STREQUAL "BIG_ENDIAN")
    target_compile_definitions(dzip_endian INTERFACE DZIP_BIG_ENDIAN)
endif()

add_library(dzip_warnings INTERFACE)
if(MSVC)
    target_compile_options(dzip_warnings INTERFACE /W4)
else()
    target_compile_options(dzip_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

option(DZIP_USE_SYSTEM_ZLIB "Use system zlib if found." ON)
if(DZIP_USE_SYSTEM_ZLIB)
    find_package(ZLIB)
endif()
if(ZLIB_FOUND)
    add_library(dzip_zlib INTERFACE)
    target_link_libraries(dzip_zlib INTERFACE ZLIB::ZLIB)
else()
    add_subdirectory(external)
endif()

if(MSVC)
    # GUI only supported on Windows with MSVC for now
    add_subdirectory(gui)
endif()

add_executable(dzip
    compress.c
    conmain.c
    crc32.c
    decode.c
    delete.c
    dzip.h
    dzipcon.h
    encode.c
    list.c
    main.c
    uncompress.c
    v1code.c
)
target_link_libraries(dzip PRIVATE dzip_endian dzip_warnings dzip_zlib)
install(TARGETS dzip DESTINATION bin)
