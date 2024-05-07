#ifndef ARRAY_FUNCS_H
#define ARRAY_FUNCS_H

#include "NameTablesTypes.h"

/// @file
/// @brief Functions for working with array

/// @brief Fills array from one border to another with some value. Name is declared in NameTablesTypes.h
///
/// @details If firstBorder > secondBorder function fills [secondBorder; firstBorder)
/// @details fills without right border.
/// @param [in]firstBorder border to fill from
/// @param [in]secondBorder border to fill to
/// @param [in]value value to fill with
void FillNameTable(Name* firstBorder, Name* secondBorder, const Name value);

#endif // ARRAY_FUNCS_H
