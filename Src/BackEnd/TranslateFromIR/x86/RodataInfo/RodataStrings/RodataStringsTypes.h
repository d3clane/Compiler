#ifndef RODATA_STRINGS_TYPES_H
#define RODATA_STRINGS_TYPES_H

/// @file 
/// @brief Contains types for stack

#include <assert.h>
#include <math.h>
#include <stdio.h>

struct RodataStringsValue
{
    char*      string;
};

/// @brief Chosen RODATA_STRINGS_POISON value for stack
static const RodataStringsValue RODATA_STRINGS_POISON = {};

#endif // TYPES_H