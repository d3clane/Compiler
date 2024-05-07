#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "LabelTableArrayFuncs.h"

void FillLabelTable(LabelTableValue* firstBorder, LabelTableValue* secondBorder, const LabelTableValue value)
{
    assert(firstBorder);
    assert(secondBorder);
    assert(firstBorder <= secondBorder);

    for (LabelTableValue* arrIterator = firstBorder; arrIterator < secondBorder; ++arrIterator)
        *arrIterator = value;
}
