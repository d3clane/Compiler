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
static inline IROperand IROperandCreate(IROperandValue val, IROperandType type);
static inline IRNode*   IRNodeCtor();

static inline IROperand IROperandRegCreate(IRRegister reg);
static inline IROperand IROperandImmCreate(const long long imm);
static inline IROperand IROperandStrCreate(const char* str);
static inline IROperand IROperandMemCreate(const long long imm, IRRegister reg);

static inline IR* IRCtor();

static void     BuildIR             (const TreeNode* node, CompilerInfoState* info);
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

#define TYPE(IR_TYPE)      IROperandType::IR_TYPE
#define IR_REG(REG_NAME)   IRRegister::REG_NAME  
#define OP(OP_NAME)        IROperation::OP_NAME
#define EMPTY_OPERAND      IROperandCtor()
#define IR_PUSH(NODE)      IRPushBack(info->ir, NODE)
#define CREATE_VALUE(...)  IROperandValueCreate(__VA_ARGS__)

IR* BuildIR(Tree* tree, const NameTableType* allNamesTable)
{
    assert(tree);
    assert(allNamesTable);

    IR* ir = IRCtor();
    
    CompilerInfoState info = CompilerInfoStateCtor();
    info.allNamesTable = allNamesTable;
    info.ir = ir;
    
    BuildIR(tree->root, &info);

    IRPushBack(ir, IRNodeCreate(OP(HLT), nullptr, 0, EMPTY_OPERAND, EMPTY_OPERAND));

    return ir;
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
    

    IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 2, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));
    
    IR_PUSH(IRNodeCreate(OP(F_CMP), nullptr, 2, IROperandRegCreate(IR_REG(XMM0)), 
                                                IROperandImmCreate(0)));

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

    IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 2, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));
    
    IR_PUSH(IRNodeCreate(OP(F_CMP), nullptr, 2, IROperandRegCreate(IR_REG(XMM0)), 
                                                IROperandImmCreate(0)));
                                               
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
    assert(node);
    assert(info);
    assert(info->ir);

    BuildIR(node->left, info);
    
    IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(MOV), nullptr, 2, IROperandRegCreate(IR_REG(RSP)), 
                                              IROperandRegCreate(IR_REG(RBP))));

    IR_PUSH(IRNodeCreate(OP(POP), nullptr, 1, IROperandRegCreate(IR_REG(RBP)), EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(RET), nullptr, 1, IROperandImmCreate((long long)info->numberOfFuncParams),
                                              EMPTY_OPERAND));    
}

static void BuildComparison(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info->allNamesTable);

    BuildIR(node->left, info);
    BuildIR(node->right, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 1, IROperandRegCreate(IR_REG(XMM1)), EMPTY_OPERAND));

    IR_PUSH(IRNodeCreate(OP(F_CMP), nullptr, 2, IROperandRegCreate(IR_REG(XMM0)),
                                                IROperandRegCreate(IR_REG(XMM1))));

    static const size_t  maxLabelLen  = 64;
    char comparePushTrue[maxLabelLen] = "";
    char compareEnd     [maxLabelLen] = "";
    size_t id = info->labelId;
    info->labelId += 1;
    snprintf(comparePushTrue, maxLabelLen, "COMPARE_PUSH_1_%zu", id);
    snprintf(compareEnd,      maxLabelLen, "COMPARE_END_%zu",    id);

    IROperation jumpOp = IROperation::JMP;

    // TODO: think about code gen 
    switch (node->value.operation)
    {
        case TreeOperationId::EQ:
            jumpOp = OP(JE);
            break;
        case TreeOperationId::NOT_EQ:
            jumpOp = OP(JNE);
            break;

        case TreeOperationId::LESS:
            jumpOp = OP(JL);
            break;
        case TreeOperationId::LESS_EQ:
            jumpOp = OP(JLE);
            break;

        case TreeOperationId::GREATER:
            jumpOp = OP(JG);
            break;
        case TreeOperationId::GREATER_EQ:
            jumpOp = OP(JGE);
            break;    

        default: // Unreachable
            assert(false);
            break;
    }

    IR_PUSH(IRNodeCreate(jumpOp,     nullptr, 1, IROperandStrCreate(comparePushTrue), EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(F_PUSH), nullptr, 1, IROperandImmCreate(0),               EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(JMP),    nullptr, 1, IROperandStrCreate(compareEnd),      EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(NOP), comparePushTrue, 0, EMPTY_OPERAND, EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(F_PUSH), nullptr, 1, IROperandImmCreate(1),               EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(NOP), compareEnd,      0, EMPTY_OPERAND, EMPTY_OPERAND));
}

static void BuildNum(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);

    IR_PUSH(IRNodeCreate(OP(F_MOV),  nullptr, 2, IROperandRegCreate(IR_REG(XMM0)),
                                                 IROperandImmCreate(node->value.num)));

    IR_PUSH(IRNodeCreate(OP(F_PUSH), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));
}

static void BuildVar(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);

    Name* name = nullptr;
    NameTableFind(info->localTable, 
                  NameTableGetName(info->allNamesTable, node->value.nameId), &name);
    assert(name);

    IR_PUSH(IRNodeCreate(OP(F_MOV), nullptr, 2, IROperandRegCreate(IR_REG(XMM0)),
                                                IROperandMemCreate(name->memShift, name->reg)));

    IR_PUSH(IRNodeCreate(OP(F_PUSH), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));
}

static void     BuildRead           (const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->ir);

    IR_PUSH(IRNodeCreate(OP(F_IN), nullptr, 0, EMPTY_OPERAND, EMPTY_OPERAND));
}

static void     BuildPrint          (const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->ir);

    if (node->left->valueType == TreeNodeValueType::STRING_LITERAL)
    {
        const char* name = NameTableGetName(info->allNamesTable, node->left->value.nameId);
        IR_PUSH(IRNodeCreate(OP(STR_OUT), nullptr, 1, IROperandStrCreate(name), EMPTY_OPERAND));

        return;
    }

    BuildIR(node->left, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));
    IR_PUSH(IRNodeCreate(OP(F_OUT), nullptr, 1, IROperandRegCreate(IR_REG(XMM0)), EMPTY_OPERAND));
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
    node->jumpTarget = nullptr;
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

void IRPushBack  (IR* ir, IRNode* node)
{
    assert(ir);
    assert(node);

    node->nextNode    = ir->end->nextNode;
    node->prevNode    = ir->end;
    ir->end->nextNode = node;
    
    ir->end = node;
}

IRNode* IRNodeCreate(IROperation operation, const char* labelName, 
                     size_t numberOfOperands, IROperand operand1, IROperand operand2)
{
    IRNode* node = IRNodeCtor();

    node->operation = operation;
    
    node->labelName = labelName ? strdup(labelName) : nullptr;

    node->numberOfOperands = numberOfOperands;
    
    node->operand1 = operand1;
    node->operand2 = operand2;

    return node;
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
    return IROperandCreate(CREATE_VALUE(0, IR_REG(NO_REG), str), TYPE(STR));
}

static inline IROperand IROperandMemCreate(const long long imm, IRRegister reg)
{
    return IROperandCreate(CREATE_VALUE(imm, reg), TYPE(MEM));
}

static inline CompilerInfoState CompilerInfoStateCtor()
{
    CompilerInfoState state  = {};
    state.allNamesTable      = nullptr;
    state.localTable         = nullptr;
    state.ir                 = nullptr;

    state.labelId            = 0;
    state.memShift           = 0;
    state.numberOfFuncParams = 0;
    state.regShift           = IR_REG(NO_REG);
    
    return state;
}

static inline void              CompilerInfoStateDtor(CompilerInfoState* info)
{
    info->allNamesTable      = nullptr;
    info->localTable         = nullptr;
    info->ir                 = nullptr;

    info->labelId            = 0;
    info->memShift           = 0;
    info->numberOfFuncParams = 0;
    info->regShift           = IR_REG(NO_REG);
}