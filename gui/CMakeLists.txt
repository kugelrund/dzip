add_subdirectory(dzip_dz)
add_subdirectory(dzip_pak)

enable_language(ASM_MASM)
add_executable(DzipGui
    ../crc32.c
    add.c
    common.c
    common.h
    deleteg.c
    dllcode.c
    extract.c
    extract.h
    file.c
    file.h
    gui_export.h
    gui_import.h
    listview.c
    listview.h
    lvhelper.asm
    lvhelper.h
    menu.c
    menu.h
    misc.c
    misc.h
    oledd.cpp
    options.c
    options.h
    recent.c
    recent.h
    resource.h
    sbar.c
    sbar.h
    thread.c
    thread.h
    toolbar.c
    view.c
    winmain.c
    wndproc.c

    arrow1.bmp
    arrow2.bmp
    dzipgui.ico
    tbbig.dat
    tbsmall.bmp
    dzipgui.ico
    dzipgui.rc
)
set_target_properties(DzipGui PROPERTIES WIN32_EXECUTABLE 1)
target_compile_definitions(DzipGui PRIVATE GUI)
target_link_libraries(DzipGui PRIVATE dzip_zlib comctl32)
# need to set this for the asm source file
target_link_options(DzipGui PUBLIC "/SAFESEH:NO")

install(TARGETS DzipGui DESTINATION gui)
