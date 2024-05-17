#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include "x64Translate.h"
#include "x64Encode.h"
#include "x64Elf.h"
#include "RodataInfo/Rodata.h"
#include "CodeArray/CodeArray.h"

//-----------------------------------------------------------------------------

#define PRINT_LABEL(LABEL) PrintLabel(outStream, LABEL)
static inline void PrintLabel(FILE* outStream, const char* label);

#define EMPTY_OPERAND IROperandCtor()

#define PRINT_OPERATION(OPERATION) PrintOperation(outStream, code, #OPERATION,     \
                                                  X64Operation::OPERATION, node)

static inline void PrintOperation(FILE* outStream, CodeArrayType* code,
                                  const char* operationName, X64Operation x64Operation, 
                                  const IRNode* node);

static inline void PrintOperationInCodeArray(CodeArrayType* code, X64Operation x64Operation,
                                             size_t numberOfOperands,
                                             X64Operand operand1, X64Operand operand2);

static inline void PrintOperationInCodeArray(CodeArrayType* code, X64Operation x64Operation,
                                             X64Operand operand1, X64Operand operand2);

static inline void PrintOperationInCodeArray(CodeArrayType* code, X64Operation x64Operation,
                                             X64Operand operand1);

#define PRINT_OPERATION_TWO_OPERANDS(OPERATION, OPERAND1, OPERAND2)         \
    PrintOperation(outStream, code, #OPERATION, X64Operation::OPERATION, OPERAND1, OPERAND2)
static inline void PrintOperation(FILE* outStream, CodeArrayType* code, 
                                  const char* operationName, X64Operation x64Operation,
                                  const IROperand operand1, const IROperand operand2);

static inline void PrintOperation(FILE* outStream, CodeArrayType* code, 
                                  const char* operationName, X64Operation x64Operation,
                                  const IROperand operand1);

#define PRINT_OPERAND(OPERAND) PrintOperand(outStream, OPERAND)
static inline void PrintOperand(FILE* outStream, const IROperand operand);

#define PRINT_FORMAT_STR(...) if (outStream) fprintf(outStream, __VA_ARGS__)

//-----------------------------------------------------------------------------

static inline void PrintRodata          (FILE* outStream, const RodataInfo* rodata);
static inline void PrintRodataImmediates(FILE* outStream, RodataImmediatesType* rodataImmediates);
static inline void PrintRodataStrings   (FILE* outStream, RodataStringsType*    rodataStrings);

#define GET_STR_ADDR_IN_RODATA(STRING) GetStrAddrInRodata(STRING, rodata.rodataStrings)
static inline RodataStringsValue*      GetStrAddrInRodata(const char* string, 
                                                          RodataStringsType* rodataString);

static inline char* CreateStringLabel   (const char* string);

#define GET_IMM_ADDR_IN_RODATA(IMM)  GetImmAddrInRodata(IMM, rodata.rodataImmediates);
static inline RodataImmediatesValue* GetImmAddrInRodata(const long long imm, 
                                                        RodataImmediatesType* rodataImm);

static inline char* CreateImmediateLabel(const long long imm);

//-----------------------------------------------------------------------------

static inline void SetLabelRelativeShift(IRNode* node);

//-----------------------------------------------------------------------------

static inline void PrintEntry(FILE* outStream);

//-----------------------------------------------------------------------------

static inline void SetLabelRelativeShift(IRNode* node)
{
    assert(node->numberOfOperands == 1);
    assert(node->operand1.type = IROperandType::LABEL);
    assert(node->jumpTarget);

    node->operand1.value.imm = node->jumpTarget->asmCmdBeginAddress - node->asmCmdEndAddress;
}

void TranslateToX64(const IR* ir, FILE* outStream, FILE* outBin)
{
    assert(ir);
    
    RodataInfo rodata = RodataInfoCtor();

    static const size_t numberOfCompilationPasses = 2;

    CodeArrayType* code = nullptr;
    CodeArrayCtor(&code, 0);

    PrintEntry(outStream);

    for (size_t compilationPass = 0; compilationPass < numberOfCompilationPasses; ++compilationPass)
    {
        IRNode* beginNode = IRHead(ir);
        IRNode* node = beginNode->nextNode;

        CodeArrayDtor(code);    // each pass writing code again but having more information
        CodeArrayCtor(&code, 0);

        while (node != beginNode)
        {
            node->asmCmdBeginAddress = (int)SegmentAddress::PROGRAM_CODE + code->size;
        #define DEF_IR_OP(OP_NAME, X64_GEN, ...)            \
            case IROperation::OP_NAME:                      \
                X64_GEN;                                    \
                break;

            switch (node->operation)
            {
                #include "BackEnd/IR/IROperations.h" // cases
            
                default:    // Unreachable
                    assert(false);
                    break;
            }

            node->asmCmdEndAddress = (int)SegmentAddress::PROGRAM_CODE + code->size;
            node = node->nextNode;
        }

        PrintRodata(outStream, &rodata);
        LoadRodata(&rodata, outBin);

        outStream = nullptr;
        // TODO: мне больше не надо писать в файл после первого прохода 
        // но по факту это очень грубый костыль
        node = node->nextNode;
    }

    LoadCode(code, outBin);
    RodataInfoDtor(&rodata);
    CodeArrayDtor(code);
}

//-----------------------------------------------------------------------------

static inline void PrintLabel(FILE* outStream, const char* label)
{
    assert(label);

    if (outStream) fprintf(outStream, "%s:\n", label);
}

static inline void PrintOperation(FILE* outStream, CodeArrayType* code,
                                  const char* operationName, X64Operation x64Operation,
                                  size_t numberOfOperands, 
                                  const IROperand operand1, const IROperand operand2)
{
    assert(code);

    // Print to outStream
    if (outStream)
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

    // Print to code

    PrintOperationInCodeArray(code, x64Operation, numberOfOperands, 
                              ConvertIRToX64Operand(operand1), ConvertIRToX64Operand(operand2));
}

static inline void PrintOperationInCodeArray(CodeArrayType* code, X64Operation x64Operation,
                                             size_t numberOfOperands,
                                             X64Operand operand1, X64Operand operand2)
{
    assert(code);

    size_t instructionLen = 0;
    uint8_t* instructionCode = EncodeX64(x64Operation, numberOfOperands, 
                                         operand1, operand2, &instructionLen);

    for (size_t i = 0; i < instructionLen; ++i)
        CodeArrayPush(code, instructionCode[i]);

    free(instructionCode);
}

static inline void PrintOperationInCodeArray(CodeArrayType* code, X64Operation x64Operation,
                                             X64Operand operand1, X64Operand operand2)
{
    PrintOperationInCodeArray(code, x64Operation, 2, operand1, operand2);
}

static inline void PrintOperationInCodeArray(CodeArrayType* code, X64Operation x64Operation,
                                             X64Operand operand1)
{
    X64Operand emptyOperand = {};
    PrintOperationInCodeArray(code, x64Operation, 1, operand1, emptyOperand);
}

static inline void PrintOperation(FILE* outStream, CodeArrayType* code,
                                  const char* operationName, X64Operation x64Operation,
                                  const IRNode* node)
{
    PrintOperation(outStream, code, operationName, x64Operation, node->numberOfOperands, 
                   node->operand1, node->operand2);
}

static inline void PrintOperation(FILE* outStream, CodeArrayType* code, 
                                  const char* operationName, X64Operation x64Operation,
                                  const IROperand operand1, const IROperand operand2)
{
    PrintOperation(outStream, code, operationName, x64Operation, 2, operand1, operand2);
}

static inline void PrintOperation(FILE* outStream, CodeArrayType* code, 
                                  const char* operationName, X64Operation x64Operation,
                                  const IROperand operand1)
{
    PrintOperation(outStream, code, operationName, x64Operation, 1, operand1, EMPTY_OPERAND);
}

static inline void PrintOperand(FILE* outStream, const IROperand operand)
{
    if (!outStream)
        return;

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
    static const char* stdLibSource = "StdLib57.s";

    if (outStream)
    {
        fprintf(outStream, "%%include '%s'\n\n"
                           "section .text\n"
                           "global _start\n\n",
                           stdLibSource);
    }
}

//-----------------------------------------------------------------------------

static inline void PrintRodata(FILE* outStream, const RodataInfo* rodata)
{
    assert(rodata);

    if (!outStream)
        return;

    fprintf(outStream, "section .rodata\n\n");

    PrintRodataImmediates(outStream, rodata->rodataImmediates);
    PrintRodataStrings   (outStream, rodata->rodataStrings);
}

static inline void PrintRodataImmediates(FILE* outStream, RodataImmediatesType* rodataImmediates)
{
    assert(outStream);
    assert(rodataImmediates);

    // TODO: Implementation defined - change on memcpy
    union 
    {
        double value;
        int    dwords[2];
    } doubleBytes;

    for (size_t i = 0; i < rodataImmediates->size; ++i)
    {
        long long imm = rodataImmediates->data[i].imm;
        doubleBytes.value = (double)imm;

        const char* immLabel = GetImmAddrInRodata(imm, rodataImmediates)->label;

        fprintf(outStream, "%s:\n"
                           "\tdd %d\n"
                           "\tdd %d\n\n",
                           immLabel, doubleBytes.dwords[0], doubleBytes.dwords[1]);
    }
}

static inline void PrintRodataStrings   (FILE* outStream, RodataStringsType* rodataStrings)
{
    assert(outStream);
    assert(rodataStrings);

    for (size_t i = 0; i < rodataStrings->size; ++i)
    {
        const char* string = rodataStrings->data[i].string;
        const char* stringLabel = GetStrAddrInRodata(string, rodataStrings)->label;

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

static inline RodataStringsValue* GetStrAddrInRodata(const char* string, 
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

    return value;
}

static inline RodataImmediatesValue* GetImmAddrInRodata(const long long imm, 
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
    
    return value;
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
#undef PRINT_STR
#undef PRINT_FORMAT_STR
#undef GET_STR_LABEL
#undef GET_IMM_LABEL