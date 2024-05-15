#ifndef RODATA_H
#define RODATA_H

#include "RodataImmediates/RodataImmediates.h"
#include "RodataStrings/RodataStrings.h"

struct RodataInfo
{
    RodataImmediatesType* rodataImmediates;
    RodataStringsType*    rodataStrings;
};

RodataInfo RodataInfoCtor();
void       RodataInfoDtor(RodataInfo* info);

#endif