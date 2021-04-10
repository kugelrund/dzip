add_library(DzipShlx SHARED
    dzipshlx.cpp
    dzipshlx.h
    dzipshlx.def
    resource.h

    dz.bmp
    dzipshlx.rc
)
target_compile_definitions(DzipShlx PRIVATE _USRDLL)
target_link_libraries(DzipShlx
    debug ucrtd.lib optimized ucrt.lib
    debug vcruntimed.lib optimized vcruntime.lib
)
install(TARGETS DzipShlx DESTINATION gui)
