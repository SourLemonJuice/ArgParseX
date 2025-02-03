CC = clang

CFLAG += -std=c99 -O0 -g
CFLAG += -Wall -Wvla
CFLAG += -I../include/

root := ..

all: test.out

test.out: test.c ${root}/build/libargparsex.a
	${CC} ${CFLAG} \
		-o test.out \
		$^

${root}/build/libargparsex.a: ${root}/source/*.c
	cd ${root} && make

.PHONY: clean
clean:
	rm *.out
