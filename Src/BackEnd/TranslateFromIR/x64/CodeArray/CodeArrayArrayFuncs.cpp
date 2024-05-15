#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "CodeArrayArrayFuncs.h"

void FillCodeArray(CodeArrayValue* firstBorder, CodeArrayValue* secondBorder, const CodeArrayValue value)
{
    assert(firstBorder);
    assert(secondBorder);
    assert(firstBorder <= secondBorder);
    
    for (CodeArrayValue* arrIterator = firstBorder; arrIterator < secondBorder; ++arrIterator)
        *arrIterator = value;
}

