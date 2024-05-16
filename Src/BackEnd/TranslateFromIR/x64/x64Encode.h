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

struct X64Instruction
{
    struct 
    {
        bool requireMandatoryPrefix     : 1;
        bool requireREX                 : 1;
        bool requireOpcodePrefix1       : 1;
        bool requireOpcodePrefix2       : 1;
        bool requireModRM               : 1;
        bool requireSIB                 : 1;
        bool requireDisp32              : 1;
        bool requireImm32               : 1;
        bool requireImm16               : 1;
    };

    uint8_t mandatoryPrefix;
    uint8_t rex;
    uint8_t opcodePrefix1;
    uint8_t opcodePrefix2;
    uint8_t opcode;
    uint8_t modRM;
    uint8_t sib;

    int32_t disp32;
    int32_t imm32;
    int16_t imm16;
};

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

enum class X64OperandByteTarget
{
    OPCODE,

    MODRM_REG,
    MODRM_RM,

    IMM32,
    IMM16,      ///< for RET instruction
};

void X64InstructionInit(X64Instruction* instruction, size_t numberOfOperands, 
                        X64Operand operand1, X64Operand operand2,
                        X64OperandByteTarget operand1Target, 
                        X64OperandByteTarget operand2Target);

X64Instruction X64InstructionCtor();

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
