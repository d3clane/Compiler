#include "Rodata.h"

RodataInfo RodataInfoCtor()
{
    RodataInfo info = {};
    RodataImmediatesCtor(&info.rodataImmediates);
    RodataStringsCtor   (&info.rodataStrings);

    return info;
}

void RodataInfoDtor(RodataInfo* info)
{
    assert(info);

    RodataImmediatesDtor(info->rodataImmediates);
    RodataStringsDtor   (info->rodataStrings);

    info->rodataImmediates = nullptr;
    info->rodataStrings    = nullptr;
}