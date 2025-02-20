#include <assert.h>

#include "argpx_hash.h"

uint32_t ArgpxHashFnv1aB32(void *buf_in, size_t buf_size, uint32_t hash)
{
    assert(buf_in != NULL);

    unsigned char *buf = (unsigned char *)buf_in;

    for (size_t i = 0; i < buf_size; i++) {
        hash ^= (uint32_t)buf[i];
        hash *= ARGPX_HASH_FNV1A_32_PRIME;
    }

    return hash;
}
