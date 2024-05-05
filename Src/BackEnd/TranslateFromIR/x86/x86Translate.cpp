#include <assert.h>
#include <stdarg.h>

#include "x86Translate.h"

#define PRINT_LABEL(LABEL) PrintLabel(outStream, #LABEL)
static inline void PrintLabel(FILE* outStream, const char* label);

#define EMPTY_OPERAND IROperandCtor()

#define PRINT_OPERATION(OPERATION) PrintOperation(outStream, #OPERATION, node)
static inline void PrintOperation(FILE* outStream, const char* operationName, const IRNode* node);

#define PRINT_OPERATION_TWO_OPERANDS(OPERATION, OPERAND1, OPERAND2)         \
    PrintOperation(outStream, #OPERATION, OPERAND1, OPERAND2)               
static inline void PrintOperation(FILE* outStream, const char* operationName, 
                                   const IROperand operand1, const IROperand operand2);

static inline void PrintOperation(FILE* outStream, const char* operationName, 
                                   const IROperand operand1);

#define PRINT_OPERAND(OPERAND) PrintOperand(outStream, OPERAND)
static inline void PrintOperand(FILE* outStream, const IROperand operand);

#define PRINT_STR(STRING) fprintf(outStream, "%s", STRING)

void TranslateToX86(const IR* ir, FILE* outStream)
{
    assert(ir);

    IRNode* beginNode = IRHead(ir);
    IRNode* node = beginNode;
    
#define DEF_IR_OP(OP_NAME, X86_ASM_CODE, ...)           \
    case IROperation::OP_NAME:                          \
    {                                                   \
        X86_ASM_CODE;                                   \
        break;                                          \
    }

    do
    {
        switch (node->operation)
        {
            #include "BackEnd/IR/IROperations.h" // cases on defines
        
            default:    // Unreachable
                assert(false);
                break;
        }

        node = node->nextNode;
    } while (node != beginNode);
    
}

static inline void PrintLabel(FILE* outStream, const char* label)
{
    assert(outStream);
    assert(label);

    fprintf(outStream, "%s:\n", label);
}

static inline void PrintOperation(FILE* outStream, const char* operationName,
                                  size_t numberOfOperands, 
                                  const IROperand operand1, const IROperand operand2)
{
    fprintf(outStream, "\t%s", operationName);
    if (numberOfOperands > 0)
        PrintOperand(outStream, operand1);
    
    if (numberOfOperands > 1)
    {
        fprintf(outStream, ", ");
        PrintOperand(outStream, operand2);
    }

    fprintf(outStream, "\n");
}   

static inline void PrintOperation(FILE* outStream, const char* operationName, const IRNode* node)
{
    PrintOperation(outStream, operationName, node->numberOfOperands, 
                   node->operand1, node->operand2);
}

static inline void PrintOperation(FILE* outStream, const char* operationName, 
                                   const IROperand operand1, const IROperand operand2)
{
    PrintOperation(outStream, operationName, 2, operand1, operand2);
}

static inline void PrintOperation(FILE* outStream, const char* operationName, 
                                   const IROperand operand1)
{
    PrintOperation(outStream, operationName, 1, operand1, EMPTY_OPERAND);
}

static inline void PrintOperand(FILE* outStream, const IROperand operand)
{
    switch (operand.type)
    {
        case IROperandType::IMM:
            fprintf(outStream, "%lld", operand.value.imm);
            break;
        
        case IROperandType::REG:
            fprintf(outStream, "%s", IRRegisterGetName(operand.value.reg));
            break;

        case IROperandType::MEM:
            assert(operand.value.reg != IRRegister::NO_REG);

            fprintf(outStream, "[%s + %lld]", 
                    IRRegisterGetName(operand.value.reg), operand.value.imm);

            break;

        case IROperandType::STR:
            assert(operand.value.string);
            fprintf(outStream, "%s", operand.value.string);
            break;

        default: // Unreachable
            assert(false);
            break;
    }
}
