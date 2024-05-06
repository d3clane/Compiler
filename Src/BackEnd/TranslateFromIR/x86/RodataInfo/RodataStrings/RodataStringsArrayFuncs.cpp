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
    
    if (firstBorder > secondBorder)
        RodataStringsSwap(&firstBorder, &secondBorder, sizeof(firstBorder));

    for (RodataStringsValue* arrIterator = firstBorder; arrIterator < secondBorder; ++arrIterator)
        *arrIterator = value;
}

void RodataStringsSwap(void* const element1, void* const element2, const size_t elemSize)
{
    assert(element1);
    assert(element2);
    assert(elemSize > 0);

    static const size_t bufferSz = 128;
    static char buffer[bufferSz] =  "";

    size_t elemLeftToSwap = elemSize;
    char* elem1 = (char*)element1;
    char* elem2 = (char*)element2;
    while (elemLeftToSwap > 0)
    {
        assert(buffer);
        assert(elem1);
        memcpy(buffer, elem1, bufferSz);
    
        assert(elem2);
        assert(elem1);
        memcpy(elem1, elem2, bufferSz);

        assert(elem2);
        assert(buffer);
        memcpy(elem2, buffer, bufferSz);

        elemLeftToSwap -= bufferSz;
        elem1 += bufferSz;
        elem2 += bufferSz;
    }
}
