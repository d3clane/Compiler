#ifndef LABEL_TABLE_TYPES_H
#define LABEL_TABLE_TYPES_H

/// @file 
/// @brief Contains types for stack

#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "BackEnd/IR/IR.h"

struct LabelTableValue
{
    const char*   label;
    IRNode*       connectedNode;
};

/// @brief Chosen LABEL_TABLE_POISON value for stack
static const LabelTableValue LABEL_TABLE_POISON = {};

#endif // TYPES_H