#ifndef DEF_X64_OP
#define DEF_X64_OP(...)
#endif

#include "x64Encode.h"

// DEF_X64_OP(OP_NAME, ...)

// Vars : IRNode* node

DEF_X64_OP(NOP,
{
    I.opcode = 0x90;
    X64_INSTRUCTION_INIT(EMPTY_BYTE_TARGET, EMPTY_BYTE_TARGET);
})

DEF_X64_OP(PUSH,
{
    assert(numberOfOperands == 1 && operand1.type == X86OperandType::REG);
    
    I.opcode = 0x50;
    I.requireRegInOpcode = true;

    X64_INSTRUCTION_INIT(EMPTY_BYTE_TARGET())
    
    instruction = 
    {
        .
    }
        X64_INSTRUCTION_CREATE(0x50, EMPTY_BYTE_TARGET, EMPTY_BYTE_TARGET, 
        false, false, false, false, false, false, false, true, // Require reg in opcode
        EMPTY_BYTE, EMPTY_BYTE, EMPTY_BYTE, EMPTY_BYTE);
})

DEF_X64_OP(POP,
{
    assert(numberOfOperands == 1 && operand1.type == X64OperandType::REG);
    
    instruction = 
        X64_INSTRUCTION_CREATE(0x58, EMPTY_BYTE_TARGET, EMPTY_BYTE_TARGET, 
        false, false, false, false, false, false, false, true, // Require reg in opcode
        EMPTY_BYTE, EMPTY_BYTE, EMPTY_BYTE, EMPTY_BYTE);
})

DEF_X64_OP(MOV,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);

    instruction = 
        X64_INSTRUCTION_CREATE(0)
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
