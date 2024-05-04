#ifndef IR_REGISTER_H
#define IR_REGISTER_H

#include <stddef.h>

static const size_t XMM_REG_BYTE_SIZE = 16;
static const size_t RXX_REG_BYTE_SIZE = 8;

#define DEF_IR_REG(REG_NAME) REG_NAME,
enum class IRRegister
{
    #include "IRRegistersDefs.h"
};
#undef DEF_IR_REG

const char* IRRegisterGetName(IRRegister reg);

#endif