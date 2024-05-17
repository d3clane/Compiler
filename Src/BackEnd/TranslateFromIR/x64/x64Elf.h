#ifndef X64_ELF_H
#define X64_ELF_H

#include <stddef.h>

#include "RodataInfo/Rodata.h"
#include "CodeArray/CodeArray.h"

enum class StdLibAddresses
{
    ENTRY        = 0x401000,

    IN_FLOAT     = 0x401000,
    OUT_STRING   = 0x40110a,
    OUT_FLOAT    = 0x401153,
    HLT          = 0x4012e1,

    RODATA       = 0x402000,
};

enum class SegmentAddress
{
    STDLIB_CODE  = (int)StdLibAddresses::ENTRY,

    RODATA       = (int)StdLibAddresses::RODATA,

    PROGRAM_CODE = 0x403000,
};

void LoadCode   (CodeArrayType* code, FILE* outBinary);
void LoadRodata (RodataInfo* rodata,  FILE* outBinary);

#endif