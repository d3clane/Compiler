#ifndef DEF_X64_OP
#define DEF_X64_OP(...)
#endif

#include "x64Encode.h"

// DEF_X64_OP(OP_NAME, ...)

// Vars : IRNode* node

DEF_X64_OP(NOP,
{
    X64InstructionCreate(0x90, 
                         false, )
    return 
    X64InstructionCreate(0x90, 
                       false, false, false, false, false, false, false, false, 
                       EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY);
})

DEF_X64_OP(PUSH,
{
    assert(numberOfOperands == 1 && operand1.type == X86OperandType::REG);
    X64InstructionCreate(0x50 | )
})

DEF_X64_OP(POP,
{
})

DEF_X64_OP(MOV,
{
})

DEF_X64_OP(ADD,
{
})

DEF_X64_OP(SUB,
{
})

DEF_X64_OP(ADDSD,
{
})

DEF_X64_OP(SUBSD,
{
})

DEF_X64_OP(MULSD,
{
})

DEF_X64_OP(DIVSD,
{
})

DEF_X64_OP(PXOR,
{
})

DEF_X64_OP(ANDPD,
{
})

DEF_X64_OP(ORPD,
{
})

DEF_X64_OP(SQRTPD,
{
})

/*DEF_X64_OP(F_SIN,
{
    assert(false); // TODO
})

DEF_X64_OP(F_COS,
{
    assert(false); // TODO
})

DEF_X64_OP(F_TAN,
{
    assert(false); // TODO
})

DEF_X64_OP(F_COT,
{
    assert(false); // TODO
})*/

DEF_X64_OP(MOVSD,
{
})

DEF_X64_OP(COMISD,
{
})

DEF_X64_OP(JMP,
{
})

DEF_X64_OP(JE,
{
})

DEF_X64_OP(JNE,
{
})

DEF_X64_OP(JB,
{
})

DEF_X64_OP(JBE,
{
})

DEF_X64_OP(JA,
{
})

DEF_X64_OP(JAE,
{
})

DEF_X64_OP(CALL,
{
})

DEF_X64_OP(RET,
{
})

DEF_X64_OP(LEA,
{
})
