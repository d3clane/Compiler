#ifndef IR_H
#define IR_H

#include <stddef.h>

#include "Tree/Tree.h"
#include "BackEnd/IR/IRRegisters.h"
#include "BackEnd/IR/IRList/IR.h"

IR* IRBuild(const Tree* tree);

//-----------------------------------------------

#endif