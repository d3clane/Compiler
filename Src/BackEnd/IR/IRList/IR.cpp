#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "Common/Log.h"
#include "IR.h"

#define TYPE(IR_TYPE)      IROperandType::IR_TYPE
#define IR_REG(REG_NAME)   IRRegister::REG_NAME  
#define OP(OP_NAME)        IROperation::OP_NAME
#define EMPTY_OPERAND      IROperandCtor()
#define CREATE_VALUE(...)  IROperandValueCreate(__VA_ARGS__)

void IRPushBack  (IR* ir, IRNode* node)
{
    assert(ir);
    assert(node);

    node->nextNode    = ir->end->nextNode;
    node->prevNode    = ir->end;
    ir->end->nextNode = node;
    
    ir->end = node;

    ir->size++;
}

IRNode* IRHead    (const IR* ir)
{
    assert(ir);
    assert(ir->end);

    return ir->end->nextNode;
}

IRNode* IRNodeCreate(IROperation operation, const char* labelName, 
                     size_t numberOfOperands, IROperand operand1, IROperand operand2,
                     bool needPatch)
{
    IRNode* node = IRNodeCtor();

    node->operation = operation;
    
    node->labelName = labelName ? strdup(labelName) : nullptr;

    node->numberOfOperands = numberOfOperands;
    
    node->operand1 = operand1;
    node->operand2 = operand2;

    node->needPatch = needPatch;

    return node;
}

IRNode* IRNodeCreate(IROperation operation, IROperand operand1, bool needPatch)
{
    return IRNodeCreate(operation, nullptr, 1, operand1, EMPTY_OPERAND, needPatch);
}

IRNode* IRNodeCreate(IROperation operation, IROperand operand1, IROperand operand2, bool needPatch)
{
    return IRNodeCreate(operation, nullptr, 2, operand1, operand2, needPatch);
}

IRNode* IRNodeCreate(IROperation operation)
{
    return IRNodeCreate(operation, nullptr, 0, EMPTY_OPERAND, EMPTY_OPERAND, false);
}

IRNode* IRNodeCreate(const char* labelName)
{
    return IRNodeCreate(OP(NOP), labelName, 0, EMPTY_OPERAND, EMPTY_OPERAND, false);
}

IROperand IROperandRegCreate(IRRegister reg)
{
    return IROperandCreate(CREATE_VALUE(0, reg), TYPE(REG));
}

IROperand IROperandImmCreate(const long long imm)
{
    return IROperandCreate(CREATE_VALUE(imm), TYPE(IMM));
}

IROperand IROperandStrCreate(const char* str)
{
    return IROperandCreate(CREATE_VALUE(0, IR_REG(NO_REG), str), TYPE(STR));
}

IROperand IROperandLabelCreate  (const char* label)
{
    return IROperandCreate(CREATE_VALUE(0, IR_REG(NO_REG), label), TYPE(LABEL));
}

IROperand IROperandMemCreate(const long long imm, IRRegister reg)
{
    return IROperandCreate(CREATE_VALUE(imm, reg), TYPE(MEM));
}

IROperandValue IROperandValueCreate(long long imm, IRRegister reg, const char* string,
                                    IRErrors* error)
{
    IROperandValue val = {};
    val.imm  = imm;
    val.reg  = reg;
    
    if (string)
    {
        val.string = strdup(string);

        if (error && !val.string) *error = IRErrors::MEM_ALLOC_ERR;
        else if (!error) assert(val.string);
    }

    return val;
}

IROperand IROperandCtor()
{
    IROperand operand = 
    {
        .value = IROperandValueCreate(),
        .type  = IROperandType::IMM,
    };

    return operand;
}

IROperand IROperandCreate(IROperandValue val, IROperandType type)
{
    IROperand operand = 
    {
        .value = val,
        .type  = type,
    };

    return operand;
}

IRNode* IRNodeCtor()
{
    IRNode* node = (IRNode*)calloc(1, sizeof(*node));

    node->operation = IROperation::NOP;

    node->asmCmdBeginAddress = 0;
    node->asmCmdEndAddress   = 0;
    
    node->jumpTarget = nullptr;
    node->needPatch  = false;

    node->numberOfOperands = 0;
    node->operand1         = IROperandCtor();
    node->operand2         = IROperandCtor();

    node->nextNode   = nullptr;
    node->prevNode   = nullptr;

    return node;
}

IR* IRCtor()
{
    IR* ir = (IR*)calloc(1, sizeof(*ir));

    ir->end = IRNodeCtor();
    ir->size  = 0;

    ir->end->nextNode = ir->end;
    ir->end->prevNode = ir->end;

    return ir;
}

void IROperandValueDtor(IROperandValue* value)
{
    if (!value)
        return;

    value->imm = 0;
    value->reg = IRRegister::NO_REG;

    if (!value->string)
        return;

    free(value->string);
    value->string = nullptr;
}

//-----------------------------------------------

void IRTextDump(const IR* ir, const char* fileName, const char* funcName, const int line)
{
    assert(ir);
    assert(fileName);
    assert(funcName);

    LogBegin(fileName, funcName, line);

    assert(ir->end);
    IRNode* beginNode = ir->end->nextNode;
    IRNode* node = beginNode;

    do
    {
        Log("---------------\n");
        Log("Operation - %s\n", IRGetOperationName(node->operation));    

        if (node->labelName) Log("Label name - %s\n", node->labelName);
        
        if (node->numberOfOperands > 0) IROperandTextDump(node->operand1);
        if (node->numberOfOperands > 1) IROperandTextDump(node->operand2);

        node = node->nextNode;
    } while (node != beginNode);
    
    LogEnd(fileName, funcName, line);
}

void IROperandTextDump(const IROperand operand)
{
    switch (operand.type)
    {
        case TYPE(IMM):
            Log("IMM: \n");
            break;
        case TYPE(REG):
            Log("REG: \n");
            break;
        case TYPE(MEM):
            Log("MEM: \n");
            break;
        case TYPE(STR):
            Log("STR: \n");
            break;
        case TYPE(LABEL):
            Log("LABEL: \n");
            break;

        default: // Unreachable
            assert(false);
            break;
    }

    Log("\t imm - %d\n"
        "\t reg - %s\n",
        operand.value.imm, IRRegisterGetName(operand.value.reg));

    if (operand.value.string) Log("\t str - %s\n", operand.value.string);
    else                      Log("\t str - null\n");
}

const char* IRGetOperationName(IROperation operation)
{
    #define DEF_IR_OP(OP_NAME, ...)     \
        case IROperation::OP_NAME:      \
            return #OP_NAME;            

    switch (operation)
    {
        #include "BackEnd/IR/IROperations.h"

        default:    // Unreachable
            assert(false);
            return nullptr;
    }

    // Unreachable
    assert(false);
    return nullptr;
}

#undef EMPTY_OPERAND
#undef CREATE_VALUE
#undef TYPE
#undef IR_REG
#undef OP
