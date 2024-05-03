#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IR.h"
#include "Tree/NameTable/NameTable.h"

struct CompilerInfoState
{
    NameTableType* localTable;
    const NameTableType* allNamesTable;

    IR* ir;

    size_t labelId;

    size_t     memShift;
    IRRegister regShift;

    size_t numberOfFuncParams;
};

static inline CompilerInfoState CompilerInfoStateCtor();
static inline void              CompilerInfoStateDtor(CompilerInfoState* info);

static inline IROperandValue IROperandValueCreate(long long imm = 0, 
                                                  IRRegister reg = IRRegister::NO_REG, 
                                                  const char* string = nullptr,
                                                  IRErrors* error = nullptr);

static inline void IROperandValueDtor (IROperandValue* value);

static inline IROperand IROperandCtor();
static inline IROperand IROperandCreate(IROperandValue val, info->irOperandType type);
static inline IRNode*   IRNodeCtor();

static inline IROperand IROperandRegCreate(IRRegister reg);
static inline IROperand IROperandImmCreate(const long long imm);
static inline IROperand IROperandStrCreate(const char* str);
static inline IROperand IROperandMemCreate(const long long imm, info->irRegister reg);

static inline IR* IRCtor();

static void     BuildIR             (const TreeNode* node, CompilerInfoState* info);
static void     BuildArithmeticOp   (const TreeNode* node, CompilerInfoState* info);
static void     BuildFuncCall       (const TreeNode* node, CompilerInfoState* info);
static void     PushFuncCallArgs    (const TreeNode* node, CompilerInfoState* info);
static void     BuildFunc           (const TreeNode* node, CompilerInfoState* info);
static size_t   InitFuncParams      (const TreeNode* node, CompilerInfoState* info);
static int      InitFuncLocalVars   (const TreeNode* node, CompilerInfoState* info);

static inline const char* CreateTmpStringDANGEROUS(const char* format, ...);

#define TYPE(IR_TYPE)      IROperandType::IR_TYPE
#define IR_REG(REG_NAME)   IRRegister::REG_NAME  
#define OP(OP_NAME)        IROperation::OP_NAME
#define EMPTY_OPERAND      IROperandCtor()
#define IR_PUSH(NODE)      IRPushBack(info->ir, NODE)
#define CREATE_VALUE(...)  IROperandValueCreate(__VA_ARGS__)

#define TMP_STR(FORMAT, ...) CreateTmpStringDANGEROUS(FORMAT, __VA_ARGS__)

IR* BuildIR(Tree* tree, const NameTableType* allNamesTable)
{
    assert(tree);
    assert(allNamesTable);

    IR* ir = IRCtor();
    
    CompilerInfoState state = CompilerInfoStateCtor();
    state.allNamesTable = allNamesTable;
    state.ir = ir;
    
    BuildIR(tree->root, &state);

    IR_PUSH(IRNodeCreate(OP(RET), nullptr, 0, EMPTY_OPERAND, EMPTY_OPERAND));
}

static void BuildIR(const TreeNode* node, CompilerInfoState* info)
{
    if (node == nullptr)
        return;

    if (node->valueType == TreeNodeValueType::NUM)
    {
        BuildNum(node, info);
        return;
    }

    if (node->valueType == TreeNodeValueType::NAME)
    {
        BuildVar(node, info);

        return;

        assert(!node->left && !node->right);

        Name* varName = nullptr;
        NameTableFind(info->localTable, info->allNamesTable->data[node->value.nameId].name, &varName);
        assert(varName);

        //return IROperandCreate(CREATE_VALUE(varName->memShift, varName->reg), TYPE(MEM));
    }

    assert(node->valueType == TreeNodeValueType::OPERATION);

    switch (node->value.operation)
    {
        case TreeOperationId::ADD:
        case TreeOperationId::SUB:
        case TreeOperationId::MUL:
        case TreeOperationId::DIV:
        case TreeOperationId::AND:
        case TreeOperationId::OR:
        case TreeOperationId::SIN:
        case TreeOperationId::COS:
        case TreeOperationId::TAN:
        case TreeOperationId::COT:
        case TreeOperationId::SQRT:
        {
            BuildArithmeticOp(node, info);
            break;
        }

        case TreeOperationId::NEW_FUNC:
        case TreeOperationId::TYPE:
        case TreeOperationId::LINE_END:
        {
            BuildIR(node->left,  info);
            BuildIR(node->right, info);
            break;
        }

        case TreeOperationId::TYPE_INT:
            break;;
    
        case TreeOperationId::FUNC:
        {
            BuildFunc(node, info);
            break;
        }

        case TreeOperationId::FUNC_CALL:
        {
            BuildFuncCall(node, info);
            break;;
        }

        case TreeOperationId::IF:
        {
            BuildIf(node, info);
            break;
        }

        case TreeOperationId::WHILE:
        {
            BuildWhile(node, info);
            break;
        }

        case TreeOperationId::ASSIGN:
        {
            BuildAssign(node, info);
            break;
        }

        case TreeOperationId::RETURN:
        {
            BuildReturn(node, info);
            break;
        }

        case TreeOperationId::EQ:
        case TreeOperationId::NOT_EQ:
        case TreeOperationId::LESS:
        case TreeOperationId::LESS_EQ:
        case TreeOperationId::GREATER:
        case TreeOperationId::GREATER_EQ:
        {
            BuildComparison(node, info);
            break;
        }

        case TreeOperationId::READ
        {
            BuildRead(ir, info);
            break;
        }

        case TreeOperationId::PRINT:
        {
            BuildPrint(node, info);
            break;
        }

        default:
            assert(false);
            break;
    }
}

static void BuildArithmeticOp(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);
    assert(info->ir);

    IROperand operand1 = IROperandRegCreate(IR_REG(XMM0));
    IROperand operand2 = IROperandRegCreate(IR_REG(XMM1));

#define GENERATE_OPERATION_CMD(OPERATION_ID, EMPTY1, CHILDREN_CNT, ...)                         \
    case TreeOperationId::OPERATION_ID:                                                         \
    {                                                                                           \
        assert(CHILDREN_CNT > 0);                                                               \
        BuildIR(node->left, info);                                                              \
        if (CHILDREN_CNT == 2)                                                                  \
        {                                                                                       \
            BuildIR(node->right, info);                                                         \
            IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 1, operand2, EMPTY_OPERAND));              \
        }                                                                                       \
        else operand2 = EMPTY_OPERAND;                                                          \
                                                                                                \
        IR_PUSH(IRNodeCreate(OP(F_POP),            nullptr, 1, operand1, EMPTY_OPERAND));       \
        IR_PUSH(IRNodeCreate(OP(F_##OPERATION_ID), nullptr, 2, operand1, operand2));            \
        break;                                                                                  \
    }

    switch (node->value.operation)
    {
        #include "Tree/Operations.h"    // cases on defines

        default:    // Unreachable
            assert(false);
            break;
    }

#undef GENERATE_OPERATION_CMD

    IR_PUSH(IRNodeCreate(OP(F_PUSH), nullptr, 1, operand1, EMPTY_OPERAND));

    return;
}

static void BuildFunc(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->allNamesTable);
    assert(info->ir);
    assert(node->left);
    assert(node->left->valueType == TreeNodeValueType::NAME);

    TreeNode* funcNameNode = node->left;

    IR_PUSH(IRNodeCreate(OP(NOP), NameTableGetName(info->allNamesTable, funcNameNode->value.nameId), 
                         0, EMPTY_OPERAND, EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(PUSH), nullptr, 1, IROperandRegCreate(IR_REG(RBP)), EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(MOV),  nullptr, 2, IROperandRegCreate(IR_REG(RBP)), 
                                               IROperandRegCreate(IR_REG(RSP))));

    NameTableType* localTable = nullptr;
    NameTableCtor(&localTable);
    info->localTable = localTable;

    NameTableSetLocalTable(info->allNamesTable, funcNameNode->value.nameId, info->localTable);

    info->memShift = 2 * (int)RXX_REG_BYTE_SIZE;
    info->regShift = IR_REG(RBP);

    info->numberOfFuncParams = InitFuncParams(funcNameNode->left, info);

    info->memShift = 0;
    int rspShift = InitFuncLocalVars(funcNameNode->right, info);

    IR_PUSH(IRNodeCreate(OP(ADD), nullptr, 2, IROperandRegCreate(IR_REG(RSP)), 
                                              IROperandImmCreate((long long)rspShift)));

    BuildIR(funcNameNode->right, info);
}

// Pascal decl
static size_t InitFuncParams(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);

    if (node->valueType == TreeNodeValueType::NAME)
    {
        Name pushName = {};
        NameCtor(&pushName, NameTableGetName(info->allNamesTable, node->value.nameId), nullptr, 
                 info->memShift, info->regShift);
        // TODO: Name ctors strdups name. 
        // It's kind of useless when I am creating local table - 
        // I'm not going to delete local tables strings without deleting global table
        // Just using extra mem + time. 

        // TODO: mem leak never DTOR name table. + Create recursive name table dtor

        NameTablePush(info->localTable, pushName);

        return 1;
    }

    assert(node->valueType == TreeNodeValueType::OPERATION);

    switch (node->value.operation)
    {
        case TreeOperationId::COMMA:
        {
            size_t cntRight = InitFuncParams(node->right, info);

            info->memShift += XMM_REG_BYTE_SIZE;

            return cntRight + InitFuncParams(node->left, info);
        }

        case TreeOperationId::TYPE:
            return InitFuncParams(node->right, info);

        default: // Unreachable
        {
            assert(false);
            break;
        }
    }   

    // Unreachable

    assert(false);
    return 0;
}

static int InitFuncLocalVars(const TreeNode* node, CompilerInfoState* info)
{
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);

    if (node == nullptr)
        return 0;
    
    if (node->valueType == TreeNodeValueType::OPERATION && 
        node->value.operation == TreeOperationId::TYPE)
    {
        assert(node->right);
        assert(node->right->value.operation == TreeOperationId::ASSIGN);
        assert(node->right->left);
        assert(node->right->left->valueType == TreeNodeValueType::NAME);

        const char* name = NameTableGetName(info->allNamesTable, node->right->left->value.nameId);
        Name pushName = {};

        info->memShift -= (int)XMM_REG_BYTE_SIZE;
        NameCtor(&pushName, name, nullptr, info->memShift, info->regShift);

        NameTablePush(info->localTable, pushName);

        return -(int)XMM_REG_BYTE_SIZE;
    }

    return InitFuncLocalVars(node->left, info) + InitFuncLocalVars(node->right, info);
}

static void BuildFuncCall(const TreeNode* node, CompilerInfoState* info)
{
    assert(node->left->valueType == TreeNodeValueType::NAME);

    // No registers saving because they are used only temporary
    PushFuncCallArgs(node->left->left, info);

    const char* funcName = NameTableGetName(info->allNamesTable, node->left->value.nameId);

    IR_PUSH(IRNodeCreate(OP(CALL), nullptr, 1, IROperandStrCreate(funcName), EMPTY_OPERAND));

    // pushing ret value on stack
    IR_PUSH(IRNodeCreate(OP(F_PUSH), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));
}

static void PushFuncCallArgs(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);

    switch (node->valueType)
    {
        case TreeNodeValueType::NAME:
        {
            Name* name = nullptr;
            NameTableFind(info->localTable, 
                          NameTableGetName(info->allNamesTable, node->value.nameId), &name);
            // TODO: вместо поиска можно внутри ноды заапдейтить nameId
            // или сделать стек какой-то из nameid потому что реально желательно в две стороны                         

            IR_PUSH(IRNodeCreate(OP(F_MOV), nullptr, 2, 
                                 IROperandRegCreate(IR_REG(XMM0)),
                                 IROperandMemCreate(name->memShift, name->reg)));

            IR_PUSH(IRNodeCreate(OP(F_PUSH), nullptr, 1, 
                                 IROperandRegCreate(IR_REG(XMM0)),
                                 EMPTY_OPERAND));

            break;
        }

        case TreeNodeValueType::OPERATION:
        {          
            assert(node->value.operation == TreeOperationId::COMMA);

            PushFuncCallArgs(node->left,  info);
            PushFuncCallArgs(node->right, info);
            break;
        }

        default: // Unreachable
        {
            assert(false);

            break;            
        }
    }
}

static void BuildIf(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);
    assert(info->ir);

    // TODO: можно отдельную функцию, где иду в condition и там, основываясь сразу на сравнении,
    // Деляю вывод о jump to if end / not jump (типо на стек не кладу, выгодно по времени)

    size_t id = info->labelId;
    info->labelId += 1;

    static const size_t maxLabelLen = 64;
    char ifEndLabel[maxLabelLen] = "";
    snprintf(ifEndLabel, maxLabelLen, "END_IF_%zu", id);

    BuildIR(node->left, info);
    

    IR_PUSH(IRNodeCreate(OP(POP), nullptr, 2, IROperandRegCreate(IR_REG(RAX)), EMPTY_OPERAND));
    
    IR_PUSH(IRNodeCreate(OP(TEST), nullptr, 2, IROperandRegCreate(IR_REG(RAX)), 
                                               IROperandRegCreate(IR_REG(RAX))));

    IR_PUSH(IRNodeCreate(OP(JE), nullptr, 1, IROperandStrCreate(ifEndLabel), 
                                             EMPTY_OPERAND));

    BuildIR(node->right, info);

    IR_PUSH(IRNodeCreate(OP(NOP), ifEndLabel, 0, EMPTY_OPERAND, EMPTY_OPERAND));
}

static void BuildWhile(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);
    assert(info->ir);

    size_t id = info->labelId;
    info->labelId += 1;

    static const size_t maxLabelLen = 64;
    char whileBeginLabel[maxLabelLen] = "";
    char whileEndLabel  [maxLabelLen] = "";
    snprintf(whileBeginLabel, maxLabelLen, "WHILE_%zu",     id);
    snprintf(whileEndLabel,   maxLabelLen, "END_WHILE_%zu", id);

    IR_PUSH(IRNodeCreate(OP(NOP), whileBeginLabel, 0, EMPTY_OPERAND, EMPTY_OPERAND));

    BuildIR(node->left, info);

    IR_PUSH(IRNodeCreate(OP(POP), nullptr, 2, IROperandRegCreate(IR_REG(RAX)), EMPTY_OPERAND));
    
    IR_PUSH(IRNodeCreate(OP(TEST), nullptr, 2, IROperandRegCreate(IR_REG(RAX)), 
                                               IROperandRegCreate(IR_REG(RAX))));
                                               
    IR_PUSH(IRNodeCreate(OP(JE), nullptr, 1, IROperandStrCreate(whileEndLabel), EMPTY_OPERAND));

    BuildIR(node->right, info);

    IR_PUSH(IRNodeCreate(OP(JMP), nullptr, 1, IROperandStrCreate(whileBeginLabel), EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(NOP), whileEndLabel, 0, EMPTY_OPERAND, EMPTY_OPERAND));
}

static void BuildAssign(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);
    assert(info->ir);

    assert(node->left->valueType == TreeNodeValueType::NAME);
    
    Name* varName = nullptr;
    NameTableFind(info->localTable, 
                  NameTableGetName(info->allNamesTable, node->left->value.nameId), &varName);

    assert(varName);

    BuildIR(node->right, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(F_MOV), nullptr, 2, IROperandMemCreate(varName->memShift, varName->reg),
                                                IROperandRegCreate(IR_REG(XMM0))));
}

static void BuildReturn(const TreeNode* node, CompilerInfoState* info)
{
    BuildIR(node->left, info);
    
    IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(MOV), nullptr, 2, IROperandRegCreate(IR_REG(RSP)), 
                                              IROperandRegCreate(IR_REG(RBP))));

    IR_PUSH(IRNodeCreate(OP(POP), nullptr, 1, IROperandRegCreate(IR_REG(RBP)), EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(RET), nullptr, 1, IROperandImmCreate((long long)info->numberOfFuncParams),
                                              EMPTY_OPERAND));    
}

static IROperand AsmCodeBuildAnd(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, info->ir* ir,
                              size_t numberOfTabs)
{
    BUILD(node->left);
    BUILD(node->right);

    PRINT("mul\n");
    PRINT("push 0\n");
    PRINT("je AND_FALSE_%zu:\n", *labelId);
    PRINT("push 1\n");
    PRINT("jmp AND_END_%zu:\n", *labelId);
    PRINT("AND_FALSE_%zu:\n", *labelId);
    PRINT("push 0\n");
    PRINT("AND_END_%zu:\n\n", *labelId);

    *labelId += 1;
}

static IROperand AsmCodeBuildOr(const TreeNode* node, NameTableType* localTable, 
                           const NameTableType* allNamesTable, size_t* labelId, info->ir* ir,
                           size_t numberOfTabs)
{
    BUILD(node->left);

    size_t id = *labelId;

    PRINT("push 0\n");
    PRINT("je OR_FIRST_VAL_SET_ZERO_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("jmp OR_FIRST_VAL_END_%zu:\n", id);
    PRINT("OR_FIRST_VAL_SET_ZERO_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("OR_FIRST_VAL_END_%zu\n\n", id);

    *labelId += 1;

    BUILD(node->right);

    PRINT("push 0\n");
    PRINT("je OR_SECOND_VAL_SET_ZERO_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("jmp OR_SECOND_VAL_END_%zu:\n", id);
    PRINT("OR_SECOND_VAL_SET_ZERO_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("OR_SECOND_VAL_END_%zu\n\n", id);

    PRINT("add\n");
    PRINT("push 0\n");
    PRINT("je OR_FALSE_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("jmp OR_END_%zu:\n", id);
    PRINT("OR_FALSE_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("OR_END_%zu:\n\n", id);
}

static IROperand AsmCodeBuildFuncCall(const TreeNode* node, NameTableType* localTable, 
                                 const NameTableType* allNamesTable, info->ir* ir,
                                 size_t numberOfTabs)
{
    PRINT("@ pushing func local vars:\n");

    for (size_t i = 0; i < localTable->size; ++i)
    {
        PRINT("push [%zu]\n", info->localTable->data[i].varRamId);
    }

    PRINT("@ pushing func args:\n");
    BUILD(node->left->left);

    assert(node->left->valueType == TreeNodeValueType::NAME);

    PRINT("call %s:\n", info->allNamesTable->data[node->left->value.nameId].name);

    PRINT("@ saving func return\n");

    PRINT("pop rax\n");

    for (int i = (int)localTable->size - 1; i > -1; --i)
    {
        PRINT("pop [%zu]\n", info->localTable->data[i].varRamId);
    }

    PRINT("push rax\n");
}

static IROperand AsmCodeBuildEq(const TreeNode* node, NameTableType* localTable, 
                           const NameTableType* allNamesTable, size_t* labelId, info->ir* ir,
                           size_t numberOfTabs)
{
    BUILD(node->left);
    BUILD(node->right);

    size_t id = *labelId;
    PRINT("je EQ_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_EQ_%zu:\n", id);
    PRINT("EQ_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_EQ_%zu:\n\n", id);

    *labelId += 1;
}

static IROperand AsmCodeBuildNotEq(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, info->ir* ir,
                              size_t numberOfTabs)
{
    BUILD(node->left);
    BUILD(node->right);

    size_t id = *labelId;
    PRINT("jne NOT_EQ_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_NOT_EQ_%zu:\n", id);
    PRINT("NOT_EQ_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_NOT_EQ_%zu:\n\n", id);
    
    *labelId += 1;
}

static IROperand AsmCodeBuildLess(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, info->ir* ir,
                              size_t numberOfTabs)
{
    BUILD(node->left);
    BUILD(node->right);

    size_t id = *labelId;
    PRINT("jb LESS_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_LESS_%zu:\n", id);
    PRINT("LESS_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_LESS_%zu:\n\n", id);
    
    *labelId += 1;
}

static IROperand AsmCodeBuildLessEq(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, info->ir* ir,
                              size_t numberOfTabs)
{
    BUILD(node->left);
    BUILD(node->right);

    size_t id = *labelId;
    PRINT("jbe LESS_EQ_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_LESS_EQ_%zu:\n", id);
    PRINT("LESS_EQ_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_LESS_EQ_%zu:\n\n", id);
    
    *labelId += 1;
}

static IROperand AsmCodeBuildGreater(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, info->ir* ir,
                              size_t numberOfTabs)
{
    BUILD(node->left);
    BUILD(node->right);

    size_t id = *labelId;
    PRINT("ja GREATER_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_GREATER_%zu:\n", id);
    PRINT("GREATER_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_GREATER_%zu:\n\n", id);

    *labelId += 1;
}

static IROperand AsmCodeBuildGreaterEq(const TreeNode* node, NameTableType* localTable, 
                                const NameTableType* allNamesTable, size_t* labelId, info->ir* ir,
                                size_t numberOfTabs)
{
    BUILD(node->left);
    BUILD(node->right);

    size_t id = *labelId;
    PRINT("jae GREATER_EQ_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_GREATER_EQ_%zu:\n", id);
    PRINT("GREATER_EQ_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_GREATER_EQ_%zu:\n\n", id);

    *labelId += 1;
}

static IROperand AsmCodePrintStringLiteral(const TreeNode* node, NameTableType* localTable, 
                          const NameTableType* allNamesTable, info->ir* ir)
{
    const char* string = allNamesTable->data[node->value.nameId].name;

    size_t pos = 1; //skipping " char

    while (string[pos] != '"')
    {
        PRINT("push %d\n", string[pos]);
        PRINT("outc\n");
        PRINT("pop\n");
        ++pos;
    }

    PRINT("push 10\n");
    PRINT("outc\n");
    PRINT("pop\n");
}

static IROperand AsmCodeBuildPrint(const TreeNode* node, NameTableType* localTable, 
                        const NameTableType* allNamesTable, info->ir* ir)
{
    assert(node->left);

    if (node->left->valueType == TreeNodeValueType::STRING_LITERAL)
    {
        AsmCodePrintStringLiteral(PARAMS(node->left));

        return;
    }

    BUILD(node->left);

    PRINT("out\n");
    PRINT("pop\n");
}

static IROperand PrintTabs(const size_t numberOfTabs, info->ir* ir)
{
    for (size_t i = 0; i < numberOfTabs; ++i)
        fprintf(outStream, "    ");
}

static IROperand FprintfLine(IR* ir, const size_t numberOfTabs, const char* format, ...)
{
    va_list args = {};

    va_start(args, format);

    PrintTabs(numberOfTabs, outStream);
    vfprintf(outStream, format, args);

    va_end(args);
}

static inline const char* CreateTmpStringDANGEROUS(const char* format, ...)
{
    static const size_t maxBufSize = 256;
    static char tmpBuffer[maxBufSize] = "";

    va_list args = {};
    va_start(args, format);

    vsnprintf(tmpBuffer, maxBufSize, format, args);

    va_end(args);

    return tmpBuffer;
}

//-----------------------------------------------------------------------------

static inline void IROperandValueDtor(IROperandValue* value)
{
    if (!value)
        return;

    value->imm   = 0;
    value->reg = IRRegister::NO_REG;

    if (!value->string)
        return;

    free(value->string);
    value->string = nullptr;
}

static inline IROperandValue IROperandValueCreate(long long imm, info->irRegister reg, const char* string,
                                                  IRErrors* error)
{
    IROperandValue val = {};
    val.imm   = imm;
    val.reg = IRRegister::NO_REG;
    
    if (string)
    {
        val.string = strdup(string);

        if (error && !val.string) *error = IRErrors::MEM_ALLOC_ERR;
        else if (!error) assert(val.string);
    }

    return val;
}

static inline IROperand IROperandCtor()
{
    IROperand operand = 
    {
        .value = IROperandValueCreate(),
        .type  = IROperandType::IMM,
    };

    return operand;
}

static inline IROperand IROperandCreate(IROperandValue val, info->irOperandType type)
{
    IROperand operand = 
    {
        .value = val,
        .type  = type,
    };

    return operand;
}

static inline IRNode* IRNodeCtor()
{
    IRNode* node = (IRNode*)calloc(1, sizeof(*node));

    node->operation = IROperation::NOP;

    node->asmAddress = 0;
    node->jumpTarget   = nullptr;
    node->needPatch  = false;

    node->numberOfOperands = 0;
    node->operand1         = IROperandCtor();
    node->operand2         = IROperandCtor();

    node->nextNode   = nullptr;
    node->prevNode   = nullptr;

    return node;
}

static inline IR* IRCtor()
{
    IR* ir = (IR*)calloc(1, sizeof(*ir));

    ir->end = IRNodeCtor();
    ir->size  = 0;

    ir->end->nextNode = ir->end;
    ir->end->prevNode = ir->end;

    return ir;
}

void IRPushBack  (IR* ir, info->irNode node, info->irErrors* error)
{
    // TODO:
}

IRNode* IRNodeCreate(IROperation operation, const char* labelName, 
                     size_t numberOfOperands, info->irOperand operand1, info->irOperand operand2)
{
    // TODO:
}

static inline IROperand IROperandRegCreate(IRRegister reg)
{
    return IROperandCreate(CREATE_VALUE(0, reg), TYPE(REG));
}

static inline IROperand IROperandImmCreate(const long long imm)
{
    return IROperandCreate(CREATE_VALUE(imm), TYPE(IMM));
}

static inline IROperand IROperandStrCreate(const char* str)
{
    return IROperandCreate(CREATE_VALUE(0, info->ir_REG(NO_REG), str), TYPE(STR));
}

static inline CompilerInfoState CompilerInfoStateCtor()
{
    CompilerInfoState state  = {};
    state.allNamesTable      = nullptr;
    state.localTable         = nullptr;
    state.labelId            = nullptr;
    state.ir                 = nullptr;

    state.memShift           = 0;
    state.numberOfFuncParams = 0;
    state.regShift           = IR_REG(NO_REG);
    
    return state;
}

static inline void              CompilerInfoStateDtor(CompilerInfoState* info)
{
    info->allNamesTable      = nullptr;
    info->localTable         = nullptr;
    info->labelId            = nullptr;
    info->ir                 = nullptr;

    info->memShift           = 0;
    info->numberOfFuncParams = 0;
    info->regShift           = IR_REG(NO_REG);
}