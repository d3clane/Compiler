#include <assert.h>

#include "RodataImmediatesHashFuncs.h"

HashType RodataImmediatesMurmurHash(const void* hashingArr, const size_t length, const uint64_t seed)
{
    assert(hashingArr);
    assert(length > 0);

    const uint64_t c1 = 0x5bd1e995; 
    const uint64_t c2 = 24;

    uint64_t hash = seed ^ (uint64_t)length;

    const unsigned char* data = (const unsigned char*)hashingArr;
    uint64_t word = 0;

    size_t len = length;
    while (len >= 4)
    {
        word  = data[0];
        word |= data[1] <<  8;
        word |= data[2] << 16;
        word |= data[3] << 24;

        word *= c1;
        word ^= word >> c2;
        word *= c1;

        hash *= c1;
        hash ^= word;

        data += 4;
        len -= 4;
    }

    assert(len < 4);

    switch (len)
    { 
        case 3:
            hash ^= data[2] << 16;
            // fall through
        case 2:
            hash ^= data[1] << 8;
            // fall through
        case 1:
            hash ^= data[0];
            hash *= c1;
            break;
        default:
            break;
    };

    hash ^= hash >> 13;
    hash *= c1;
    hash ^= hash >> 15;

    return hash;
}
