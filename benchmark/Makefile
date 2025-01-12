CC:=/usr/bin/clang
CFLAGS+=-Wall -std=c99 -I../include/
CFLAGS+=-O3 -g
# CFLAGS+=-Og

argpx_src:=../source

all: bm_argpx.out bm_getopt.out

bm_argpx.out: argpx*.o bm_argpx.c
	${CC} argpx*.o bm_argpx.c \
		--output ./bm_argpx.out \
		${CFLAGS}

argpx.o: ${argpx_src}/*.c
	${CC} ${argpx_src}/*.c \
		-c \
		${CFLAGS}

bm_getopt.out: bm_getopt.c
	${CC} bm_getopt.c \
		--output ./bm_getopt.out \
		${CFLAGS}
