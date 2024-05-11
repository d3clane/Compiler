#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "x64Encode.h"

#define EMPTY 0

static const uint8_t MandatoryPrefix_66 = 0x66;
static const uint8_t MandatoryPrefix_F2 = 0xF2;
static const uint8_t MandatoryPrefix_F3 = 0xF3;

static const uint8_t OpcodePrefix1_0F   = 0x0F;
static const uint8_t OpcodePrefix2_38   = 0x38;
static const uint8_t OpcodePrefix2_3A   = 0x3A;

static inline void SetRexRequirement(X64Instruction* instruction, 
                                     size_t numberOfOperands,
                                     X64Operand operand1, X64Operand operand2);

static inline void SetRexRequirement(X64Instruction* instruction, X64Operand operand);

static inline void SetOperands(X64Instruction* instruction, size_t numberOfOperands,
                               X64Operand operand1, X64Operand operand2,
                               X64OperandByteTarget operand1Target, 
                               X64OperandByteTarget operand2Target);

static inline void SetOperand (X64Instruction* instruction, 
                               X64Operand operand1, X64OperandByteTarget operand1Target);

static inline void SetRex (X64Instruction* instruction); // Simply set rex to 0b01000000
static inline void SetRexB(X64Instruction* instruction, uint8_t bit);
static inline void SetRexW(X64Instruction* instruction, uint8_t bit);
static inline void SetRexR(X64Instruction* instruction, uint8_t bit);

#define EMPTY_BYTE_TARGET   X64OperandByteTarget::IMM32 // some default value
#define BYTE_TARGET(TARGET) X64OperandByteTarget::TARGET

X64Instruction X64InstructionInit(X64Instruction* instruction, size_t numberOfOperands, 
                                  X64Operand operand1, X64Operand operand2,
                                  X64OperandByteTarget operand1Target, 
                                  X64OperandByteTarget operand2Target)
{
    SetRexRequirement(instruction, numberOfOperands, operand1, operand2);

    SetOperands(instruction, numberOfOperands, operand1, operand2, 
                operand1Target, operand2Target);
}

X64Instruction X64InstructionInit(X64Operation operation, size_t numberOfOperands, 
                                  X64Operand operand1, X64Operand operand2)
{
    X64Instruction instruction = X64InstructionCtor();

#define DEF_X64_OP(OP, INSTRUCTION_INIT_CODE, ...)              \
    case X64Operation::OP:                                      \
        INSTRUCTION_INIT_CODE;                                  \
        break;

    switch (operation)
    {
        #include "x64Operations.h"

        default:    // Unreachable
            assert(false);
            break;
    }

    return instruction;
}

X64Instruction X64InstructionCtor()
{
    X64Instruction instruction = {};

#define SET_0(FIELD) instruction. ##FIELD = 0;
    SET_0(requireMandatoryPrefix);
    SET_0(requireREX);
    SET_0(requireOpcodePrefix1);
    SET_0(requireOpcodePrefix2);
    SET_0(requireModRM);
    SET_0(requireSIB);
    SET_0(requireDisp32);
    SET_0(requireImm32);
    SET_0(requireImm16);

    SET_0(mandatoryPrefix);
    SET_0(rex);
    SET_0(opcodePrefix1);
    SET_0(opcodePrefix2);
    SET_0(opcode);
    SET_0(modRM);
    SET_0(sib);
    SET_0(disp32);
    SET_0(imm32);
    SET_0(imm16);
#undef SET_0

    SetRex(&instruction);
    
    return instruction;
}


X64Operand ConvertIRToX64Operand(IROperand irOperand)
{
    X64Operand x64Operand = {};

    x64Operand.type  = ConvertIRToX64OperandType (irOperand.type);
    x64Operand.value = ConvertIRToX64OperandValue(irOperand.value);

    return x64Operand;
}

X64OperandType ConvertIRToX64OperandType(IROperandType type)
{
    X64OperandType x64Type = X64OperandType::IMM;

#define CASE(IR_TYPE)                               \
    case IROperandType::IR_TYPE:                    \
        x64Type = X64OperandType::IR_TYPE;          \
        break;

    switch (type)
    {
        CASE(IMM);
        CASE(REG);
        CASE(MEM);

        case IROperandType::STR:
            x64Type = X64OperandType::MEM; // string in rodata
            break;

        case IROperandType::LABEL:
            x64Type = X64OperandType::IMM; // labels are converted to offsets
            break;

        default:    // Unreachable
            assert(false);
            break;
    }
#undef CASE

    return x64Type;
}

X64OperandValue ConvertIRToX64OperandValue  (IROperandValue value)
{
    X64OperandValue x64Value = {};

    x64Value.imm = value.imm;
    x64Value.reg = ConvertIRToX64Register(value.reg);

    return x64Value;
}

X64Register     ConvertIRToX64Register      (IRRegister reg)
{
#define DEF_IR_REG(REG, ...)                \
    case IRRegister::REG:                   \
        return X64Register::REG;            \

    switch (reg)
    {
        #include "BackEnd/IR/IRRegistersDefs.h"

        default: // Unreachable
            assert(false);
            break;
    }
#undef DEF_IR_REG    

    // Unreachable

    assert(false);
    return X64Register::NO_REG;
}

static inline void SetRexRequirement(X64Instruction* instruction, 
                                     size_t numberOfOperands,
                                     X64Operand operand1, X64Operand operand2)
{
    assert(instruction);

    if (numberOfOperands == 0)
        return;

    if (instruction->requireREX)
        return;

    if (numberOfOperands > 0)
        SetRexRequirement(instruction, operand1);
    if (numberOfOperands > 1)
        SetRexRequirement(instruction, operand2);
}

static inline void SetRexRequirement(X64Instruction* instruction, X64Operand operand)
{
    assert(instruction);

    if (operand.value.reg == X64Register::NO_REG)
        return;
    
#define DEF_X64_REG(REG, LOW_BITS, HIGH_BIT, ...)       \
    case X64Register::REG:                              \
        if (HIGH_BIT) instruction->requireREX = true;   \
        break;

    switch (operand.value.reg)
    {
        #include "x64RegistersDefs.h"

        default:    // Unreachable
            assert(false);
            break;
    }     

#undef DEF_X64_REG
}

static inline void SetOperands(X64Instruction* instruction, size_t numberOfOperands,
                               X64Operand operand1, X64Operand operand2,
                               X64OperandByteTarget operand1Target,
                               X64OperandByteTarget operand2Target)
{
    assert(instruction);

    if (numberOfOperands == 0)
        return;
    
    if (operand1Target == BYTE_TARGET(OPCODE))
    {
        assert(numberOfOperands == 1);
        assert(operand1.type == X64OperandType::REG);
        assert(!instruction->requireModRM);
        assert(!instruction->requireSIB);

        SetRegInOpcode(instruction, operand1);

        return;
    }

    SetOperand(instruction, operand1, operand1Target);

    if (numberOfOperands > 1)
        SetOperand(instruction, operand2, operand2Target);
}

static inline void SetRegInOpcode(X64Instruction* instruction, X64Operand operand)
{
    assert(operand.type == X64OperandType::REG);

    static const size_t opcodeRegShift = 0;
#define DEF_X64_REG(REG, LOW_BITS, HIGH_BIT)                    \
    case X64Register::REG:                                      \
        instruction->opcode |= (LOW_BITS << opcodeRegShift);    \
        SetRexB(instruction, HIGH_BIT);                         \
        break;

    switch (operand.value.reg)
    {
        #include "x64RegistersDefs.h"
    
        default: // Unreachable
            assert(false);
            break;
    }
}

static inline void SetOperand (X64Instruction* instruction, 
                               X64Operand operand, X64OperandByteTarget operandTarget)
{
    assert(instruction);

    switch (operandTarget)
    {
        case BYTE_TARGET(MODRM_REG):
        {
            assert(operand.type == X64OperandType::REG);
            assert(instruction->requireModRM);

            SetModRmReg(instruction, operand);

            break;
        }

        case BYTE_TARGET(MODRM_RM):
        {
            assert(operand.type == X64OperandType::REG);
            assert(instruction->requireModRM);

            SetModRmRmToReg(instruction, operand);

            break;
        }

        case BYTE_TARGET(SIB):
        {
            assert(operand.type == X64OperandType::MEM);
            assert(instruction->requireModRM);
            assert(instruction->requireSIB);
            assert(instruction->requireDisp32);

            SetSibDisp32(instruction, operand);

            break;
        }
        
        case BYTE_TARGET(MODRM_RIP_ADDR):
        {
            assert(operand.type == X64OperandType::MEM); //TODO: подумать
            assert(instruction->requireModRM);
            assert(instruction->requireDisp32);

            SetRipAddressing(instruction, operand);

            break;
        }

        case BYTE_TARGET(IMM32):
        {
            assert(operand.type == X64OperandType::IMM);
            assert(instruction->requireImm32);

            SetImm32(instruction, operand);

            break;
        }

        case BYTE_TARGET(IMM16):
        {
            assert(operand.type == X64OperandType::IMM);
            assert(instruction->requireImm16);

            SetImm16(instruction, operand);

            break;
        }

        default: // Unreachable
            assert(false);
            break;
    }
}

static inline void SetModRmReg(X64Instruction* instruction, X64Operand operand)
{
#define DEF_X64_REG(REG, LOW_BITS, HIGH_BIT, ...)               \
    case X64Register::REG:                                      \
        SetModRmRegField(instruction, LOW_BITS);                \
        SetRexR(instruction, HIGH_BIT);                         \
        break;
    
    switch (operand.value.reg)
    {
        #include "x64RegistersDefs.h"
        
        default:    // Unreachable
            assert(false);
            break;
    }
}

static inline void SetModRmRegDirectAddressing(X64Instruction* instruction)
{
    static const uint8_t regDirectAddressingMod = 3; // 0b11

    SetModRmModField(instruction, regDirectAddressingMod);
}

static inline void SetModRmRmToReg(X64Instruction* instruction, X64Operand operand)
{
    SetModRmRegDirectAddressing(instruction);

    
#define DEF_X64_REG(REG, LOW_BITS, HIGH_BIT, ...)               \
    case X64Register::REG:                                      \
        SetModRmRmField(instruction, LOW_BITS);                 \
        SetRexB(instruction, HIGH_BIT);                         \
        break;

    switch (operand.value.reg)
    {
        #include "x64RegistersDefs.h"

        default: // Unreachable
            assert(false);
            break;
    }
}

static inline void SetModRmSibDisp32(X64Instruction* instruction)
{
    static const size_t sibDisp32Mod = 2; // 0b10

    SetModRmModField(instruction, sibDisp32Mod);
}

static inline void SetModRmModField(X64Instruction* instruction, uint8_t mod)
{
    static const size_t modFieldShift = 6;

    instruction->modRM |= (mod << modFieldShift);
}

static inline void SetModRmRegField(X64Instruction* instruction, uint8_t bits)
{
    static const size_t regFieldShift = 3;
    
    instruction->modRM |= (bits << regFieldShift);
}

static inline void SetSibDisp32(X64Instruction* instruction, X64Operand operand)
{
    SetModRmSibDisp32(instruction);

    static const uint8_t baseDisp32Index = 4; // 0100

    SetSibIndex(instruction, baseDisp32Index);

#define DEF_X64_REG(REG, LOW_BITS, HIGH_BIT, ...)               \
    case X64Register::REG:                                      \
        SetSibBase(instruction, LOW_BITS);                      \
        SetRexB(instruction, HIGH_BIT);                         \
        break;

    switch (operand.value.reg)
    {
        #include "x64RegistersDefs.h"
        
        default: // Unreachable
            assert(false);
            break;
    }

    SetDisp32(instruction, operand);
}

static inline void SetSibIndex(X64Instruction* instruction, uint8_t index)
{
    static const size_t indexFieldShift = 3;
    
    instruction->sib |= (index << indexFieldShift);
}

static inline void SetSibBase(X64Instruction* instruction, uint8_t base)
{
    static const size_t baseFieldShift = 0;

    instruction->sib |= (base << baseFieldShift);
}

static inline void SetDisp32(X64Instruction* instruction, X64Operand operand)
{
    instruction->disp32 = operand.value.imm;
}

static inline void SetModRmRipAddressing(X64Instruction* instruction)
{
    static uint8_t rmRipAddressing = 5; // 0b101;

    SetModRmRmField(instruction, rmRipAddressing);
}

static inline void SetModRmRmField(X64Instruction* instruction, uint8_t rmBits)
{
    static const size_t rmFieldShift = 0;

    instruction->modRM |= (rmBits << rmFieldShift);
}

static inline void SetRipAddressing(X64Instruction* instruction, X64Operand operand)
{
    SetModRmRipAddressing(instruction);
    SetDisp32(instruction, operand);
}

static inline void SetImm32(X64Instruction* instruction, X64Operand operand)
{
    instruction->imm32 = operand.value.imm;
}

static inline void SetImm16(X64Instruction* instruction, X64Operand operand)
{
    instruction->imm16 = (int16_t)operand.value.imm;
}

// TODO: копипаста. Ваще подумать что в функции можно передавать шифт как будто и избавиться от копипасты
// но ваще как будто разница тем что я меняю одну строчку с | на вызов функции ваще никакая???
static inline void SetRexB(X64Instruction* instruction, uint8_t bit)
{
    static const size_t rexBFieldShift = 0;

    instruction->rex |= (bit << rexBFieldShift);
}

static inline void SetRexW(X64Instruction* instruction, uint8_t bit)
{
    static const size_t rexWFieldShift = 3;
    
    instruction->rex |= (bit << rexWFieldShift);
}

static inline void SetRexR(X64Instruction* instruction, uint8_t bit)
{
    static const size_t rexRFieldShift = 2;
    
    instruction->rex |= (bit << rexRFieldShift);
}

//-----------------------------------------------------------------------------

#define X64_INSTRUCTION_INIT(OPERAND1_TARGET, OPERAND2_TARGET)              \
    X64InstructionInit(&instruction, numberOfOperands, operand1, operand2,  \
                       OPERAND1_TARGET, OPERAND2_TARGET)

uint8_t* EncodeX64(X64Operation operation, size_t numberOfOperands, 
                   X64Operand operand1, X64Operand operand2, 
                   size_t* outInstructionLen)
{
    X64Instruction instruction = X64InstructionInit(operation, numberOfOperands, 
                                                    operand1, operand2);

    static const size_t maxInstructionLen = 16;
    uint8_t* instructionBytes = (uint8_t*)calloc(maxInstructionLen, sizeof(*instructionBytes));

    size_t instructionLen = 0;

    if (instruction.requireMandatoryPrefix) 
        instructionBytes[instructionLen++] = instruction.mandatoryPrefix;
    if (instruction.requireREX)
        instructionBytes[instructionLen++] = instruction.rex;
    if (instruction.requireOpcodePrefix1)
        instructionBytes[instructionLen++] = instruction.opcodePrefix1;
    if (instruction.opcodePrefix2)
        instructionBytes[instructionLen++] = instruction.opcodePrefix2;

    instructionBytes[instructionLen++] = instruction.opcode;

    if (instruction.requireModRM)
        instructionBytes[instructionLen++] = instruction.modRM;
    if (instruction.requireSIB)
        instructionBytes[instructionLen++] = instruction.sib;
    if (instruction.requireDisp32)
    {
        memcpy(instructionBytes + instructionLen, &instruction.disp32, sizeof(instruction.disp32));
        instructionLen += sizeof(instruction.disp32);
    }
    if (instruction.requireImm16)
    {
        memcpy(instructionBytes + instructionLen, &instruction.imm16, sizeof(instruction.imm16));
        instructionLen += sizeof(instruction.imm16);
    }
    if (instruction.requireImm32)
    {
        memcpy(instructionBytes + instructionLen, &instruction.imm32, sizeof(instruction.imm32));
        instructionLen += sizeof(instruction.imm32);
    }
    
    assert(instructionLen <= maxInstructionLen);

    *outInstructionLen = instructionLen;
    return instructionBytes;
}

uint8_t* EncodeX64(X64Operation operation, X64Operand operand1, X64Operand operand2, 
                   size_t* outInstructionLen)
{
    return EncodeX64(operation, 2, operand1, operand2, outInstructionLen);
}

uint8_t* EncodeX64(X64Operation operation, X64Operand operand, size_t* outInstructionLen)
{
    X64Operand emptyOperand = {};
    return EncodeX64(operation, 1, operand, emptyOperand, outInstructionLen);
}
