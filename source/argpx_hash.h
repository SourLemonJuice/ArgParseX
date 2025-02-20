#ifndef ARGPX_HASH_H_
#define ARGPX_HASH_H_

#include <stdint.h>
#include <stdlib.h>

#define ARGPX_HASH_FNV1A_32_INIT ((uint32_t)0x811c9dc5)

#define ARGPX_HASH_FNV1A_32_PRIME ((uint32_t)0x01000193)

uint32_t ArgpxHashFnv1aB32(void *buf_in, size_t buf_size, uint32_t hash);
uint32_t ArgpxHashOffsetIntFnv1aB32(int offset, uint32_t hash);

#endif
