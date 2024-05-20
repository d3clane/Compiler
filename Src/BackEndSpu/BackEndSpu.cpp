#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "Tree/NameTable/NameTable.h"
#include "BackEndSpu.h"

static void AsmCodeBuild(TreeNode* node, NameTableType* localTable, 
                         const NameTableType* allNamesTable, FILE* outStream, size_t numberOfTabs);

static void AsmCodeBuildFunc(TreeNode* node, const NameTableType* allNamesTable, 
                             size_t* varRamId, FILE* outStream, size_t numberOfTabs);

static void AsmCodePrintStringLiteral(TreeNode* node, NameTableType* localTable, 
                          const NameTableType* allNamesTable, FILE* outStream, size_t numberOfTabs);   

static void NameTablePushFuncParams(TreeNode* node, NameTableType* local, 
                                    const NameTableType* allNamesTable, size_t* varRamId);
static void AsmCodeBuildIf(TreeNode* node, NameTableType* localTable, 
                             const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                             size_t numberOfTabs);
static void AsmCodeBuildWhile(TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                              size_t numberOfTabs);
static void AsmCodeBuildAssign(TreeNode* node, NameTableType* localTable, 
                               const NameTableType* allNamesTable, size_t* varRamId, FILE* outStream,
                               size_t numberOfTabs);
static void AsmCodeBuildAnd(TreeNode* node, NameTableType* localTable, 
                            const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                            size_t numberOfTabs);
static void AsmCodeBuildOr(TreeNode* node, NameTableType* localTable, 
                            const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                            size_t numberOfTabs);
static void AsmCodeBuildFuncCall(TreeNode* node, NameTableType* localTable, 
                                 const NameTableType* allNamesTable, FILE* outStream,
                                 size_t numberOfTabs);
static void AsmCodeBuildEq(TreeNode* node, NameTableType* localTable, 
                            const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                            size_t numberOfTabs);
static void AsmCodeBuildNotEq(TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                              size_t numberOfTabs);
static void AsmCodeBuildGreaterEq(TreeNode* node, NameTableType* localTable, 
                                  const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                                  size_t numberOfTabs);
static void AsmCodeBuildLessEq(TreeNode* node, NameTableType* localTable, 
                               const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                               size_t numberOfTabs);
static void AsmCodeBuildLess(TreeNode* node, NameTableType* localTable, 
                             const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                             size_t numberOfTabs);
static void AsmCodeBuildGreater(TreeNode* node, NameTableType* localTable, 
                                const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                                size_t numberOfTabs);

static void AsmCodeBuildPrint(TreeNode* node, NameTableType* localTable, 
                        const NameTableType* allNamesTable, FILE* outStream, size_t numberOfTabs);

static void PrintTabs(const size_t numberOfTabs, FILE* outStream);
static void FprintfLine(FILE* outStream, const size_t numberOfTabs, const char* format, ...);

#define PRINT(...) FprintfLine(outStream, numberOfTabs, __VA_ARGS__)
#define CODE_BUILD(NODE) AsmCodeBuild(NODE, localTable, allNamesTable, outStream, numberOfTabs)

#define PARAMS(NODE)                NODE, localTable, allNamesTable, outStream, numberOfTabs
#define PARAMS_WITH_RAM_ID(NODE)    NODE, localTable, allNamesTable, &varRamId, outStream, numberOfTabs
#define PARAMS_WITH_LABEL_ID(NODE)  NODE, localTable, allNamesTable, &labelId,  outStream, numberOfTabs

void AsmCodeBuild(Tree* tree, FILE* outStream, FILE* outBinStream)
{
    assert(tree);
    assert(outStream);
    assert(outBinStream);

    fprintf(outStream, "call main:\n"
                       "hlt\n\n");
    
    AsmCodeBuild(tree->root, nullptr, tree->allNamesTable, outStream, 0);

    fprintf(outStream, "    ret\n");
}

static void AsmCodeBuild(TreeNode* node, NameTableType* localTable, 
                         const NameTableType* allNamesTable, FILE* outStream, size_t numberOfTabs)
{
    static size_t varRamId = 0;
    static size_t labelId  = 0;

    if (node == nullptr)
        return;

    if (node->valueType == TreeNodeValueType::NUM)
    {
        PRINT("push %d\n", node->value.num);
        return;
    }

    if (node->valueType == TreeNodeValueType::NAME)
    {
        assert(!node->left && !node->right);

        Name* varName = nullptr;
        NameTableFind(localTable, allNamesTable->data[node->value.nameId].name, &varName);
        assert(varName);

        PRINT("push [%d]\n", varName->memShift);

        return;
    }

    if (node->valueType == TreeNodeValueType::STRING_LITERAL)
    {
        AsmCodePrintStringLiteral(PARAMS(node));

        return;
    }

    assert(node->valueType == TreeNodeValueType::OPERATION);

    switch (node->value.operation)
    {
        case TreeOperationId::ADD:
        {
            CODE_BUILD(node->left);
            CODE_BUILD(node->right);

            PRINT("add\n");
            break;
        }
        case TreeOperationId::SUB:
        {
            CODE_BUILD(node->left);
            CODE_BUILD(node->right);

            PRINT("sub\n");
            break;
        }
        case TreeOperationId::MUL:
        {
            CODE_BUILD(node->left);
            CODE_BUILD(node->right);

            PRINT("mul\n");
            break;
        }
        case TreeOperationId::DIV:
        {
            CODE_BUILD(node->left);
            CODE_BUILD(node->right);

            PRINT("div\n");
            break;
        }
        case TreeOperationId::SIN:
        {
            CODE_BUILD(node->left);

            PRINT("sin\n");
            break;
        }
        case TreeOperationId::COS:
        {
            CODE_BUILD(node->left);

            PRINT("cos\n");
            break;
        }
        case TreeOperationId::TAN:
        {
            CODE_BUILD(node->left);

            PRINT("tan\n");
            break;
        }
        case TreeOperationId::COT:
        {
            CODE_BUILD(node->left);

            PRINT("cot\n");
            break;
        }
        case TreeOperationId::SQRT:
        {
            CODE_BUILD(node->left);

            PRINT("sqrt\n");
            break;
        }

        case TreeOperationId::COMMA:
        {
            CODE_BUILD(node->right);
            CODE_BUILD(node->left);

            // Comma goes in the opposite way, because it is used here only for func call
            break;
        }
        case TreeOperationId::NEW_FUNC:
        {
            CODE_BUILD(node->left);

            fprintf(outStream, "\n");

            CODE_BUILD(node->right);

            break;
        }

        case TreeOperationId::TYPE:
        {
            CODE_BUILD(node->right);
            break;
        }

        case TreeOperationId::FUNC:
        {
            AsmCodeBuildFunc(node, allNamesTable, &varRamId, outStream, numberOfTabs);
            
            break;
        }

        case TreeOperationId::LINE_END:
        {
            CODE_BUILD(node->left);
            CODE_BUILD(node->right);

            break;
        }

        case TreeOperationId::IF:
        {
            AsmCodeBuildIf(PARAMS_WITH_LABEL_ID(node));
            break;
        }

        case TreeOperationId::WHILE:
        {
            AsmCodeBuildWhile(PARAMS_WITH_LABEL_ID(node));
            break;
        }

        case TreeOperationId::AND:
        {
            AsmCodeBuildAnd(PARAMS_WITH_LABEL_ID(node));
            break;
        }

        case TreeOperationId::OR:
        {
            AsmCodeBuildOr(PARAMS_WITH_LABEL_ID(node));
            break;
        }

        case TreeOperationId::ASSIGN:
        {
            AsmCodeBuildAssign(PARAMS_WITH_RAM_ID(node));
            break;
        }

        case TreeOperationId::FUNC_CALL:
        {
            AsmCodeBuildFuncCall(PARAMS(node));
            break;
        }

        case TreeOperationId::RETURN:
        {
            CODE_BUILD(node->left);

            PRINT("ret\n");
            break;
        }

        case TreeOperationId::EQ:
        {
            AsmCodeBuildEq(PARAMS_WITH_LABEL_ID(node));
            break;
        }
        case TreeOperationId::NOT_EQ:
        {
            AsmCodeBuildNotEq(PARAMS_WITH_LABEL_ID(node));
            break;
        }
        case TreeOperationId::LESS:
        {
            AsmCodeBuildLess(PARAMS_WITH_LABEL_ID(node));
            break;
        }
        case TreeOperationId::LESS_EQ:
        {
            AsmCodeBuildLessEq(PARAMS_WITH_LABEL_ID(node));
            break;
        }
        case TreeOperationId::GREATER:
        {
            AsmCodeBuildGreater(PARAMS_WITH_LABEL_ID(node));
            break;
        }
        case TreeOperationId::GREATER_EQ:
        {
            AsmCodeBuildGreaterEq(PARAMS_WITH_LABEL_ID(node));
            break;
        }

        case TreeOperationId::READ:
        {
            PRINT("in\n");
            break;
        }

        case TreeOperationId::PRINT:
        {
            AsmCodeBuildPrint(PARAMS(node));

            break;
        }

        default:
            assert(false);
            break;
    }
}

static void AsmCodeBuildFunc(TreeNode* node, const NameTableType* allNamesTable, 
                             size_t* varRamId, FILE* outStream, size_t numberOfTabs)
{
    assert(node->left->valueType == TreeNodeValueType::NAME);

    PRINT("%s: \n", allNamesTable->data[node->left->value.nameId].name);
    numberOfTabs += 1;

    NameTableType* localTable = nullptr;
    NameTableCtor(&localTable);

    allNamesTable->data[node->left->value.nameId].localNameTable = localTable;

    NameTablePushFuncParams(node->left->left, localTable, allNamesTable, varRamId);

    for (size_t i = 0; i < localTable->size; ++i)
    {
        PRINT("pop [%d]\n", localTable->data[i].memShift);
    }

    CODE_BUILD(node->left->right);
}

static void NameTablePushFuncParams(TreeNode* node, NameTableType* local, 
                                    const NameTableType* allNamesTable, size_t* varRamId)
{
    assert(local);
    assert(allNamesTable);

    if (node == nullptr)
        return;

    if (node->valueType == TreeNodeValueType::NAME)
    {
        Name pushName = {};
        NameCtor(&pushName, allNamesTable->data[node->value.nameId].name, nullptr, *varRamId);
        // TODO: Name ctors strdups name. 
        // It's kind of useless when I am creating local table - 
        // I'm not going to delete local tables strings without deleting global table
        // Just using extra mem + time. 

        // TODO: mem leak never DTOR name table. + Create recursive name table dtor
        *varRamId += 1;

        NameTablePush(local, pushName);

        return;
    }

    assert(node->valueType == TreeNodeValueType::OPERATION);

    switch (node->value.operation)
    {
        case TreeOperationId::COMMA:
        {
            NameTablePushFuncParams(node->left, local,  allNamesTable, varRamId);
            NameTablePushFuncParams(node->right, local, allNamesTable, varRamId);

            break;
        }
        case TreeOperationId::TYPE:
        {
            NameTablePushFuncParams(node->right, local, allNamesTable, varRamId);
            
            break;
        }   

        default:
        {
            assert(false);
            break;
        }
    }
}

static void AsmCodeBuildIf(TreeNode* node, NameTableType* localTable, 
                           const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                           size_t numberOfTabs)
{
    fprintf(outStream, "\n");
    PRINT("@ if condition:\n");

    CODE_BUILD(node->left);

    size_t id = *labelId;
    PRINT("push 0\n");
    PRINT("je END_IF_%zu:\n", id);
    *labelId += 1;

    PRINT("@ if code block:\n");

    AsmCodeBuild(node->right, localTable, allNamesTable, outStream, numberOfTabs + 1);
    
    PRINT("END_IF_%zu:\n\n", id);
}

static void AsmCodeBuildWhile(TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                              size_t numberOfTabs)
{
    size_t id = *labelId;

    fprintf(outStream, "\n");

    PRINT("while_%zu:\n", id);

    *labelId += 1;
    PRINT("@ while condition: \n");

    CODE_BUILD(node->left);

    PRINT("push 0\n");
    PRINT("je END_WHILE_%zu:\n", id);

    *labelId += 1;

    PRINT("@ while code block: \n");

    AsmCodeBuild(node->right, localTable, allNamesTable, outStream, numberOfTabs + 1);

    PRINT("END_WHILE_%zu:\n\n", id);
}

static void AsmCodeBuildAssign(TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* varRamId, FILE* outStream,
                              size_t numberOfTabs)
{
    CODE_BUILD(node->right);

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

    PRINT("pop [%d]\n", varNameInTablePtr->memShift);
}

static void AsmCodeBuildAnd(TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                              size_t numberOfTabs)
{
    CODE_BUILD(node->left);
    CODE_BUILD(node->right);

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

static void AsmCodeBuildOr(TreeNode* node, NameTableType* localTable, 
                           const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                           size_t numberOfTabs)
{
    CODE_BUILD(node->left);

    size_t id = *labelId;

    PRINT("push 0\n");
    PRINT("je OR_FIRST_VAL_SET_ZERO_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("jmp OR_FIRST_VAL_END_%zu:\n", id);
    PRINT("OR_FIRST_VAL_SET_ZERO_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("OR_FIRST_VAL_END_%zu\n\n", id);

    *labelId += 1;

    CODE_BUILD(node->right);

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

static void AsmCodeBuildFuncCall(TreeNode* node, NameTableType* localTable, 
                                 const NameTableType* allNamesTable, FILE* outStream,
                                 size_t numberOfTabs)
{
    PRINT("@ pushing func local vars:\n");

    for (size_t i = 0; i < localTable->size; ++i)
    {
        PRINT("push [%d]\n", localTable->data[i].memShift);
    }

    PRINT("@ pushing func args:\n");
    CODE_BUILD(node->left->left);

    assert(node->left->valueType == TreeNodeValueType::NAME);

    PRINT(
                    "call %s:\n", allNamesTable->data[node->left->value.nameId].name);

    PRINT("@ saving func return\n");

    PRINT("pop rax\n");

    for (int i = (int)localTable->size - 1; i > -1; --i)
    {
        PRINT("pop [%d]\n", localTable->data[i].memShift);
    }

    PRINT("push rax\n");
}

static void AsmCodeBuildEq(TreeNode* node, NameTableType* localTable, 
                           const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                           size_t numberOfTabs)
{
    CODE_BUILD(node->left);
    CODE_BUILD(node->right);

    size_t id = *labelId;
    PRINT("je EQ_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_EQ_%zu:\n", id);
    PRINT("EQ_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_EQ_%zu:\n\n", id);

    *labelId += 1;
}

static void AsmCodeBuildNotEq(TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                              size_t numberOfTabs)
{
    CODE_BUILD(node->left);
    CODE_BUILD(node->right);

    size_t id = *labelId;
    PRINT("jne NOT_EQ_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_NOT_EQ_%zu:\n", id);
    PRINT("NOT_EQ_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_NOT_EQ_%zu:\n\n", id);
    
    *labelId += 1;
}

static void AsmCodeBuildLess(TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                              size_t numberOfTabs)
{
    CODE_BUILD(node->left);
    CODE_BUILD(node->right);

    size_t id = *labelId;
    PRINT("jb LESS_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_LESS_%zu:\n", id);
    PRINT("LESS_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_LESS_%zu:\n\n", id);
    
    *labelId += 1;
}

static void AsmCodeBuildLessEq(TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                              size_t numberOfTabs)
{
    CODE_BUILD(node->left);
    CODE_BUILD(node->right);

    size_t id = *labelId;
    PRINT("jbe LESS_EQ_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_LESS_EQ_%zu:\n", id);
    PRINT("LESS_EQ_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_LESS_EQ_%zu:\n\n", id);
    
    *labelId += 1;
}

static void AsmCodeBuildGreater(TreeNode* node, NameTableType* localTable, 
                              const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                              size_t numberOfTabs)
{
    CODE_BUILD(node->left);
    CODE_BUILD(node->right);

    size_t id = *labelId;
    PRINT("ja GREATER_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_GREATER_%zu:\n", id);
    PRINT("GREATER_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_GREATER_%zu:\n\n", id);

    *labelId += 1;
}

static void AsmCodeBuildGreaterEq(TreeNode* node, NameTableType* localTable, 
                                const NameTableType* allNamesTable, size_t* labelId, FILE* outStream,
                                size_t numberOfTabs)
{
    CODE_BUILD(node->left);
    CODE_BUILD(node->right);

    size_t id = *labelId;
    PRINT("jae GREATER_EQ_%zu:\n", id);
    PRINT("push 0\n");
    PRINT("jmp AFTER_GREATER_EQ_%zu:\n", id);
    PRINT("GREATER_EQ_%zu:\n", id);
    PRINT("push 1\n");
    PRINT("AFTER_GREATER_EQ_%zu:\n\n", id);

    *labelId += 1;
}

static void AsmCodePrintStringLiteral(TreeNode* node, NameTableType* localTable, 
                          const NameTableType* allNamesTable, FILE* outStream, size_t numberOfTabs)
{
    const char* string = allNamesTable->data[node->value.nameId].name;

    size_t pos = 0; //skipping " char

    while (string[pos])
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

static void AsmCodeBuildPrint(TreeNode* node, NameTableType* localTable, 
                        const NameTableType* allNamesTable, FILE* outStream, size_t numberOfTabs)
{
    assert(node->left);

    if (node->left->valueType == TreeNodeValueType::STRING_LITERAL)
    {
        AsmCodePrintStringLiteral(PARAMS(node->left));

        return;
    }

    CODE_BUILD(node->left);

    PRINT("out\n");
    PRINT("pop\n");
}

static void PrintTabs(const size_t numberOfTabs, FILE* outStream)
{
    for (size_t i = 0; i < numberOfTabs; ++i)
        fprintf(outStream, "    ");
}

static void FprintfLine(FILE* outStream, const size_t numberOfTabs, const char* format, ...)
{
    va_list args = {};

    va_start(args, format);

    PrintTabs(numberOfTabs, outStream);
    vfprintf(outStream, format, args);

    va_end(args);
}
