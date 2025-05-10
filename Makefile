CC := clang

CFLAGS += --std=c99
CFLAGS += -Wall -Wvla
CFLAGS += -Iinclude/

LDFLAGS =

build_dir := build
source_dir := source

include config.mk

sources := ${source_dir}/argpx.c

ifeq (${debug}, true)
# -Og is still missing something
CFLAGS += -O0 -g
else
CFLAGS += -O3
endif

objects := $(sources:${source_dir}/%.c=${build_dir}/%.o)

.PHONY: all
all: ${build_dir}/libargparsex.a ${build_dir}/libargparsex.so

${build_dir}/libargparsex.a: ${objects}
	ar -rc $@ $^

${build_dir}/libargparsex.so: ${objects}
	${CC} -shared -o $@ ${LDFLAGS} $^

${build_dir}/%.o: ${source_dir}/%.c
	${CC} -c -o $@ ${CFLAGS} $<

.PHONY: clean
clean:
	rm build/*.o build/libargparsex.so build/libargparsex.a
