default: test.out

test.out: test.c
	clang test.c ../parser.c \
		--output test.out \
		-I../ \
		-O0 \
		-g
