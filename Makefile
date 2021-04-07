# Makefile for linux

# Code does not work on 64-bit architectures at the moment. For example it
# implicitly assumes that sizeof(long) == 4. So only 32-bit compilation works
# for now. That's why we pass -m32.
CFLAGS = -m32 -Wall -O3
TARGET = dzip
OBJECTS = main.o compress.o uncompress.o list.o crc32.o \
	  encode.o decode.o v1code.o conmain.o delete.o \
	  external/zlib/adler32.o external/zlib/deflate.o external/zlib/trees.o \
	  external/zlib/inflate.o external/zlib/inftrees.o external/zlib/inffast.o \
	  external/zlib/zutil.o external/zlib/crc32.o

TMPFILES = gmon.out frag*

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(TMPFILES)

main.o: main.c dzip.h
compress.o: compress.c dzip.h dzipcon.h
uncompress.o: uncompress.c dzip.h dzipcon.h
crc32.o: crc32.c
encode.o: encode.c dzip.h
list.o: list.c dzip.h dzipcon.h
decode.o: decode.c dzip.h dzipcon.h
v1code.o: v1code.c dzip.h dzipcon.h