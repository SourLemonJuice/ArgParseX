CC := clang

CFLAGS += --std=c99
CFLAGS += -O3
CFLAGS += -Wall -Wvla
CFLAGS += -Iinclude/

LDFLAGS =

build_dir := build
source_dir := source

include config.mk

sources := ${source_dir}/argpx.c

ifeq (${enable_hash}, true)
sources += ${source_dir}/argpx_hash.c
CFLAGS += -DARGPX_ENABLE_HASH
endif

ifeq (${enable_batch_alloc}, true)
CFLAGS += -DARGPX_ENABLE_BATCH_ALLOC
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
