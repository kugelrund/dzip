add_library(dzip_zlib
    zlib/adler32.c
    zlib/crc32.c
    zlib/deflate.c
    zlib/inffast.c
    zlib/inflate.c
    zlib/inftrees.c
    zlib/trees.c
    zlib/zutil.c
)
target_include_directories(dzip_zlib PUBLIC
    zlib
)
