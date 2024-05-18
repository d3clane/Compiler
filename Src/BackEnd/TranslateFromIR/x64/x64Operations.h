#ifndef DEF_X64_OP
#define DEF_X64_OP(...)
#endif

// DEF_X64_OP(OP_NAME, INSTRUCTION_INIT_CODE)

// Available Vars    : IRNode* node, X64Instruction instruction
// Available defines : BYTE_TARGET(), EMPTY_BYTE_TARGET from x64Encode.cpp

DEF_X64_OP(NOP,
{
    instruction.opcode = 0x90;
    X64_INSTRUCTION_INIT(EMPTY_BYTE_TARGET, EMPTY_BYTE_TARGET);
})

DEF_X64_OP(PUSH,
{
    assert(numberOfOperands == 1 && operand1.type == X64OperandType::REG);
    
    instruction.opcode = 0x50;

    X64_INSTRUCTION_INIT(BYTE_TARGET(OPCODE), EMPTY_BYTE_TARGET);
})

DEF_X64_OP(POP,
{
    assert(numberOfOperands == 1 && operand1.type == X64OperandType::REG);

    instruction.opcode = 0x58;

    X64_INSTRUCTION_INIT(BYTE_TARGET(OPCODE), EMPTY_BYTE_TARGET);
})

DEF_X64_OP(MOV,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);

    instruction.opcode = 0x8B;
    SetRexW(&instruction);

    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})

DEF_X64_OP(ADD,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG &&
                                    operand2.type == X64OperandType::IMM);

    instruction.opcode = 0x81;
    SetRexW(&instruction);
    SetModRmRegField(&instruction, 0);
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_RM), BYTE_TARGET(IMM32));
})

DEF_X64_OP(SUB,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG &&
                                    operand2.type == X64OperandType::IMM);

    instruction.opcode = 0x81;
    SetRexW(&instruction);
    SetModRmRegField(&instruction, 5);
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_RM), BYTE_TARGET(IMM32));
})

//TODO: Think about copypaste ADDSD, SUBSD, ...
DEF_X64_OP(ADDSD,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);
                                    
    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_F2;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    instruction.opcode = 0x58;

    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})

DEF_X64_OP(SUBSD,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);
                                    
    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_F2;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    instruction.opcode = 0x5C;
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})

DEF_X64_OP(MULSD,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);
                                    
    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_F2;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    instruction.opcode = 0x59;
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})

DEF_X64_OP(DIVSD,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);
                                    
    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_F2;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    instruction.opcode = 0x5E;
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})

DEF_X64_OP(PXOR,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);
                                    
    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_66;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    instruction.opcode = 0xEF;
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})

DEF_X64_OP(ANDPD,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);
                                    
    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_66;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    instruction.opcode = 0x54;
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})

DEF_X64_OP(ORPD,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);
                                    
    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_66;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    instruction.opcode = 0x56;
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})

DEF_X64_OP(SQRTPD,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);
                                    
    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_66;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    instruction.opcode = 0x51;
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
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
    assert(numberOfOperands == 2);
    assert((operand1.type == X64OperandType::MEM && operand2.type == X64OperandType::REG) ||
           (operand1.type == X64OperandType::REG && operand2.type == X64OperandType::MEM));

    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_F2;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    if (operand1.type == X64OperandType::MEM && operand2.type == X64OperandType::REG)
    {
        instruction.opcode = 0x11;

        X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_RM), BYTE_TARGET(MODRM_REG));    
    }
    else
    {
        instruction.opcode = 0x10;

        X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
    }
})

DEF_X64_OP(COMISD,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::REG);
                                    
    instruction.requireMandatoryPrefix = true;
    instruction.mandatoryPrefix        = MandatoryPrefix_66;

    instruction.requireOpcodePrefix1   = true; 
    instruction.opcodePrefix1          = OpcodePrefix1_0F;

    instruction.opcode = 0x2F;
    
    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})

DEF_X64_OP(JMP,
{
    assert(numberOfOperands == 1 && operand1.type == X64OperandType::IMM);

    instruction.opcode = 0xE9;

    X64_INSTRUCTION_INIT(BYTE_TARGET(IMM32), EMPTY_BYTE_TARGET);
})

#define GEN_JCC(OPCODE)                                             \
do                                                                  \
{                                                                   \
    instruction.requireOpcodePrefix1 = true;                        \
    instruction.opcodePrefix1 = OpcodePrefix1_0F;                   \
    instruction.opcode = OPCODE;                                    \
    X64_INSTRUCTION_INIT(BYTE_TARGET(IMM32), EMPTY_BYTE_TARGET);    \
} while (0)

DEF_X64_OP(JE,
{
    GEN_JCC(0x84);
})

DEF_X64_OP(JNE,
{
    GEN_JCC(0x85);
})

DEF_X64_OP(JB,
{
    GEN_JCC(0x82);
})

DEF_X64_OP(JBE,
{
    GEN_JCC(0x86);
})

DEF_X64_OP(JA,
{
    GEN_JCC(0x87);
})

DEF_X64_OP(JAE,
{
    GEN_JCC(0x83);
})

#undef GEN_JCC

DEF_X64_OP(CALL,
{
    assert(numberOfOperands == 1 && operand1.type == X64OperandType::IMM);

    instruction.opcode = 0xE8;

    X64_INSTRUCTION_INIT(BYTE_TARGET(IMM32), EMPTY_BYTE_TARGET);
})

DEF_X64_OP(RET,
{
    assert(numberOfOperands == 1 && operand1.type == X64OperandType::IMM);

    instruction.opcode = 0xC2;

    X64_INSTRUCTION_INIT(BYTE_TARGET(IMM16), EMPTY_BYTE_TARGET);
})

DEF_X64_OP(LEA,
{
    assert(numberOfOperands == 2 && operand1.type == X64OperandType::REG && 
                                    operand2.type == X64OperandType::MEM);

    instruction.opcode = 0x8D;
    SetRexW(&instruction);

    X64_INSTRUCTION_INIT(BYTE_TARGET(MODRM_REG), BYTE_TARGET(MODRM_RM));
})
