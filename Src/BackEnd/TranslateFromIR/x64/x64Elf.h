#ifndef X64_ELF_H
#define X64_ELF_H

#include <stddef.h>

enum class StdLibAddresses
{
    ENTRY        = 0x401000,

    IN_FLOAT     = 0x401000,
    OUT_STRING   = 0x40110a,
    OUT_FLOAT    = 0x401153,
    HLT          = 0x4012c3,

    RODATA       = 0x402000,
};

enum class SegmentAddress
{
    STDLIB_CODE  = (int)StdLibAddresses::ENTRY,

    RODATA       = (int)StdLibAddresses::RODATA,

    PROGRAM_CODE = 0x403000,
};

enum class SegmentFilePos
{
    STDLIB_CODE  = 0x1000,
    PROGRAM_CODE = 0x2000,

    RODATA       = 0x3000,
};

static const size_t StdLibFileSize = 748;


#endif