default: test.out

test.out: test.c ../parser.c
	clang test.c ../parser.c \
		--output test.out \
		-I../ \
		-O0 \
		-g
