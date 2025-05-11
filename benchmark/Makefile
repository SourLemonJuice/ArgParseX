CC := clang

CFLAGS += -std=c99 -O3 -g
CFLAGS += -Wall -Wvla
CFLAGS += -I../include/

root := ..

all: bm_getopt.out bm_argpx.out

bm_getopt.out: bm_getopt.c
	${CC} ${CFLAGS} \
		-o bm_getopt.out \
		$^

bm_argpx.out: bm_argpx.c ${root}/build/libargparsex.a
	${CC} ${CFLAGS} \
		-o bm_argpx.out \
		$^

${root}/build/libargparsex.a: ${root}/source/*.c
	cd ${root} && make

.PHONY: clean
clean:
	-rm *.out *.o
