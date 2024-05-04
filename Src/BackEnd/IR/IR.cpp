#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "IR.h"
#include "Tree/NameTable/NameTable.h"
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

static void     IRBuild             (const TreeNode* node, CompilerInfoState* info);
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

IR* IRBuild(const Tree* tree, const NameTableType* allNamesTable)
{
    assert(tree);
    assert(allNamesTable);

    IR* ir = IRCtor();
    
    CompilerInfoState info = CompilerInfoStateCtor();
    info.allNamesTable = allNamesTable;
    info.ir = ir;
    
    IRBuild(tree->root, &info);

    IRPushBack(ir, IRNodeCreate(OP(HLT)));

    return ir;
}

static void IRBuild(const TreeNode* node, CompilerInfoState* info)
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
            IRBuild(node->left,  info);
            IRBuild(node->right, info);
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
        IRBuild(node->left, info);                                                              \
        if (CHILDREN_CNT == 2)                                                                  \
        {                                                                                       \
            IRBuild(node->right, info);                                                         \
            IR_PUSH(IRNodeCreate(OP(F_POP), operand2));                                         \
        }                                                                                       \
        else operand2 = EMPTY_OPERAND;                                                          \
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

    IR_PUSH(IRNodeCreate(NameTableGetName(info->allNamesTable, funcNameNode->value.nameId)));

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

    IRBuild(funcNameNode->right, info);
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

    IR_PUSH(IRNodeCreate(OP(CALL), IROperandStrCreate(funcName), true));

    // pushing ret value on stack
    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandRegCreate(IR_REG(XMM0))));
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

            IR_PUSH(IRNodeCreate(OP(F_MOV),
                                 IROperandRegCreate(IR_REG(XMM0)),
                                 IROperandMemCreate(name->memShift, name->reg)));

            IR_PUSH(IRNodeCreate(OP(F_PUSH), 
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

    IRBuild(node->left, info);
    

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    
    IR_PUSH(IRNodeCreate(OP(F_CMP), IROperandRegCreate(IR_REG(XMM0)), 
                                    IROperandImmCreate(0)));

    IR_PUSH(IRNodeCreate(OP(JE), IROperandStrCreate(ifEndLabel), true));

    IRBuild(node->right, info);

    IR_PUSH(IRNodeCreate(ifEndLabel));
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

    IR_PUSH(IRNodeCreate(whileBeginLabel));

    IRBuild(node->left, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    
    IR_PUSH(IRNodeCreate(OP(F_CMP), IROperandRegCreate(IR_REG(XMM0)), 
                                    IROperandImmCreate(0)));
                                               
    IR_PUSH(IRNodeCreate(OP(JE), IROperandStrCreate(whileEndLabel), true));

    IRBuild(node->right, info);

    IR_PUSH(IRNodeCreate(OP(JMP), IROperandStrCreate(whileBeginLabel), true));

    IR_PUSH(IRNodeCreate(whileEndLabel));
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

    IRBuild(node->right, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    IR_PUSH(IRNodeCreate(OP(F_MOV), IROperandMemCreate(varName->memShift, varName->reg),
                                    IROperandRegCreate(IR_REG(XMM0))));
}

static void BuildReturn(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->ir);

    IRBuild(node->left, info);
    
    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));

    IR_PUSH(IRNodeCreate(OP(MOV), IROperandRegCreate(IR_REG(RSP)), 
                                  IROperandRegCreate(IR_REG(RBP))));

    IR_PUSH(IRNodeCreate(OP(POP), IROperandRegCreate(IR_REG(RBP))));

    IR_PUSH(IRNodeCreate(OP(RET), IROperandImmCreate((long long)info->numberOfFuncParams)));    
}

static void BuildComparison(const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info->allNamesTable);

    IRBuild(node->left, info);
    IRBuild(node->right, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM1))));

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

    // TODO: think about code gen 

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

    IR_PUSH(IRNodeCreate(jumpOp, IROperandStrCreate(comparePushTrue), true));
    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandImmCreate(0)));
    IR_PUSH(IRNodeCreate(OP(JMP), IROperandStrCreate(compareEnd), true));
    IR_PUSH(IRNodeCreate(comparePushTrue));
    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandImmCreate(1)));
    IR_PUSH(IRNodeCreate(compareEnd));
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

static void     BuildRead           (const TreeNode* node, CompilerInfoState* info)
{
    assert(node);
    assert(info);
    assert(info->ir);

    IR_PUSH(IRNodeCreate(OP(F_IN)));
}

static void     BuildPrint          (const TreeNode* node, CompilerInfoState* info)
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

    IRBuild(node->left, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    IR_PUSH(IRNodeCreate(OP(F_OUT), IROperandRegCreate(IR_REG(XMM0))));
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

/*
static inline void DotFileBegin(FILE* outDotFile)
{
    fprintf(outDotFile, "digraph G{\nrankdir=TB;\ngraph [bgcolor=\"#31353b\"];\n"
                        "edge[color=\"#00D0D0\"];\n");
}

static inline void DotFileEnd(FILE* outDotFile)
{
    fprintf(outDotFile, "\n}\n");
}

void IRGraphicDump(const IR* ir, bool openImg, const NameTableType* nameTable)
{
    assert(ir);
    assert(nameTable);

    static const char* dotFileName = "IR.dot";
    FILE* outDotFile = fopen(dotFileName, "w");

    if (outDotFile == nullptr)
        return;

    DotFileBegin(outDotFile);

    assert(ir->end);
    IRNode* irBegin = ir->end->nextNode;

    DotFileCreateNodes(irBegin, outDotFile, nameTable);

    IRGraphicDump(irBegin, outDotFile);

    DotFileEnd(outDotFile);

    fclose(outDotFile);

    static size_t imgIndex = 0;
    CreateImgInLogFile(imgIndex, openImg);
    imgIndex++;
}

static void DotFileCreateNodes(const IRNode* node, FILE* outDotFile,
                               const NameTableType* nameTable)
{
    assert(outDotFile);

    if (node == nullptr)
        return;
    

    fprintf(outDotFile, "node%p[shape=Mrecord, style=filled, fillcolor=\"89AC76\","
                        "label =\"", node);
    if (node->labelName)
        fprintf(outDotFile, "labelName - %s |", node->labelName);

    if (node->numberOfOperands > 0)
        fprintf(outDotFile, "")
                  

    fprintf(outDotFile, ""
    if (node. == TreeNodeValueType::OPERATION)
    {
        //printf("OP: %s\n", TreeOperationGetLongName(node->value.operation));
        fprintf(outDotFile, "fillcolor=\"#89AC76\", label = \"%s\", ", 
                            TreeOperationGetLongName(node->value.operation));
    }
    else if (node->valueType == TreeNodeValueType::NUM)
    {
        //printf("VAL: %d\n", node->value.value);
        fprintf(outDotFile, "fillcolor=\"#7293ba\", label = \"%d\", ", node->value.num);
    }
    else if (node->valueType == TreeNodeValueType::NAME)
    {
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = \"%s\", ",
                                nameTable->data[node->value.nameId].name);
    }
    else if (node->valueType == TreeNodeValueType::STRING_LITERAL)
    {
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = %s, ",
                                nameTable->data[node->value.nameId].name);   
    }
    else 
        fprintf(outDotFile, "fillcolor=\"#FF0000\", label = \"ERROR\", ");

    fprintf(outDotFile, "color = \"#D0D000\"];\n");
    
    const NameTableType* localNameTable = nameTable;

    if (node->valueType == TreeNodeValueType::NAME && 
        nameTable->data[node->value.nameId].localNameTable)
    {
        localNameTable = (NameTableType*)nameTable->data[node->value.nameId].localNameTable;
    }

    DotFileCreateNodes(node->left,  outDotFile, localNameTable);
    DotFileCreateNodes(node->right, outDotFile, localNameTable);
}

static void IRGraphicDump(const IRNode* node, FILE* outDotFile)
{
    if (node == nullptr)
    {
        fprintf(outDotFile, "\n");
        return;
    }
    
    fprintf(outDotFile, "node%p;\n", node);

    if (node->left != nullptr) fprintf(outDotFile, "node%p->", node);
    TreeGraphicDump(node->left, outDotFile);

    if (node->right != nullptr) fprintf(outDotFile, "node%p->", node);
    TreeGraphicDump(node->right, outDotFile);
}
*/

//---------------------------------------------------------------------------------------

void IRTextDump(const IR* ir, const NameTableType* allNamesTable, 
                const char* fileName, const char* funcName, const int line)
{
    assert(ir);
    assert(fileName);
    assert(funcName);
    assert(allNamesTable);

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
            Log("STR \n");
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
        #include "IROperations.h"

        default:    // Unreachable
            assert(false);
            return nullptr;
    }

    // Unreachable
    assert(false);
    return nullptr;
}