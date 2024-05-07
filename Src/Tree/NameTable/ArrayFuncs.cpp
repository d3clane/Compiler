#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ArrayFuncs.h"

void FillNameTable(Name* firstBorder, Name* secondBorder, const Name value)
{
    assert(firstBorder);
    assert(secondBorder);
    assert(firstBorder <= secondBorder);

    for (Name* arrIterator = firstBorder; arrIterator < secondBorder; ++arrIterator)
        *arrIterator = value;
}
