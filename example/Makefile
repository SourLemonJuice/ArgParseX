CC = clang

CFLAG += -std=c99 -Og -g
CFLAG += -Wall -Wvla
CFLAG += -I../include/

all: test.out

test.out: test.c ../source/argpx.c ../source/argpx_hash.c
	${CC} ${CFLAG} \
		-o test.out \
		test.c ../source/argpx.c ../source/argpx_hash.c

.PHONY: clean
clean:
	rm -- *.out
