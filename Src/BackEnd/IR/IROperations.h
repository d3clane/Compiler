#ifndef DEF_IR_OP
#define DEF_IR_OP(...)
#endif

// DEF_IR_OP(OP_NAME, X64_GEN_CODE)
// PRINT_LABEL(LABEL) - prints label to outStream x64 asm

// PRINT_OPERATION(OP_NAME) - prints operation. Operands are taken from current node info

// PrintOperation(outStream, code, opNameInX64Asm, X64Operation, IROperand operand1, 
//                                                               IROperand operand2)

// Vars : IRNode* node, FILE* outStream, CodeArrayType* code

DEF_IR_OP(NOP,
{
    if (node->labelName)
    {
        PRINT_LABEL(node->labelName);
    }
    else
    {
        PRINT_OPERATION(NOP);
    }
})

DEF_IR_OP(PUSH,
{
    PRINT_OPERATION(PUSH);
})

DEF_IR_OP(POP,
{
    PRINT_OPERATION(POP);
})

DEF_IR_OP(MOV,
{
    PRINT_OPERATION(MOV);
})

DEF_IR_OP(ADD,
{
    PRINT_OPERATION(ADD);
})

DEF_IR_OP(SUB,
{
    PRINT_OPERATION(SUB);
})

DEF_IR_OP(F_ADD,
{
    PRINT_OPERATION(ADDSD);
})

DEF_IR_OP(F_SUB,
{
    PRINT_OPERATION(SUBSD);
})

DEF_IR_OP(F_MUL,
{
    PRINT_OPERATION(MULSD);
})

DEF_IR_OP(F_DIV,
{
    PRINT_OPERATION(DIVSD);
})

DEF_IR_OP(F_XOR,
{
    PRINT_OPERATION(PXOR);
})

DEF_IR_OP(F_AND,
{
    PRINT_OPERATION(ANDPD);
})

DEF_IR_OP(F_OR,
{
    PRINT_OPERATION(ORPD);
})

DEF_IR_OP(F_POW,
{
    assert(false); // TODO
})

DEF_IR_OP(F_SQRT,
{
    PrintOperation(outStream, code, "SQRTPD", X64Operation::SQRTPD, 
                   node->operand1, node->operand1);
})

DEF_IR_OP(F_SIN,
{
    assert(false); // TODO
})

DEF_IR_OP(F_COS,
{
    assert(false); // TODO
})

DEF_IR_OP(F_TAN,
{
    assert(false); // TODO
})

DEF_IR_OP(F_COT,
{
    assert(false); // TODO
})

DEF_IR_OP(F_PUSH,
{
    PrintAsmCodeLine(outStream, "\tSUB RSP, %d\n", (int)XMM_REG_BYTE_SIZE);
    PrintAsmCodeLine(outStream, "\tMOVSD [RSP], ");
    PrintOperand    (outStream, node->operand1);
    PrintAsmCodeLine(outStream, "\n");

    PrintOperationInCodeArray(code, X64Operation::SUB, 
                              X64OperandRegCreate(X64Register::RSP),
                              X64OperandImmCreate((int)XMM_REG_BYTE_SIZE));

    PrintOperationInCodeArray(code, X64Operation::MOVSD,
                              X64OperandMemCreate(X64Register::RSP, 0),
                              ConvertIRToX64Operand(node->operand1));
})

DEF_IR_OP(F_POP,
{
    PrintAsmCodeLine(outStream, "\tMOVSD ");
    PrintOperand    (outStream, node->operand1); 
    PrintAsmCodeLine(outStream, ", [RSP]\n");
    PrintAsmCodeLine(outStream, "\tADD RSP, %d\n", (int)XMM_REG_BYTE_SIZE);

    PrintOperationInCodeArray(code, X64Operation::MOVSD,
                              ConvertIRToX64Operand(node->operand1),
                              X64OperandMemCreate(X64Register::RSP, 0));

    PrintOperationInCodeArray(code, X64Operation::ADD, 
                              X64OperandRegCreate(X64Register::RSP),
                              X64OperandImmCreate((int)XMM_REG_BYTE_SIZE));
})

DEF_IR_OP(F_MOV,
{
    if (node->operand2.type == IROperandType::IMM)
    {
        PrintAsmCodeLine(outStream, "\tMOVSD ");
        PrintOperand    (outStream, node->operand1);

        long long imm = node->operand2.value.imm;

        RodataImmediatesValue* immLabelInfo = GetImmLabelInfo(imm, rodata.rodataImmediates);

        PrintAsmCodeLine(outStream, ", [%s]\n", immLabelInfo->label);

        PrintOperationInCodeArray(code, X64Operation::MOVSD, 
                                  ConvertIRToX64Operand(node->operand1),
                                  X64OperandMemCreate(X64Register::NO_REG, immLabelInfo->asmAddr));

    }
    else
        PRINT_OPERATION(MOVSD);
})

DEF_IR_OP(F_CMP,
{
    PRINT_OPERATION(COMISD);
})

DEF_IR_OP(JMP,
{
    SetLabelRelativeShift(node);
    PRINT_OPERATION(JMP);
})

DEF_IR_OP(JE,
{
    SetLabelRelativeShift(node);
    PRINT_OPERATION(JE);
})

DEF_IR_OP(JNE,
{
    SetLabelRelativeShift(node);
    PRINT_OPERATION(JNE);
})

DEF_IR_OP(JL,
{
    SetLabelRelativeShift(node);
    PRINT_OPERATION(JB);
})

DEF_IR_OP(JLE,
{
    SetLabelRelativeShift(node);
    PRINT_OPERATION(JBE);
})

DEF_IR_OP(JG,
{
    SetLabelRelativeShift(node);
    PRINT_OPERATION(JA);
})

DEF_IR_OP(JGE,
{
    SetLabelRelativeShift(node);
    PRINT_OPERATION(JAE);
})

DEF_IR_OP(CALL,
{
    SetLabelRelativeShift(node);
    PRINT_OPERATION(CALL);
})

DEF_IR_OP(RET,
{
    PRINT_OPERATION(RET);
})

DEF_IR_OP(F_OUT,
{
    PrintAsmCodeLine(outStream, "\tSUB RSP, %d\n", (int)XMM_REG_BYTE_SIZE);
    PrintAsmCodeLine(outStream, "\tMOVSD [RSP], ");
    PrintOperand    (outStream, node->operand1);
    PrintAsmCodeLine(outStream, "\n");
    PrintAsmCodeLine(outStream, "\tCALL StdFOut\n");
    
    PrintOperationInCodeArray(code, X64Operation::SUB, 
                              X64OperandRegCreate(X64Register::RSP),
                              X64OperandImmCreate((int)XMM_REG_BYTE_SIZE));

    PrintOperationInCodeArray(code, X64Operation::MOVSD,
                              X64OperandMemCreate(X64Register::RSP, 0),
                              ConvertIRToX64Operand(node->operand1));

    PrintOperationInCodeArray(code, X64Operation::CALL,
                              X64OperandImmCreate(
                              (int)StdLibAddresses::OUT_FLOAT - node->asmCmdEndAddress));
})

DEF_IR_OP(F_IN,
{
    PrintAsmCodeLine(outStream, "\tCALL StdIn\n");

    PrintOperationInCodeArray(code, X64Operation::CALL,
                              X64OperandImmCreate(
                              (int)StdLibAddresses::IN_FLOAT - node->asmCmdEndAddress));
})

DEF_IR_OP(STR_OUT,
{
    const char* string = node->operand1.value.string;

    RodataStringsValue* strLabelInfo  = GetStrLabelInfo(string, rodata.rodataStrings);

    PrintAsmCodeLine(outStream, "\tLEA RAX, [%s]\n", strLabelInfo->label);
    PrintAsmCodeLine(outStream, "\tPUSH RAX\n");
    PrintAsmCodeLine(outStream, "\tCALL StdStrOut\n");

    PrintOperationInCodeArray(code, X64Operation::LEA, 
                              X64OperandRegCreate(X64Register::RAX),
                              X64OperandMemCreate(X64Register::NO_REG, strLabelInfo->asmAddr));

    PrintOperationInCodeArray(code, X64Operation::PUSH,
                              X64OperandRegCreate(X64Register::RAX));

    PrintOperationInCodeArray(code, X64Operation::CALL,
                              X64OperandImmCreate(
                              (int)StdLibAddresses::OUT_STRING - node->asmCmdEndAddress));
})

DEF_IR_OP(HLT,
{
    PrintAsmCodeLine(outStream, "\tCALL StdHlt\n");

    PrintOperationInCodeArray(code, X64Operation::CALL,
                              X64OperandImmCreate(
                              (int)StdLibAddresses::HLT - node->asmCmdEndAddress));
})
