add_library(dzip_dz SHARED
    dllmain.c
    dz.c
    resource.h
    ../../compress.c
    ../../decode.c
    ../../delete.c
    ../../encode.c
    ../../main.c
    ../../uncompress.c
    ../gui_export.h
    ../gui_import.h

    dzip_dz.rc
)
target_compile_definitions(dzip_dz PRIVATE _USRDLL GUI)
target_link_libraries(dzip_dz PRIVATE
    dzip_zlib
    debug ucrtd.lib optimized ucrt.lib
    debug vcruntimed.lib optimized vcruntime.lib
)
install(TARGETS dzip_dz DESTINATION gui)
