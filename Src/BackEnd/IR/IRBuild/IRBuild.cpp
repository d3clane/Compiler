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
static void     BuildNum            (const TreeNode* node, CompilerInfoState* info);
static void     BuildVar            (const TreeNode* node, CompilerInfoState* info);
static void     PushFuncCallArgs    (const TreeNode* node, CompilerInfoState* info);

static void     BuildALUOp          (IROperation aluOp, size_t numberOfChildren,
                                     const TreeNode* node, CompilerInfoState* info);
static void     BuildComparison     (IROperation jccOp,
                                     const TreeNode* node, CompilerInfoState* info);

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

#define GENERATE_OPERATION_CMD(OP_NAME, _1, BUILD_IR_CODE, ...) \
    case TreeOperationId::OP_NAME:                              \
    {                                                           \
        BUILD_IR_CODE;                                          \
        break;                                                  \
    }   

    switch (node->value.operation)
    {
        #include "Tree/Operations.h" // cases on defines

        default:     // Unreachable
            assert(false);
            break;
    }
}

static void BuildALUOp(IROperation aluOp, size_t numberOfChildren, 
                              const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->localTable);
    assert(info->allNamesTable);
    assert(info->ir);

    IROperand operand1 = IROperandRegCreate(IR_REG(XMM0));

    assert(numberOfChildren > 0);
    Build(node->left, info);
    
    if (numberOfChildren == 2)
    {
        IROperand operand2 = IROperandRegCreate(IR_REG(XMM1));

        Build(node->right, info);
        IR_PUSH(IRNodeCreate(OP(F_POP), operand2));

        IR_PUSH(IRNodeCreate(OP(F_POP), operand1));
        IR_PUSH(IRNodeCreate(aluOp, operand1, operand2));
    }
    else 
    {
        IR_PUSH(IRNodeCreate(OP(F_POP), operand1));
        IR_PUSH(IRNodeCreate(aluOp, operand1));    
    }

    IR_PUSH(IRNodeCreate(OP(F_PUSH), operand1));   
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

static inline void BuildFuncQuit(CompilerInfoState* info)
{
    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));

    IR_PUSH(IRNodeCreate(OP(MOV), IROperandRegCreate(IR_REG(RSP)), 
                                  IROperandRegCreate(IR_REG(RBP))));

    IR_PUSH(IRNodeCreate(OP(POP), IROperandRegCreate(IR_REG(RBP))));

    IR_PUSH(IRNodeCreate(OP(RET), IROperandImmCreate(
            (long long)info->numberOfFuncParams * XMM_REG_BYTE_SIZE)));    
}

static void BuildComparison(IROperation jccOp, const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info->allNamesTable);

    Build(node->left,  info);
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

    IR_PUSH(IRNodeCreate(jccOp, IROperandLabelCreate(comparePushTrue), true));

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

//---------------------------------------------------------------------------------------

#undef IR_REG
#undef OP
#undef IR_PUSH
