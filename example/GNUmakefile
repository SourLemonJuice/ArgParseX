clang_flag += -std=c99
clang_flag += -I../
clang_flag += -O0 -g

default: test.out

test.out: test.c ../parser.c
	clang test.c ../parser.c \
		--output test.out \
		${clang_flag}
