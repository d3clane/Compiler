#ifndef RODATA_IMMEDIATES_TYPES_H
#define RODATA_IMMEDIATES_TYPES_H

/// @file 
/// @brief Contains types for stack

#include <assert.h>
#include <math.h>
#include <stdio.h>

struct RodataImmediatesValue
{
    long long imm;

};

/// @brief Chosen RODATA_IMMEDIATES_POISON value for stack
static const RodataImmediatesValue RODATA_IMMEDIATES_POISON = {};

#endif // TYPES_H