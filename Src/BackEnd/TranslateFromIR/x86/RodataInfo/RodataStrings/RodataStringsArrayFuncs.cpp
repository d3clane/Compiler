#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "RodataStringsArrayFuncs.h"

void FillRodataStrings(RodataStringsValue* firstBorder, RodataStringsValue* secondBorder, const RodataStringsValue value)
{
    assert(firstBorder);
    assert(secondBorder);
    assert(firstBorder <= secondBorder);
    
    for (RodataStringsValue* arrIterator = firstBorder; arrIterator < secondBorder; ++arrIterator)
        *arrIterator = value;
}
