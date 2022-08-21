cmake_minimum_required(VERSION 3.1.0)
project(dzip)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/install
        CACHE PATH "Default installation directory." FORCE)
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
target_link_libraries(dzip PRIVATE dzip_warnings dzip_zlib)
install(TARGETS dzip DESTINATION .)
