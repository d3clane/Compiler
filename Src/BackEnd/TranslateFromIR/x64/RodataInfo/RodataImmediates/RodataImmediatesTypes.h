#ifndef RODATA_IMMEDIATES_TYPES_H
#define RODATA_IMMEDIATES_TYPES_H

/// @file 
/// @brief Contains types for stack

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

struct RodataImmediatesValue
{
    long long imm;
    char*     label;

    uint64_t  asmAddr;
};

/// @brief Chosen RODATA_IMMEDIATES_POISON value for stack
static const RodataImmediatesValue RODATA_IMMEDIATES_POISON = {};

#endif // TYPES_H