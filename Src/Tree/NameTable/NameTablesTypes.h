#ifndef NAME_TABLE_TYPES_H
#define NAME_TABLE_TYPES_H

/// @file 
/// @brief Contains types for stack

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "BackEnd/IR/IRRegisters.h"

struct Name
{
    char* name;

    void*  localNameTable;

    IRRegister reg;    /// < register to address from 
    int        memShift; /// < shift relatively to register
};

/// @brief Chosen NAME_TABLE_POISON value for stack
static const Name NAME_TABLE_POISON = {};

#endif // TYPES_H