#ifndef BACK_END_SPU_H
#define BACK_END_SPU_H

#include "Tree/Tree.h"
#include "Tree/NameTable/NameTable.h"

void AsmCodeBuild(Tree* tree, FILE* outStream, FILE* outBinStream);

#endif