#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IRBuild.h"
#include "Tree/NameTable/NameTable.h"
#include "Tree/Tree.h"
#include "LabelTable/LabelTable.h"
#include "Common/Log.h"

struct CompilerInfoState
{
    NameTableType* localTable;
    const NameTableType* allNamesTable;

    IR* ir;

    size_t labelId;

    int        memShift;
    IRRegister regShift;

    size_t numberOfFuncParams;

    LabelTableType* labelTable;
};

static inline CompilerInfoState CompilerInfoStateCtor();
static inline void              CompilerInfoStateDtor(CompilerInfoState* info);

static void     Build               (const TreeNode* node, CompilerInfoState* info);
static void     BuildArithmeticOp   (const TreeNode* node, CompilerInfoState* info);
static void     BuildComparison     (const TreeNode* node, CompilerInfoState* info);
static void     BuildWhile          (const TreeNode* node, CompilerInfoState* info);
static void     BuildIf             (const TreeNode* node, CompilerInfoState* info);
static void     BuildAssign         (const TreeNode* node, CompilerInfoState* info);
static void     BuildReturn         (const TreeNode* node, CompilerInfoState* info);
static void     BuildNum            (const TreeNode* node, CompilerInfoState* info);
static void     BuildVar            (const TreeNode* node, CompilerInfoState* info);
static void     BuildRead           (const TreeNode* node, CompilerInfoState* info);
static void     BuildPrint          (const TreeNode* node, CompilerInfoState* info);
static void     BuildFuncCall       (const TreeNode* node, CompilerInfoState* info);
static void     PushFuncCallArgs    (const TreeNode* node, CompilerInfoState* info);
static void     BuildFunc           (const TreeNode* node, CompilerInfoState* info);

static size_t   InitFuncParams      (const TreeNode* node, CompilerInfoState* info);
static int      InitFuncLocalVars   (const TreeNode* node, CompilerInfoState* info);

static inline void BuildFuncQuit    (CompilerInfoState* info);

static void PatchJumps(IR* ir, const LabelTableType* labelTable);

#define IR_REG(REG_NAME)   IRRegister::REG_NAME  
#define OP(OP_NAME)        IROperation::OP_NAME
#define IR_PUSH(NODE)      IRPushBack(info->ir, NODE)

#define LABEL_PUSH(NAME)                                        \
do                                                              \
{                                                               \
    LabelTableValue tmpLabelVal = {};                           \
    LabelTableValueCtor(&tmpLabelVal, NAME, info->ir->end);     \
    LabelTablePush(info->labelTable, tmpLabelVal);              \
} while (0)

#define IR_PUSH_LABEL(NAME)                                     \
do                                                              \
{                                                               \
    IRPushBack(info->ir, IRNodeCreate(NAME));                   \
    LABEL_PUSH(NAME);                                           \
} while (0)


IR* IRBuild(const Tree* tree, const NameTableType* allNamesTable)
{
    assert(tree);
    assert(allNamesTable);

    IR* ir = IRCtor();
    
    CompilerInfoState info = CompilerInfoStateCtor();
    info.allNamesTable = allNamesTable;
    info.ir = ir;

    info.labelTable = nullptr;
    
    LabelTableCtor(&info.labelTable);
        
    IRPushBack(ir, IRNodeCreate("_start"));
    IRPushBack(ir, IRNodeCreate(OP(CALL), IROperandLabelCreate("main"), true));
    IRPushBack(ir, IRNodeCreate(OP(HLT)));

    Build(tree->root, &info);

    PatchJumps(ir, info.labelTable);

    CompilerInfoStateDtor(&info);

    return ir;
}

static void Build(const TreeNode* node, CompilerInfoState* info)
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
            Build(node->left,  info);
            Build(node->right, info);
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

        case TreeOperationId::READ:
        {
            BuildRead(node, info);
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

#define GENERATE_OPERATION_CMD(OPERATION_ID, _1, CHILDREN_CNT, ARITHM_OP, ...)                  \
    case TreeOperationId::OPERATION_ID:                                                         \
    {                                                                                           \
        assert(CHILDREN_CNT > 0);                                                               \
        Build(node->left, info);                                                                \
        if (CHILDREN_CNT == 2)                                                                  \
        {                                                                                       \
            Build(node->right, info);                                                           \
            IR_PUSH(IRNodeCreate(OP(F_POP), operand2));                                         \
        }                                                                                       \
        else operand2 = IROperandCtor();                                                        \
                                                                                                \
        IR_PUSH(IRNodeCreate(OP(F_POP), operand1));                                             \
        IR_PUSH(IRNodeCreate(OP(ARITHM_OP), operand1, operand2));                               \
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

    IR_PUSH(IRNodeCreate(OP(F_PUSH), operand1));
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

    IR_PUSH_LABEL(NameTableGetName(info->allNamesTable, funcNameNode->value.nameId));

    IR_PUSH(IRNodeCreate(OP(PUSH), IROperandRegCreate(IR_REG(RBP))));
    IR_PUSH(IRNodeCreate(OP(MOV),  IROperandRegCreate(IR_REG(RBP)), 
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

    IR_PUSH(IRNodeCreate(OP(ADD), IROperandRegCreate(IR_REG(RSP)), 
                                  IROperandImmCreate((long long)rspShift)));

    Build(funcNameNode->right, info);

    BuildFuncQuit(info);
}

// Pascal decl
static size_t InitFuncParams(const TreeNode* node, CompilerInfoState* info)
{
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);

    if (node == nullptr)
        return 0;

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

            info->memShift += (int)XMM_REG_BYTE_SIZE;
        
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

    IR_PUSH(IRNodeCreate(OP(CALL), IROperandLabelCreate(funcName), true));

    // pushing ret value on stack
    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandRegCreate(IR_REG(XMM0))));
}

static void PushFuncCallArgs(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);

    if (node->valueType == TreeNodeValueType::OPERATION && 
        node->value.operation == TreeOperationId::COMMA)
    {
        PushFuncCallArgs(node->left, info);
        Build(node->right, info);
    }    
    else
        Build(node, info);
}

static void BuildIf(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);
    assert(info->ir);

    // TODO: можно отдельную функцию, где иду в condition и там, основываясь сразу на сравнении,
    // Делаю вывод о jump to if end / not jump (типо на стек не кладу, выгодно по времени)

    size_t id = info->labelId;
    info->labelId += 1;

    static const size_t maxLabelLen = 64;
    char ifEndLabel[maxLabelLen] = "";
    snprintf(ifEndLabel, maxLabelLen, "END_IF_%zu", id);

    Build(node->left, info);
    

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    
    IR_PUSH(IRNodeCreate(OP(F_XOR), IROperandRegCreate(IR_REG(XMM1)), 
                                    IROperandRegCreate(IR_REG(XMM1))));

    IR_PUSH(IRNodeCreate(OP(F_CMP), IROperandRegCreate(IR_REG(XMM0)), 
                                    IROperandRegCreate(IR_REG(XMM1))));

    IR_PUSH(IRNodeCreate(OP(JE), IROperandLabelCreate(ifEndLabel), true));

    Build(node->right, info);

    IR_PUSH_LABEL(ifEndLabel);
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

    IR_PUSH_LABEL(whileBeginLabel);

    Build(node->left, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    
    IR_PUSH(IRNodeCreate(OP(F_XOR), IROperandRegCreate(IR_REG(XMM1)),
                                    IROperandRegCreate(IR_REG(XMM1))));

    IR_PUSH(IRNodeCreate(OP(F_CMP), IROperandRegCreate(IR_REG(XMM0)), 
                                    IROperandRegCreate(IR_REG(XMM1))));
                                               
    IR_PUSH(IRNodeCreate(OP(JE), IROperandLabelCreate(whileEndLabel), true));

    Build(node->right, info);

    IR_PUSH(IRNodeCreate(OP(JMP), IROperandLabelCreate(whileBeginLabel), true));

    IR_PUSH_LABEL(whileEndLabel);
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

    Build(node->right, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    IR_PUSH(IRNodeCreate(OP(F_MOV), IROperandMemCreate(varName->memShift, varName->reg),
                                    IROperandRegCreate(IR_REG(XMM0))));
}

static void BuildReturn(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->ir);

    Build(node->left, info);
    
    BuildFuncQuit(info);
}

static inline void BuildFuncQuit(CompilerInfoState* info)
{
    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));

    IR_PUSH(IRNodeCreate(OP(MOV), IROperandRegCreate(IR_REG(RSP)), 
                                  IROperandRegCreate(IR_REG(RBP))));

    IR_PUSH(IRNodeCreate(OP(POP), IROperandRegCreate(IR_REG(RBP))));

    IR_PUSH(IRNodeCreate(OP(RET), IROperandImmCreate(
            (long long)info->numberOfFuncParams * XMM_REG_BYTE_SIZE)));    
}

static void BuildComparison(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info->allNamesTable);

    Build(node->left, info);
    Build(node->right, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM1))));
    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));

    IR_PUSH(IRNodeCreate(OP(F_CMP), IROperandRegCreate(IR_REG(XMM0)),
                                    IROperandRegCreate(IR_REG(XMM1))));

    static const size_t  maxLabelLen  = 64;
    char comparePushTrue[maxLabelLen] = "";
    char compareEnd     [maxLabelLen] = "";
    size_t id = info->labelId;
    info->labelId += 1;
    snprintf(comparePushTrue, maxLabelLen, "COMPARE_PUSH_1_%zu", id);
    snprintf(compareEnd,      maxLabelLen, "COMPARE_END_%zu",    id);

    IROperation jumpOp = IROperation::JMP;

#define GENERATE_OPERATION_CMD(OPERATION_ID, _1, _2, _3, IR_JMP_OP, ...)        \
    case TreeOperationId::OPERATION_ID:                                         \
        jumpOp = OP(IR_JMP_OP);                                                 \
        break;                                                                  \

    switch (node->value.operation)
    {
        #include "Tree/Operations.h"    // building cases

        default: // Unreachable
            assert(false);
            break;  
    }

#undef GENERATE_OPERATION_CMD

    IR_PUSH(IRNodeCreate(jumpOp, IROperandLabelCreate(comparePushTrue), true));

    IR_PUSH(IRNodeCreate(OP(F_XOR),  IROperandRegCreate(IR_REG(XMM0)),
                                     IROperandRegCreate(IR_REG(XMM0))));
    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandRegCreate(IR_REG(XMM0))));

    IR_PUSH(IRNodeCreate(OP(JMP), IROperandLabelCreate(compareEnd), true));

    IR_PUSH_LABEL(comparePushTrue);

    IR_PUSH(IRNodeCreate(OP(F_MOV), IROperandRegCreate(IR_REG(XMM0)), 
                                    IROperandImmCreate(1)));

    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandRegCreate(IR_REG(XMM0))));

    IR_PUSH_LABEL(compareEnd);
}

static void BuildNum(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);

    IR_PUSH(IRNodeCreate(OP(F_MOV), IROperandRegCreate(IR_REG(XMM0)),
                                    IROperandImmCreate(node->value.num)));

    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandRegCreate(IR_REG(XMM0))));
}

static void BuildVar(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);

    Name* name = nullptr;
    NameTableFind(info->localTable, 
                  NameTableGetName(info->allNamesTable, node->value.nameId), &name);
    assert(name);

    IR_PUSH(IRNodeCreate(OP(F_MOV), IROperandRegCreate(IR_REG(XMM0)),
                                    IROperandMemCreate(name->memShift, name->reg)));

    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandRegCreate(IR_REG(XMM0))));
}

static void BuildRead(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->ir);

    IR_PUSH(IRNodeCreate(OP(F_IN)));
    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandRegCreate(IR_REG(XMM0))));
}

static void BuildPrint(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->ir);

    if (node->left->valueType == TreeNodeValueType::STRING_LITERAL)
    {
        const char* name = NameTableGetName(info->allNamesTable, node->left->value.nameId);
        IR_PUSH(IRNodeCreate(OP(STR_OUT), IROperandStrCreate(name)));

        return;
    }

    Build(node->left, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    IR_PUSH(IRNodeCreate(OP(F_OUT), IROperandRegCreate(IR_REG(XMM0))));
}

//-----------------------------------------------------------------------------

static void PatchJumps(IR* ir, const LabelTableType* labelTable)
{
    assert(ir);
    assert(ir->end);

    IRNode* beginNode = ir->end->nextNode;
    IRNode* node      = beginNode;

    do
    {
        if (node->needPatch && !node->jumpTarget)
        {
            assert(node->operand1.type == IROperandType::LABEL);

            LabelTableValue* outLabel = nullptr;
            LabelTableFind(labelTable, node->operand1.value.string, &outLabel);

            assert(outLabel);
            assert(outLabel->connectedNode);
            assert(outLabel->connectedNode->nextNode);

            node->jumpTarget = outLabel->connectedNode->nextNode;
        }

        node = node->nextNode;
    } while (node != beginNode);
}

//-----------------------------------------------------------------------------

static inline CompilerInfoState CompilerInfoStateCtor()
{
    CompilerInfoState info  = {};
    info.allNamesTable      = nullptr;
    info.localTable         = nullptr;
    info.ir                 = nullptr;
    info.labelTable         = nullptr;

    info.labelId            = 0;
    info.memShift           = 0;
    info.numberOfFuncParams = 0;
    info.regShift           = IR_REG(NO_REG);
    
    return info;
}

static inline void              CompilerInfoStateDtor(CompilerInfoState* info)
{
    info->allNamesTable      = nullptr;
    info->localTable         = nullptr;
    info->ir                 = nullptr;

    LabelTableDtor(info->labelTable);
    info->labelTable         = nullptr;

    info->labelId            = 0;
    info->memShift           = 0;
    info->numberOfFuncParams = 0;
    info->regShift           = IR_REG(NO_REG);
}


static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg)
{
    static const size_t maxImgNameLength  = 64;
    static char imgName[maxImgNameLength] = "";
    snprintf(imgName, maxImgNameLength, "../imgs/img_%zu_time_%s.png", imgIndex, __TIME__);

    static const size_t     maxCommandLength  = 128;
    static char commandName[maxCommandLength] =  "";
    snprintf(commandName, maxCommandLength, "dot TreeHandler.dot -T png -o %s", imgName);
    system(commandName);

    snprintf(commandName, maxCommandLength, "<img src = \"%s\">\n", imgName);    
    Log(commandName);

    if (openImg)
    {
        snprintf(commandName, maxCommandLength, "open %s", imgName);
        system(commandName);
    }
}

//---------------------------------------------------------------------------------------

#undef IR_REG
#undef OP
#undef IR_PUSH
