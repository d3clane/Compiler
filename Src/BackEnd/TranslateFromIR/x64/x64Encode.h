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

        bool requireRegInOpcode         : 1;
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
    // TODO: think about requireRegInOpcode
    MODRM_REG,
    MODRM_RM,

    MODRM_RIP_ADDR,

    SIB,

    IMM32,
    IMM16,      ///< for RET instruction
};

X64Instruction X64InstructionCreate
(uint8_t opcode, size_t numberOfOperands, 
 X64Operand operand1, X64Operand operand2,
 X64OperandByteTarget operand1Target, X64OperandByteTarget operand2Target,
 bool requireMandatoryPrefix, bool requireREXByDefault, 
 bool requireOpcodePrefix1, bool requireOpcodePrefix2,
 bool requireDisp32, bool requireImm32, bool requireImm16, bool requireRegInOpcode,
 uint8_t mandatoryPrefix, uint8_t opcodePrefix1, uint8_t opcodePrefix2);

X64Operand      ConvertIRToX64Operand       (IROperand irOperand);
X64OperandValue ConvertIRToX64OperandValue  (IROperandValue value);
X64OperandType  ConvertIRToX64OperandType   (IROperandType type);
X64Register     ConvertIRToX64Register      (IRRegister reg);

#endif 
