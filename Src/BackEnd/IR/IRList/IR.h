#ifndef IR_LIST_H
#define IR_LIST_H

#include "BackEnd/IR/IRRegisters.h"

#define DEF_IR_OP(IR_OP, ...) IR_OP,
enum class IROperation
{
    #include "BackEnd/IR/IROperations.h"
};
#undef DEF_IR_OP

enum class IROperandType
{
    IMM, 
    MEM,
    REG,

    LABEL,
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

//-----------------------------------------------

IR* IRCtor();

void    IRPushBack(IR* irList, IRNode* node);
IRNode* IRHead    (const IR* irList);

//-----------------------------------------------

IRNode* IRNodeCreate(IROperation operation, const char* labelName, 
                     size_t numberOfOperands, IROperand operand1, IROperand operand2,
                     bool needPatch = false);

IRNode* IRNodeCreate(IROperation operation, IROperand operand1, bool needPatch = false);
IRNode* IRNodeCreate(IROperation operation, IROperand operand1, IROperand operand2, 
                     bool needPatch = false);
IRNode* IRNodeCreate(IROperation operation);
IRNode* IRNodeCreate(const char* labelName);

//-----------------------------------------------

IROperandValue IROperandValueCreate(long long imm = 0,  IRRegister reg = IRRegister::NO_REG, 
                                    const char* string = nullptr, IRErrors* error = nullptr);

void IROperandValueDtor (IROperandValue* value);

IROperand IROperandCtor();
IROperand IROperandCreate(IROperandValue val, IROperandType type);
IRNode*   IRNodeCtor();

IROperand IROperandRegCreate    (IRRegister reg);
IROperand IROperandImmCreate    (const long long imm);
IROperand IROperandStrCreate    (const char* str);
IROperand IROperandMemCreate    (const long long imm, IRRegister reg);
IROperand IROperandLabelCreate  (const char* label);

//-----------------------------------------------

#define IR_TEXT_DUMP(IR) IRTextDump(IR, __FILE__, __func__, __LINE__)
void IRTextDump(const IR* ir, const char* fileName, const char* funcName, const int line);

void IROperandTextDump(const IROperand operand);

//-----------------------------------------------

const char* IRGetOperationName(IROperation operation);

#endif