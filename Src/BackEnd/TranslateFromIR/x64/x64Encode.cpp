#include <assert.h>
#include <stddef.h>

#include "x64Encode.h"

#define EMPTY 0

static const uint8_t MandatoryPrefix_66 = 0x66;
static const uint8_t MandatoryPrefix_F2 = 0xF2;
static const uint8_t MandatoryPrefix_F3 = 0xF3;

static const uint8_t OpcodePrefix1_0F   = 0x0F;
static const uint8_t OpcodePrefix2_38   = 0xF2;
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

static inline void SetRexB(X64Instruction* instruction, uint8_t bit);
static inline void SetRexW(X64Instruction* instruction, uint8_t bit);
static inline void SetRexR(X64Instruction* instruction, uint8_t bit);

#define BYTE_TARGET(TARGET) X64OperandByteTarget::TARGET


#define X64_INSTRUCTION_CREATE(OPCODE, TARGET1, TARGET2,                                    \
                               REQ_MANDATORY_PREFIX, REQ_REX_BY_DEFAULT, REQ_OPCODE_PREF1,  \
                               REQ_OPCODE_PREF2, REQ_DISP32, REQ_IMM32, REQ_REG_IN_OPCODE,  \
                               MANDATORY_PREFIX, OPCODE_PREF1, OPCODE_PREF2)                \
    X64InstructionCreate(OPCODE, numberOfOperands)
X64Instruction X64InstructionCreate
(uint8_t opcode, size_t numberOfOperands, 
 X64Operand operand1, X64Operand operand2,
 X64OperandByteTarget operand1Target, X64OperandByteTarget operand2Target,
 bool requireMandatoryPrefix, bool requireREXByDefault, 
 bool requireOpcodePrefix1, bool requireOpcodePrefix2,
 bool requireDisp32, bool requireImm32, bool requireImm16, bool requireRegInOpcode,
 uint8_t mandatoryPrefix, uint8_t opcodePrefix1, uint8_t opcodePrefix2)
{
    X64Instruction instruction = {};

#define SET(INFO) instruction.##INFO = ##INFO;
    SET(requireMandatoryPrefix);
    SET(requireOpcodePrefix1);
    SET(requireOpcodePrefix2);
    SET(requireDisp32);
    SET(requireImm32);
    SET(requireImm16);
    SET(mandatoryPrefix);
    SET(opcodePrefix1);
    SET(opcodePrefix2);
    SET(opcode);
#undef SET

    instruction.requireREX = requireREXByDefault;

    if (operand1Target == BYTE_TARGET(MODRM_REG) || operand1Target == BYTE_TARGET(MODRM_RM) ||
        operand2Target == BYTE_TARGET(MODRM_REG) || operand2Target == BYTE_TARGET(MODRM_RM))
        instruction.requireModRM = true;
    
    if (operand1Target == BYTE_TARGET(SIB) || operand2Target == BYTE_TARGET(SIB))
    {
        instruction.requireModRM = true;
        instruction.requireSIB   = true;
    }

    SetRexRequirement(&instruction, numberOfOperands, operand1, operand2);

    static const size_t standardRex = 0x40;
    if (instruction.requireREX)
        instruction.rex = standardRex;

    SetOperands(&instruction, numberOfOperands, operand1, operand2, 
                operand1Target, operand2Target);

    return instruction;
} 

X64Operand ConvertIRToX64Operand(IROperand irOperand)
{
    X64Operand x64Operand = {};

#define CASE(IR_TYPE)                               \
    case IROperandType::IR_TYPE:                    \
        x64Operand.type = X64OperandType::IR_TYPE;  \
        break;

    switch (irOperand.type)
    {
        CASE(IMM);
        CASE(REG);
        CASE(MEM);

        case IROperandType::STR:
            // TODO: проблема - у меня строчка фактически в двух кейсах - 
            // лейбел для прыжка, строчка для вывода. Ваще надо разделить эти кейсы потому что лейбл 
            // - это фактически imm32, а строчка это RIP адрессация
        case IROperandType::IMM:
            x64Operand.type = X64OperandType::IMM;
            break;
        
        default:
            break;
    }
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

    if (operand.value.reg == X64Register::NONE)
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

    if (instruction->requireRegInOpcode)
    {
        assert(numberOfOperands == 1);
        assert(operand1.type == X64OperandType::REG);
        assert(!instruction->requireModRM);
        assert(!instruction->requireSIB);

        SetRegInOpcode(instruction, operand1);

        return;
    }

    if (numberOfOperands > 0)
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
    static const size_t regFieldShift = 3;
#define DEF_X64_REG(REG, LOW_BITS, HIGH_BIT, ...)               \
    case X64Register::REG:                                      \
        instruction->modRM |= (LOW_BITS << regFieldShift);      \
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
