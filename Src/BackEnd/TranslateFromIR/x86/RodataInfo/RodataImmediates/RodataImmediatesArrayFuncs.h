#ifndef RODATA_IMMEDIATES_ARRAY_FUNCS_H
#define RODATA_IMMEDIATES_ARRAY_FUNCS_H

#include "RodataImmediatesTypes.h"

/// @file
/// @brief Functions for working with array

/// @brief Fills array from one border to another with some value. RodataImmediatesValue is declared in RodataImmediatessTypes.h
///
/// @details If firstBorder > secondBorder function fills [secondBorder; firstBorder)
/// @details fills without right border.
/// @param [in]firstBorder border to fill from
/// @param [in]secondBorder border to fill to
/// @param [in]value value to fill with
void FillRodataImmediates(RodataImmediatesValue* firstBorder, RodataImmediatesValue* secondBorder, const RodataImmediatesValue value);


/// @brief Swaps two elements
/// @param [out]element1 
/// @param [out]element2 
/// @param [in]elemSize size of one element for swapping
void RodataImmediatesSwap(void* const element1, void* const element2, const size_t elemSize);

#endif // ARRAY_FUNCS_H
