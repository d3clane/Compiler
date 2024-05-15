#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "x64Translate.h"
#include "RodataInfo/RodataImmediates/RodataImmediates.h"
#include "RodataInfo/RodataStrings/RodataStrings.h"

//-----------------------------------------------------------------------------

#define PRINT_LABEL(LABEL) PrintLabel(outStream, LABEL)
static inline void PrintLabel(FILE* outStream, const char* label);

#define EMPTY_OPERAND IROperandCtor()

#define PRINT_OPERATION(OPERATION) PrintOperation(outStream, #OPERATION, node)
static inline void PrintOperation(FILE* outStream,
                                  const char* operationName, const IRNode* node);

#define PRINT_OPERATION_TWO_OPERANDS(OPERATION, OPERAND1, OPERAND2)         \
    PrintOperation(outStream, #OPERATION, OPERAND1, OPERAND2)               
static inline void PrintOperation(FILE* outStream, const char* operationName, 
                                   const IROperand operand1, const IROperand operand2);

static inline void PrintOperation(FILE* outStream, const char* operationName, 
                                   const IROperand operand1);

#define PRINT_OPERAND(OPERAND) PrintOperand(outStream, OPERAND)
static inline void PrintOperand(FILE* outStream, const IROperand operand);

#define PRINT_STR_WITH_SHIFT(STRING) fprintf(outStream, "\t%s", STRING)
#define PRINT_STR(STRING)            fprintf(outStream, "%s", STRING)
#define PRINT_FORMAT_STR(...)        fprintf(outStream, __VA_ARGS__)

//-----------------------------------------------------------------------------

struct RodataInfo
{
    RodataImmediatesType* rodataImmediates;
    RodataStringsType*    rodataStrings;
};

static inline RodataInfo RodataInfoCtor();
static inline void       RodataInfoDtor(RodataInfo* info);

static inline void PrintRodata          (FILE* outStream, const RodataInfo* info);
static inline void PrintRodataImmediates(FILE* outStream, RodataImmediatesType* rodataImmediates);
static inline void PrintRodataStrings   (FILE* outStream, RodataStringsType*    rodataStrings);

#define GET_STR_LABEL(STRING) GetStringLabel(STRING, info.rodataStrings);
static inline const char* GetStringLabel      (const char* string, RodataStringsType* rodataString);
static inline       char* CreateStringLabel   (const char* string);
#define GET_IMM_LABEL(IMM) GetImmediateLabel(IMM, info.rodataImmediates);
static inline const char* GetImmediateLabel   (const long long imm, RodataImmediatesType* rodataImm);
static inline       char* CreateImmediateLabel(const long long imm);

//-----------------------------------------------------------------------------

static inline void PrintEntry   (FILE* outStream);

//-----------------------------------------------------------------------------

void TranslateToX64(const IR* ir, FILE* outStream, FILE* outBin)
{
    assert(ir);

    PrintEntry(outStream);
    
    RodataInfo info = RodataInfoCtor();

    IRNode* beginNode = IRHead(ir);
    IRNode* node = beginNode->nextNode;

#define DEF_IR_OP(OP_NAME, X86_ASM_CODE, ...)           \
    case IROperation::OP_NAME:                          \
    {                                                   \
        X86_ASM_CODE;                                   \
        break;                                          \
    }

    static const size_t maxBytecodeLen = 10000;
    uint8_t* bytecode  = (uint8_t*)calloc(maxBytecodeLen, sizeof(*bytecode));
    size_t bytecodePos = 0;

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
    
    PrintRodata(outStream, &info);

    RodataInfoDtor(&info);
}

//-----------------------------------------------------------------------------

static inline void PrintLabel(FILE* outStream, const char* label)
{
    assert(outStream);
    assert(label);

    fprintf(outStream, "%s:\n", label);
}

static inline void PrintOperation(FILE* outStream,
                                  const char* operationName, size_t numberOfOperands, 
                                  const IROperand operand1, const IROperand operand2)
{
    fprintf(outStream, "\t%s ", operationName);
    if (numberOfOperands > 0)
        PrintOperand(outStream, operand1);
    
    if (numberOfOperands > 1)
    {
        fprintf(outStream, ", ");
        PrintOperand(outStream, operand2);
    }

    fprintf(outStream, "\n");
}   

static inline void PrintOperation(FILE* outStream,
                                  const char* operationName, const IRNode* node)
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
        
        case IROperandType::LABEL:
            assert(operand.value.string);
            fprintf(outStream, "%s", operand.value.string);
            break;

        case IROperandType::STR: // Unreachable
            assert(false);
            break;

        default: // Unreachable
            assert(false);
            break;
    }
}

//-----------------------------------------------------------------------------

static inline void PrintEntry(FILE* outStream)
{
    assert(outStream);

    static const char* stdLibName = "StdLib57.s";

    fprintf(outStream, "%%include '%s'\n\n"
                       "section .text\n"
                       "global _start\n\n",
                       stdLibName);

    fprintf(outStream, "_start:\n"
                       "\tcall main\n"
                       "\tcall StdHlt\n\n");

}

//-----------------------------------------------------------------------------

static inline RodataInfo RodataInfoCtor()
{
    RodataInfo info = {};
    RodataImmediatesCtor(&info.rodataImmediates);
    RodataStringsCtor   (&info.rodataStrings);

    return info;
}

static inline void       RodataInfoDtor(RodataInfo* info)
{
    assert(info);

    RodataImmediatesDtor(info->rodataImmediates);
    RodataStringsDtor   (info->rodataStrings);

    info->rodataImmediates = nullptr;
    info->rodataStrings    = nullptr;
}

static inline void PrintRodata(FILE* outStream, const RodataInfo* info)
{
    assert(outStream);
    assert(info);

    fprintf(outStream, "section .rodata\n\n");

    PrintRodataImmediates(outStream, info->rodataImmediates);
    PrintRodataStrings   (outStream, info->rodataStrings);
}

static inline void PrintRodataImmediates(FILE* outStream, RodataImmediatesType* rodataImmediates)
{
    assert(outStream);
    assert(rodataImmediates);

    // TODO: Implementation defined - change on memcpy
    union 
    {
        double value;
        int    bytes[2];
    } doubleBytes;

    for (size_t i = 0; i < rodataImmediates->size; ++i)
    {
        long long imm = rodataImmediates->data[i].imm;
        doubleBytes.value = (double)imm;

        const char* immLabel = GetImmediateLabel(imm, rodataImmediates);

        fprintf(outStream, "%s:\n"
                           "\tdd %d\n"
                           "\tdd %d\n\n",
                           immLabel, doubleBytes.bytes[0], doubleBytes.bytes[1]);
    }
}

static inline void PrintRodataStrings   (FILE* outStream, RodataStringsType* rodataStrings)
{
    assert(outStream);
    assert(rodataStrings);

    for (size_t i = 0; i < rodataStrings->size; ++i)
    {
        const char* string = rodataStrings->data[i].string;
        const char* stringLabel  = GetStringLabel(string, rodataStrings);

        fprintf(outStream, "%s:\n"
                           "\tdb \'%s\', 0\n\n", 
                           stringLabel, string);
    }
}

static inline char* CreateStringLabel(const char* string)
{
    assert(string);

    static size_t labelId = 0;

    static const size_t maxLabelLen = 32;
    char label[maxLabelLen] = "";

    snprintf(label, maxLabelLen, "STR_%zu", labelId);
    ++labelId;

    return strdup(label);   
}

static inline const char* GetStringLabel(const char* string, 
                                         RodataStringsType* rodataStrings)
{
    assert(string);

    RodataStringsValue* value = nullptr;
    RodataStringsFind(rodataStrings, string, &value);

    if (value == nullptr)
    {
        char* label = CreateStringLabel(string);

        RodataStringsValue pushValue = {};
        RodataStringsValueCtor(&pushValue, string, label);
        RodataStringsPush(rodataStrings, pushValue);

        RodataStringsFind(rodataStrings, string, &value);

        free(label);
    }

    assert(value);
    assert(value->label);

    return value->label;
}

static inline const char* GetImmediateLabel(const long long imm, 
                                            RodataImmediatesType* rodataImmediates)
{
    RodataImmediatesValue* value = nullptr;
    RodataImmediatesFind(rodataImmediates, imm, &value);

    if (value == nullptr)
    {
        char* label = CreateImmediateLabel(imm);

        RodataImmediatesValue pushValue = {};
        RodataImmediatesValueCtor(&pushValue, imm, label);
        RodataImmediatesPush(rodataImmediates, pushValue);

        RodataImmediatesFind(rodataImmediates, imm, &value);

        free(label);
    }

    assert(value);
    assert(value->label);
    
    return value->label;
}

static inline char* CreateImmediateLabel(const long long imm)
{
    static const size_t maxLabelLen = 32;
    char label[maxLabelLen] = "";

    if (imm < 0)
        snprintf(label, maxLabelLen, "XMM_VALUE__%lld", -imm);
    else
        snprintf(label, maxLabelLen, "XMM_VALUE_%lld", imm);

    return strdup(label);
}


#undef PRINT_LABEL
#undef EMPTY_OPERAND
#undef PRINT_OPERATION
#undef PRINT_OPERATION_TWO_OPERANDS
#undef PRINT_OPERAND
#undef PRINT_STR_WITH_SHIFT
#undef PRINT_STR
#undef PRINT_FORMAT_STR
#undef GET_STR_LABEL
#undef GET_IMM_LABEL