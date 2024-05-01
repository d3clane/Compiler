#ifndef NAME_TABLE_TYPES_H
#define NAME_TABLE_TYPES_H

/// @file 
/// @brief Contains types for stack

#include <assert.h>
#include <math.h>
#include <stdio.h>

struct Name
{
    char* name;

    void* localNameTable;
    size_t varRamId;
};

/// @brief Chosen NAME_TABLE_POISON value for stack
static const Name NAME_TABLE_POISON = {};

#endif // TYPES_H