#ifndef X64_ENCODE_H
#define X64_ENCODE_H

#include <stdint.h>
#include "BackEnd/IR/IRList/IR.h"

#define DEF_X64_REG(REG, ...) REG,
enum class X64Register
{
    #include "x64RegistersDefs.h"
};
#undef DEF_X64_REG

#define DEF_X64_OP(NAME, ...) NAME,
enum class X64Operation
{
    #include "x64Operations.h"
};
#undef DEF_X64_OP

enum class X64OperandType
{
    REG,
    MEM,
    IMM,
};

struct X64OperandValue
{
    X64Register reg;
    int         imm;
};

struct X64Operand
{
    X64OperandValue value;
    X64OperandType  type;
};

X64Operand X64OperandRegCreate(X64Register reg);
X64Operand X64OperandImmCreate(int imm);
X64Operand X64OperandMemCreate(X64Register reg, int imm);

X64Operand      ConvertIRToX64Operand       (IROperand irOperand);
X64OperandValue ConvertIRToX64OperandValue  (IROperandValue value);
X64OperandType  ConvertIRToX64OperandType   (IROperandType type);
X64Register     ConvertIRToX64Register      (IRRegister reg);

uint8_t* EncodeX64(X64Operation operation, size_t numberOfOperands, 
                   X64Operand operand1, X64Operand operand2, 
                   size_t* outInstructionLen);

uint8_t* EncodeX64(X64Operation operation, X64Operand operand1, X64Operand operand2, 
                   size_t* outInstructionLen);

uint8_t* EncodeX64(X64Operation operation, X64Operand operand, size_t* outInstructionLen);

#endif 
