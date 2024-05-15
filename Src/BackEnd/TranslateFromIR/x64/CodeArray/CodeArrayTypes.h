#ifndef CODE_ARRAY_TYPES_H
#define CODE_ARRAY_TYPES_H

/// @file 
/// @brief Contains types for stack

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>

typedef uint8_t CodeArrayValue;

/// @brief Chosen CODE_VALUE_POISON value for stack
static const CodeArrayValue CODE_VALUE_POISON = 0xDEAD;

#endif // TYPES_H