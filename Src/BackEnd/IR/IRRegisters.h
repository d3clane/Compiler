#ifndef IR_REGISTER_H
#define IR_REGISTER_H

static const size_t XMM_REG_BYTE_SIZE = 16;
static const size_t RXX_REG_BYTE_SIZE = 8;

enum class IRRegister
{
    NO_REG,

    RAX, RBX, RCX, RDX, 
    RSI, RDI,
    RBP, RSP, 
    R8, R9, R10, R11, R12, R13, R14, R15,

    XMM0, XMM1, XMM2,  XMM3, 
    XMM4, XMM5, XMM6,  XMM7,
};


#endif