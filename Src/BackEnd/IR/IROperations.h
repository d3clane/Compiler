#ifndef DEF_IR_OP
#define DEF_IR_OP(...)
#endif

// DEF_IR_OP(OP_NAME, ...)

// PRINT_LABEL(LABEL) - prints label to outStream x86 asm
// Vars : IRNode* node

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
    PRINT_OPERATION_TWO_OPERANDS(SQRTPD, node->operand1, node->operand1);
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
    PRINT_STR_WITH_SHIFT("SUB RSP, 0x10\n");
    PRINT_STR_WITH_SHIFT("MOVSD [RSP], ");
    PRINT_OPERAND(node->operand1);
    PRINT_STR("\n");
})

DEF_IR_OP(F_POP,
{
    PRINT_STR_WITH_SHIFT("MOVSD ");
    PRINT_OPERAND(node->operand1); PRINT_STR(", [RSP]\n");
    PRINT_STR_WITH_SHIFT("ADD RSP, 0x10\n");
})

DEF_IR_OP(F_MOV,
{
    if (node->operand2.type == IROperandType::IMM)
    {
        PRINT_STR_WITH_SHIFT("MOVSD ");
        PRINT_OPERAND(node->operand1);

        long long imm = node->operand2.value.imm;
        const char* immLabel = GET_IMM_LABEL(imm);

        PRINT_FORMAT_STR(", [rel %s]\n", immLabel);
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
    PRINT_OPERATION(JMP);
})

DEF_IR_OP(JE,
{
    PRINT_OPERATION(JE);
})

DEF_IR_OP(JNE,
{
    PRINT_OPERATION(JNE);
})

DEF_IR_OP(JL,
{
    PRINT_OPERATION(JB);
})

DEF_IR_OP(JLE,
{
    PRINT_OPERATION(JBE);
})

DEF_IR_OP(JG,
{
    PRINT_OPERATION(JA);
})

DEF_IR_OP(JGE,
{
    PRINT_OPERATION(JAE);
})

DEF_IR_OP(CALL,
{
    PRINT_OPERATION(CALL);
})

DEF_IR_OP(RET,
{
    PRINT_OPERATION(RET);
})

DEF_IR_OP(F_OUT,
{
    PRINT_STR_WITH_SHIFT("SUB RSP, 0x10\n");
    PRINT_STR_WITH_SHIFT("MOVSD [RSP], ");
    PRINT_OPERAND(node->operand1);
    PRINT_STR("\n");
    PRINT_STR_WITH_SHIFT("CALL StdFOut\n");
})

DEF_IR_OP(F_IN,
{
    PRINT_STR_WITH_SHIFT("CALL StdIn\n");
})

DEF_IR_OP(STR_OUT,
{
    const char* string = node->operand1.value.string;
    const char* stringLabel  = GET_STR_LABEL(string);

    PRINT_FORMAT_STR    ("\tLEA RAX, [rel %s]\n", stringLabel);
    PRINT_STR_WITH_SHIFT("PUSH RAX\n");
    PRINT_STR_WITH_SHIFT("CALL StdStrOut\n");
})

DEF_IR_OP(HLT,
{
    PRINT_STR_WITH_SHIFT("CALL StdHlt\n");
})
