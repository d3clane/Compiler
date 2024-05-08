#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "RodataImmediatesArrayFuncs.h"

void FillRodataImmediates(RodataImmediatesValue* firstBorder, RodataImmediatesValue* secondBorder, const RodataImmediatesValue value)
{
    assert(firstBorder);
    assert(secondBorder);
    assert(firstBorder <= secondBorder);
    
    for (RodataImmediatesValue* arrIterator = firstBorder; arrIterator < secondBorder; ++arrIterator)
        *arrIterator = value;
}

