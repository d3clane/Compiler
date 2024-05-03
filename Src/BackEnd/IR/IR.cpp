#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IR.h"
#include "Tree/NameTable/NameTable.h"

static inline IROperandValue IROperandValueCreate(long long imm = 0, 
                                                  IRRegister reg = IRRegister::NO_REG, 
                                                  const char* string = nullptr,
                                                  IRErrors* error = nullptr);

static inline void IROperandValueDtor (IROperandValue* value);

static inline IROperand IROperandCtor();
static inline IROperand IROperandCreate(IROperandValue val, IROperandType type);
static inline IRNode*   IRNodeCtor();

static inline IROperand IROperandRegCreate(IRRegister reg);
static inline IROperand IROperandImmCreate(const long long imm);
static inline IROperand IROperandStrCreate(const char* str);
static inline IROperand IROperandMemCreate(const long long imm, IRRegister reg);

static inline IR* IRCtor();

static IROperand AsmCodeBuildFunc(const TreeNode* node, const NameTableType* allNamesTable, 
                             size_t* varRamId, IR* ir);

static IROperand AsmCodePrintStringLiteral(const TreeNode* node, NameTableType* localTable, 
                          const NameTableType* allNamesTable, IR* ir);   

static IROperand NameTablePushFuncParams(const TreeNode* node, NameTableType* local, 
                                    const NameTableType* allNamesTable, size_t* varRamId);
static IROperand AsmCodeBuildIf(const TreeNode* node, NameTableType* localTable, 
                             const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                             size_t numberOfTabs);
static IROperand AsmCodeBuildWhile(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                              size_t numberOfTabs);
static IROperand AsmCodeBuildAssign(const TreeNode* node, NameTableType* localTable, 
                               const NameTableType* allNamesTable, size_t* varRamId, IR* ir,
                               size_t numberOfTabs);
static IROperand AsmCodeBuildAnd(const TreeNode* node, NameTableType* localTable, 
                            const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                            size_t numberOfTabs);
static IROperand AsmCodeBuildOr(const TreeNode* node, NameTableType* localTable, 
                            const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                            size_t numberOfTabs);
static IROperand AsmCodeBuildFuncCall(const TreeNode* node, NameTableType* localTable, 
                                 const NameTableType* allNamesTable, IR* ir,
                                 size_t numberOfTabs);
static IROperand AsmCodeBuildEq(const TreeNode* node, NameTableType* localTable, 
                            const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                            size_t numberOfTabs);
static IROperand AsmCodeBuildNotEq(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                              size_t numberOfTabs);
static IROperand AsmCodeBuildGreaterEq(const TreeNode* node, NameTableType* localTable, 
                                  const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                                  size_t numberOfTabs);
static IROperand AsmCodeBuildLessEq(const TreeNode* node, NameTableType* localTable, 
                               const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                               size_t numberOfTabs);
static IROperand AsmCodeBuildLess(const TreeNode* node, NameTableType* localTable, 
                             const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                             size_t numberOfTabs);
static IROperand AsmCodeBuildGreater(const TreeNode* node, NameTableType* localTable, 
                                const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                                size_t numberOfTabs);

static IROperand AsmCodeBuildPrint(const TreeNode* node, NameTableType* localTable, 
                        const NameTableType* allNamesTable, IR* ir);

#define PRINT(...) FprintfLine(outStream, __VA_ARGS__)
#define CODE_BUILD(NODE) AsmCodeBuild(NODE, localTable, allNamesTable, outStream)

#define PARAMS(NODE)                NODE, localTable, allNamesTable, outStream
#define PARAMS_WITH_RAM_ID(NODE)    NODE, localTable, allNamesTable, &varRamId, outStream
#define PARAMS_WITH_LABEL_ID(NODE)  NODE, localTable, allNamesTable, &labelId,  outStream

static void BuildIR                    (const TreeNode* node, NameTableType* localTable, 
                                        const NameTableType* allNamesTable, IR* ir);

static void BuildArithmeticOp          (const TreeNode* node, NameTableType* localTable,
                                        const NameTableType* allNamesTable, IR* ir);

static void BuildFuncCall              (const TreeNode* node, NameTableType* localTable, 
                                        const NameTableType* allNamesTable, IR* ir);

static void BuildFunc(const TreeNode* node, const NameTableType* allNamesTable, IR* ir);

static size_t InitFuncParams(const TreeNode* node, NameTableType* local, 
                             const NameTableType* allNamesTable, 
                             int memShift, IRRegister regShift);

static int InitFuncLocalVars(const TreeNode* node, NameTableType* local,
                             const NameTableType* allNamesTable, 
                             int* memShift, IRRegister regShift);
                             
#define TYPE(IR_TYPE)      IROperandType::IR_TYPE
#define IR_REG(REG_NAME)   IRRegister::REG_NAME  
#define OP(OP_NAME)        IROperation::OP_NAME
#define EMPTY_OPERAND      IROperandCtor()
#define IR_PUSH(NODE)      IRPushBack(ir, NODE)
#define CREATE_VALUE(...)  IROperandValueCreate(__VA_ARGS__)

IR* BuildIR(Tree* tree, const NameTableType* allNamesTable)
{
    assert(tree);
    assert(allNamesTable);

    IR* ir = IRCtor();
    
    BuildIR(tree->root, nullptr, allNamesTable, ir);

    IR_PUSH(IRNodeCreate(OP(RET), nullptr, 0, EMPTY_OPERAND, EMPTY_OPERAND));
}

static void BuildIR(const TreeNode* node, NameTableType* localTable, 
                    const NameTableType* allNamesTable, IR* ir)
{
    static size_t labelId  = 0;

    if (node == nullptr)
        return;

    if (node->valueType == TreeNodeValueType::NUM)
    {
        BuildNum(node, ir);
        return;
    }

    if (node->valueType == TreeNodeValueType::NAME)
    {
        BuildVar(node, localTable, allNamesTable, ir);

        return;

        assert(!node->left && !node->right);

        Name* varName = nullptr;
        NameTableFind(localTable, allNamesTable->data[node->value.nameId].name, &varName);
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
            BuildArithmeticOp(node, localTable, allNamesTable, ir);
            break;

        case TreeOperationId::COMMA:
        {
            assert(false);
            // Это надо обработать в передаче аргументов функции и никак иначе

            // TODO:
            // BUILD(node->right);
            // BUILD(node->left);

            // Comma goes in the opposite way, because it is used here only for func call
            break;
        }
        case TreeOperationId::NEW_FUNC:
        case TreeOperationId::TYPE:
        case TreeOperationId::LINE_END:
        {
            BuildIR(node->left,  localTable, allNamesTable, ir);
            BuildIR(node->right, localTable, allNamesTable, ir);
            return;
        }

        case TreeOperationId::TYPE_INT:
            return;
    
        case TreeOperationId::FUNC:
        {
            BuildFunc(node, allNamesTable, ir);
            
            return;
        }

        case TreeOperationId::FUNC_CALL:
        {
            BuildFuncCall(node, localTable, allNamesTable, ir);
            return;
        }

        case TreeOperationId::IF:
        {
            BuildIf(node, localTable, allNamesTable, &labelId, ir);

            return;
        }

        case TreeOperationId::WHILE:
        {
            BuildWhile(node, localTable, allNamesTable, &labelId, ir);

            return;
        }

        case TreeOperationId::ASSIGN:
        {
            BuildAssign(node, localTable, allNamesTable, ir);
            break;
        }

        case TreeOperationId::RETURN:
        {
            BuildReturn(node, localTable, allNamesTable, ir);

            //BUILD(node->left);

            //TODO: вопрос оставлять так или двигать в функцию. Наверное двигать в функцию стоит
            //IRPushBack(ir->end, 
            //   IRNodeCreate(IROperation::RET, nullptr, 0, IROperandCtor(), IROperandCtor()));
            
            return;
        }

        case TreeOperationId::EQ:
        case TreeOperationId::NOT_EQ:
        case TreeOperationId::LESS:
        case TreeOperationId::LESS_EQ:
        case TreeOperationId::GREATER:
        case TreeOperationId::GREATER_EQ:
            return BuildComparison(node, localTable, allNamesTable, &labelId, ir);

        case TreeOperationId::READ:
            return BuildRead(ir);

        case TreeOperationId::PRINT:
        {
            BuildPrint(node, localTable, allNamesTable, ir);

            break;
        }

        default:
            assert(false);
            break;
    }
}

static void BuildArithmeticOp(const TreeNode* node, NameTableType* localTable,
                              const NameTableType* allNamesTable, IR* ir)
{
    assert(node);
    assert(localTable);
    assert(allNamesTable);
    assert(ir);

    IROperand operand1 = IROperandCreate(CREATE_VALUE(0, IR_REG(XMM0)), TYPE(REG));
    IROperand operand2 = IROperandCreate(CREATE_VALUE(0, IR_REG(XMM1)), TYPE(REG));

#define GENERATE_OPERATION_CMD(OPERATION_ID, EMPTY1, CHILDREN_CNT, ...)                         \
    case TreeOperationId::OPERATION_ID:                                                         \
    {                                                                                           \
        assert(CHILDREN_CNT > 0);                                                               \
        BuildIR(node->left,  localTable, allNamesTable, ir);                                    \
        if (CHILDREN_CNT == 2)                                                                  \
        {                                                                                       \
            BuildIR(node->right, localTable, allNamesTable, ir);                                \
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

static void BuildFunc(const TreeNode* node, const NameTableType* allNamesTable, IR* ir)
{
    assert(node);
    assert(allNamesTable);
    assert(ir);
    assert(node->left);
    assert(node->left->valueType == TreeNodeValueType::NAME);

    TreeNode* funcNameNode = node->left;

    IR_PUSH(IRNodeCreate(OP(NOP), NameTableGetName(allNamesTable, funcNameNode->value.nameId), 
                         0, EMPTY_OPERAND, EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(PUSH), nullptr, 1, IROperandRegCreate(IR_REG(RBP)), EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(MOV),  nullptr, 2, IROperandRegCreate(IR_REG(RBP)), 
                                               IROperandRegCreate(IR_REG(RSP))));

    NameTableType* localTable = nullptr;
    NameTableCtor(&localTable);

    NameTableSetLocalTable(allNamesTable, funcNameNode->value.nameId, localTable);

    static int        memShift = 0;
    static IRRegister regShift = IRRegister::RBP;

    memShift = 2 * (int)RXX_REG_BYTE_SIZE; // func addr + rbp
    size_t numberOfParams = InitFuncParams(funcNameNode->left, localTable, allNamesTable, 
                                           memShift, regShift);

    memShift = 0;
    int rspShift = InitFuncLocalVars(funcNameNode->right, localTable, allNamesTable, 
                                     &memShift, regShift);

    IR_PUSH(IRNodeCreate(OP(ADD), nullptr, 2, IROperandRegCreate(IR_REG(RSP)), 
                                              IROperandImmCreate((long long)rspShift)));

    BuildIR(funcNameNode->right, localTable, allNamesTable, ir);

    // pop return value in XMM0
    IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)),
                                                EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(MOV), nullptr, 2, IROperandRegCreate(IR_REG(RSP)), 
                                              IROperandRegCreate(IR_REG(RBP))));

    IR_PUSH(IRNodeCreate(OP(POP), nullptr, 1, IROperandRegCreate(IR_REG(RBP)), EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(RET), nullptr, 1, IROperandImmCreate((long long)numberOfParams),
                                              EMPTY_OPERAND));                                              
}

// Pascal decl
static size_t InitFuncParams(const TreeNode* node, NameTableType* local, 
                             const NameTableType* allNamesTable, 
                             int memShift, IRRegister regShift)
{
    assert(node);
    assert(local);
    assert(allNamesTable);

    if (node->valueType == TreeNodeValueType::NAME)
    {
        Name pushName = {};
        NameCtor(&pushName, allNamesTable->data[node->value.nameId].name, nullptr, 
                 memShift, regShift);
        // TODO: Name ctors strdups name. 
        // It's kind of useless when I am creating local table - 
        // I'm not going to delete local tables strings without deleting global table
        // Just using extra mem + time. 

        // TODO: mem leak never DTOR name table. + Create recursive name table dtor

        NameTablePush(local, pushName);

        return 1;
    }

    assert(node->valueType == TreeNodeValueType::OPERATION);

    switch (node->value.operation)
    {
        case TreeOperationId::COMMA:
            return 
                InitFuncParams(node->left,  local, allNamesTable, 
                               memShift + XMM_REG_BYTE_SIZE, regShift) +
                InitFuncParams(node->right, local, allNamesTable, memShift, regShift);

        case TreeOperationId::TYPE:
            return InitFuncParams(node->right, local, allNamesTable, memShift, regShift);

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

static int InitFuncLocalVars(const TreeNode* node, NameTableType* local,
                             const NameTableType* allNamesTable, 
                             int* memShift, IRRegister regShift)
{
    assert(local);
    assert(allNamesTable);
    assert(memShift);

    if (node == nullptr)
        return 0;
    
    if (node->valueType == TreeNodeValueType::OPERATION && 
        node->value.operation == TreeOperationId::TYPE)
    {
        assert(node->right);
        assert(node->right->value.operation == TreeOperationId::ASSIGN);
        assert(node->right->left);
        assert(node->right->left->valueType == TreeNodeValueType::NAME);

        const char* name = NameTableGetName(allNamesTable, node->right->left->value.nameId);
        Name pushName = {};

        *memShift -= (int)XMM_REG_BYTE_SIZE;
        NameCtor(&pushName, name, nullptr, *memShift, regShift);

        NameTablePush(local, pushName);

        return -(int)XMM_REG_BYTE_SIZE;
    }

    return InitFuncLocalVars(node->left,  local, allNamesTable, memShift, regShift) +
           InitFuncLocalVars(node->right, local, allNamesTable, memShift, regShift);
}

static void BuildFuncCall              (const TreeNode* node, const NameTableType* localTable, 
                                        const NameTableType* allNamesTable, IR* ir)
{
    assert(node->left->valueType == TreeNodeValueType::NAME);

    // No registers saving because they are used only temporary
    PushFuncCallArgs(node->left->left, localTable, allNamesTable, ir);

    const char* funcName = NameTableGetName(allNamesTable, node->left->value.nameId);

    IR_PUSH(IRNodeCreate(OP(CALL), nullptr, 1, IROperandStrCreate(funcName),
                                               EMPTY_OPERAND));
    // pushing ret value on stack
    IR_PUSH(IRNodeCreate(OP(F_PUSH), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)),
                                                 EMPTY_OPERAND));
}

static void PushFuncCallArgs(const TreeNode* node, const NameTableType* localTable, 
                             const NameTableType* allNamesTable, IR* ir)
{
    assert(node);
    assert(localTable);
    assert(allNamesTable);

    switch (node->valueType)
    {
        case TreeNodeValueType::NAME:
        {
            Name* name = nullptr;
            NameTableFind(localTable, 
                          NameTableGetName(allNamesTable, node->value.nameId), &name);
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

            PushFuncCallArgs(node->left,  localTable, allNamesTable, ir);
            PushFuncCallArgs(node->right, localTable, allNamesTable, ir);
            break;
        }

        default: // Unreachable
        {
            assert(false);

            break;            
        }
    }
}

static IROperand AsmCodeBuildIf(const TreeNode* node, NameTableType* localTable, 
                           const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                           size_t numberOfTabs)
{
    fprintf(outStream, "\n");
    PRINT("@ if condition:\n");

    BUILD(node->left);

    size_t id = *labelId;
    PRINT("push 0\n");
    PRINT("je END_IF_%zu:\n", id);
    *labelId += 1;

    PRINT("@ if code block:\n");

    BuildIR(node->right, localTable, allNamesTable, outStream);
    
    PRINT("END_IF_%zu:\n\n", id);
}

static IROperand AsmCodeBuildWhile(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, IR* ir,
                              size_t numberOfTabs)
{
    size_t id = *labelId;

    fprintf(outStream, "\n");

    PRINT("while_%zu:\n", id);

    *labelId += 1;
    PRINT("@ while condition: \n");

    BUILD(node->left);

    PRINT("push 0\n");
    PRINT("je END_WHILE_%zu:\n", id);

    *labelId += 1;

    PRINT("@ while code block: \n");

    BuildIR(node->right, localTable, allNamesTable, outStream);

    PRINT("END_WHILE_%zu:\n\n", id);
}

static IROperand AsmCodeBuildAssign(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* varRamId, IR* ir,
                              size_t numberOfTabs)
{
    BUILD(node->right);

    assert(node->left->valueType == TreeNodeValueType::NAME);

    Name* varNameInTablePtr = nullptr;
    NameTableFind(localTable, allNamesTable->data[node->left->value.nameId].name, &varNameInTablePtr);

    // TODO: it is hotfix, a lot of operations are done
    // better fix: split assigning and defining and push only once
    if (varNameInTablePtr == nullptr)
    {
        Name pushName = {};
        NameCtor(&pushName, allNamesTable->data[node->left->value.nameId].name, nullptr, *varRamId);

        *varRamId += 1;

        NameTablePush(localTable, pushName);
        varNameInTablePtr = localTable->data + localTable->size - 1;    // pointing on pushed name
    }

    PRINT("pop [%zu]\n", varNameInTablePtr->varRamId);
}

static IROperand AsmCodeBuildAnd(const TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, IR* ir,
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
                           const NameTableType* allNamesTable, size_t* labelId, IR* ir,
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
                                 const NameTableType* allNamesTable, IR* ir,
                                 size_t numberOfTabs)
{
    PRINT("@ pushing func local vars:\n");

    for (size_t i = 0; i < localTable->size; ++i)
    {
        PRINT("push [%zu]\n", localTable->data[i].varRamId);
    }

    PRINT("@ pushing func args:\n");
    BUILD(node->left->left);

    assert(node->left->valueType == TreeNodeValueType::NAME);

    PRINT("call %s:\n", allNamesTable->data[node->left->value.nameId].name);

    PRINT("@ saving func return\n");

    PRINT("pop rax\n");

    for (int i = (int)localTable->size - 1; i > -1; --i)
    {
        PRINT("pop [%zu]\n", localTable->data[i].varRamId);
    }

    PRINT("push rax\n");
}

static IROperand AsmCodeBuildEq(const TreeNode* node, NameTableType* localTable, 
                           const NameTableType* allNamesTable, size_t* labelId, IR* ir,
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
                              const NameTableType* allNamesTable, size_t* labelId, IR* ir,
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
                              const NameTableType* allNamesTable, size_t* labelId, IR* ir,
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
                              const NameTableType* allNamesTable, size_t* labelId, IR* ir,
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
                              const NameTableType* allNamesTable, size_t* labelId, IR* ir,
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
                                const NameTableType* allNamesTable, size_t* labelId, IR* ir,
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
                          const NameTableType* allNamesTable, IR* ir)
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
                        const NameTableType* allNamesTable, IR* ir)
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

static IROperand PrintTabs(const size_t numberOfTabs, IR* ir)
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

static inline IROperandValue IROperandValueCreate(long long imm, IRRegister reg, const char* string,
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

static inline IROperand IROperandCreate(IROperandValue val, IROperandType type)
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

void IRPushBack  (IR* ir, IRNode node, IRErrors* error)
{
    // TODO:
}

IRNode* IRNodeCreate(IROperation operation, const char* labelName, 
                     size_t numberOfOperands, IROperand operand1, IROperand operand2)
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
    return IROperandCreate(CREATE_VALUE(0, 0, str), TYPE(STR));
}
