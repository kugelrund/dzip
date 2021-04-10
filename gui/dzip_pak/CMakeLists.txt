add_library(dzip_pak SHARED
    dzip_pak.c
    dzip_pak.rc
    resource.h
    ../gui_export.h
    ../gui_import.h
)
target_compile_definitions(dzip_pak PRIVATE _USRDLL GUI)
target_link_libraries(dzip_pak
    debug ucrtd.lib optimized ucrt.lib
    debug vcruntimed.lib optimized vcruntime.lib
)
install(TARGETS dzip_pak DESTINATION gui)
