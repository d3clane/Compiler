#ifndef IR_H
#define IR_H

#include <stddef.h>

#include "Tree/Tree.h"
#include "IRRegisters.h"

enum class IROperation
{
    NOP,

    PUSH, POP, MOV,

    ADD, SUB,

    F_ADD, F_SUB, F_MUL, F_DIV,

    F_AND, F_OR,

    F_SQRT, F_SIN, F_COS, F_TAN, F_COT,

    F_PUSH, F_POP, F_MOV,

    F_CMP,

    CMOVE, CMOVNE, CMOVG, CMOVGE, CMOVL, CMOVLE,

    JMP, JE, JNE, JG, JGE, JL, JLE,

    CALL, RET,

    F_OUT, F_IN,

    STR_OUT,

    HLT,

    #include "IROperations.h"
};

enum class IROperandType
{
    IMM, 
    MEM,
    REG,
    STR,    /// < string operand
};

struct IROperandValue
{
    long long   imm;
    IRRegister  reg;
    char*       string;
};

struct IROperand
{
    IROperandValue value;
    IROperandType  type;
};

static const size_t MaxCmdLenInBytes = 16;

struct IRNode
{
    IROperation operation;
    char*       labelName; // TODO: подумать статический массив или динамический

    size_t    numberOfOperands;
    IROperand operand1;
    IROperand operand2;

    IRNode* jumpTarget;

    bool needPatch;

    size_t asmAddress;
    size_t asmCmdLen;
    char   asmCmdBytes[MaxCmdLenInBytes];

    IRNode* nextNode;
    IRNode* prevNode;
};

struct IR
{
    IRNode* end;

    size_t size;
};

enum class IRErrors
{
    NO_ERR,

    MEM_ALLOC_ERR,
};

void IRPushBack(IR* ir, IRNode* node);

IRNode* IRNodeCreate(IROperation operation, const char* labelName, 
                     size_t numberOfOperands, IROperand operand1, IROperand operand2);

IR* BuildIR(const Tree* tree, const NameTableType* allNamesTable);

#endif