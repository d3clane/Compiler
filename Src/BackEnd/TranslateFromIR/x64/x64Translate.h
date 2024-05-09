#ifndef X86_TRANSLATE_H
#define X86_TRANSLATE_H

#include <stdio.h>
#include "BackEnd/IR/IRList/IR.h"

void TranslateToX64(const IR* ir, FILE* outStream);

#endif