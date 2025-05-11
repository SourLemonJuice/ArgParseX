CC := clang

CFLAGS += -std=c99 -O0 -g
CFLAGS += -Wall -Wvla
CFLAGS += -I../include/

root := ..

all: test.out

test.out: test.c ${root}/build/libargparsex.a
	${CC} ${CFLAGS} \
		-o test.out \
		$^

${root}/build/libargparsex.a: ${root}/source/*.c
	cd ${root} && make

.PHONY: clean
clean:
	-rm *.out *.o
