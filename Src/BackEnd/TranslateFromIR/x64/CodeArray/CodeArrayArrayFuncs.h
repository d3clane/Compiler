#ifndef CODE_ARRAY_ARRAY_FUNCS_H
#define CODE_ARRAY_ARRAY_FUNCS_H

#include "CodeArrayTypes.h"

/// @file
/// @brief Functions for working with array

/// @brief Fills array from one border to another with some value. CodeArrayValue is declared in CodeArraysTypes.h
///
/// @details If firstBorder > secondBorder function fills [secondBorder; firstBorder)
/// @details fills without right border.
/// @param [in]firstBorder border to fill from
/// @param [in]secondBorder border to fill to
/// @param [in]value value to fill with
void FillCodeArray(CodeArrayValue* firstBorder, CodeArrayValue* secondBorder, const CodeArrayValue value);

#endif // ARRAY_FUNCS_H
