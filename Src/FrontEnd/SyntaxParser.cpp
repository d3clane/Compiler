#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "LexicalParser.h"
#include "Tree/DSL.h"
#include "Tree/Tree.h"
#include "Tree/NameTable/NameTable.h"
#include "SyntaxParser.h"
#include "Common/StringFuncs.h"
#include "TokensArr/TokensArr.h"
#include "Common/Colors.h"
#include "Common/Log.h"
#include "LexicalParserTokenType.h"

struct DescentState
{
    TokensArr tokens;

    size_t tokenPos;

    NameTableType* globalTable; 
    NameTableType* currentLocalTable;

    NameTableType* allNamesTable;

    const char* codeString;
};

static void DescentStateCtor(DescentState* state, const char* codeString);
static void DescentStateDtor(DescentState* state);

#define POS(state) state->tokenPos

// G                ::= FUNC+ '\0'
// FUNC             ::= FUNC_DEF
// FUNC_DEF         ::= TYPE VAR FUNC_VARS_DEF '57' OP '{'
// FUNC_VAR_DEF     ::= {TYPE VAR}*
// OP               ::= {IF | WHILE | '57' OP+ '{' | { {VAR_DEF | PRINT | ASSIGN | RET} '57' }
// IF               ::= '57?' OR '57' OP
// WHILE            ::= '57!' OR '57' OP
// RET              ::= OR
// VAR_DEF          ::= TYPE VAR '==' OR
// PRINT            ::= '{' { ARG | CONST_STRING }
// READ             ::= '{'
// ASSIGN           ::= VAR '==' OR
// OR               ::= AND {and AND}*
// AND              ::= CMP {or CMP}*
// CMP              ::= ADD_SUB {[<, <=, >, >=, =, !=] ADD_SUB}*
// ADD_SUB          ::= MUL_DIV {[+, -] MUL_DIV}*
// MUL_DIV          ::= POW {[*, /] POW}*
// POW              ::= FUNC_CALL {['^'] FUNC_CALL}*
// FUNC_CALL        ::= IN_BUILD_FUNCS | CREATED_FUNCS | EXPR
// IN_BUILT_FUNCS   ::= [sin/cos/tan/cot/sqrt] '(' OR ')' | READ
// MADE_FUNC_CALL   ::= VAR '{' FUNC_VARS_CALL '57' 
// FUNC_VARS_CALL   ::= {OR}*
// EXPR             ::= '(' OR ')' | ARG
// ARG              ::= NUM | GET_VAR
// NUM              ::= ['0'-'9']+
// VAR              ::= ['a'-'z' 'A'-'Z' '_']+ ['a'-'z' 'A'-'Z' '_' '0'-'9']*
// CONST_STRING     ::= '"' [ANY_ASCII_CHAR]+ '"'

static TreeNode* GetVar              (DescentState* state, bool* outErr);
static TreeNode* CreateVar           (DescentState* state, bool* outErr);
static TreeNode* GetGrammar          (DescentState* state, bool* outErr);
static TreeNode* GetAddSub           (DescentState* state, bool* outErr);
static TreeNode* GetType             (DescentState* state, bool* outErr);
static TreeNode* GetFuncDef          (DescentState* state, bool* outErr);
static TreeNode* GetFunc             (DescentState* state, bool* outErr);
static TreeNode* GetFuncVarsDef      (DescentState* state, bool* outErr);
static TreeNode* GetPrint            (DescentState* state, bool* outErr);
static TreeNode* GetRead             (DescentState* state, bool* outErr);
static TreeNode* GetVarDef           (DescentState* state, bool* outErr);
static TreeNode* GetFuncCall         (DescentState* state, bool* outErr);
static TreeNode* GetFuncVarsCall     (DescentState* state, bool* outErr);
static TreeNode* GetWhile            (DescentState* state, bool* outErr);
static TreeNode* GetIf               (DescentState* state, bool* outErr);
static TreeNode* GetOp               (DescentState* state, bool* outErr);
static TreeNode* GetAssign           (DescentState* state, bool* outErr);
static TreeNode* GetAnd              (DescentState* state, bool* outErr);
static TreeNode* GetOr               (DescentState* state, bool* outErr);
static TreeNode* GetCmp              (DescentState* state, bool* outErr);
static TreeNode* GetMulDiv           (DescentState* state, bool* outErr);
static TreeNode* GetPow              (DescentState* state, bool* outErr);
static TreeNode* GetMadeFuncCall     (DescentState* state, bool* outErr);
static TreeNode* GetBuiltInFuncCall  (DescentState* state, bool* outErr);
static TreeNode* GetExpr             (DescentState* state, bool* outErr);
static TreeNode* GetArg              (DescentState* state, bool* outErr);
static TreeNode* GetNum              (DescentState* state, bool* outErr);
static TreeNode* GetReturn           (DescentState* state, bool* outErr);
static TreeNode* GetLiteral      (DescentState* state, bool* outErr);

static inline Token* GetLastToken(DescentState* state)
{
    assert(state);

    return &state->tokens.data[POS(state)];
}

#define SynAssert(state, statement, outErr)                 \
do                                                          \
{                                                           \
    assert(outErr);                                         \
    SYN_ASSERT(state, statement, outErr);                   \
    LOG_BEGIN();                                            \
    Log("func - %s, line - %d\n", __func__, __LINE__);      \
    LOG_END();                                              \
} while (0)

#define IF_ERR_RET(outErr, node1, node2)                \
do                                                      \
{                                                       \
    if (*outErr)                                        \
    {                                                   \
        if (node1) TreeNodeDeepDtor(node1);             \
        if (node2) TreeNodeDeepDtor(node2);             \
        return nullptr;                                 \
    }                                                   \
} while (0)

static inline void SYN_ASSERT(DescentState* state, bool statement, bool* outErr)
{
    assert(state);
    assert(outErr);

    if (statement)
        return;

    printf(RED_TEXT("Syntax error in line %zu, string - %s\n"), 
           GetLastToken(state)->line, state->codeString + GetLastToken(state)->pos);
    *outErr = true;
}

static inline bool PickToken(DescentState* state, LangOpId langOpId)
{
    assert(state);

    if (state->tokens.data[POS(state)].valueType      == TokenValueType::LANG_OP &&
        state->tokens.data[POS(state)].value.langOpId == langOpId)
        return true;
    
    return false;
}

static inline bool PickNum(DescentState* state)
{
    assert(state);

    if (state->tokens.data[POS(state)].valueType == TokenValueType::NUM)
        return true;

    return false;
}

static inline bool PickName(DescentState* state)
{
    assert(state);

    if (state->tokens.data[POS(state)].valueType == TokenValueType::NAME)
        return true;

    return false;
}

static inline bool PickLiteral(DescentState* state)
{
    assert(state);

    if (state->tokens.data[POS(state)].valueType == TokenValueType::STRING_LITERAL)
        return true;
    
    return false;
}

static inline bool ConsumeToken(DescentState* state, LangOpId langOpId, bool* outErr)
{
    assert(state);

    if (PickToken(state, langOpId))
    {
        POS(state)++;

        return true;
    }

    SynAssert(state, false, outErr);
    *outErr = true;
    return false;
}

static inline bool ConsumeNum(DescentState* state)
{
    assert(state);

    if (PickNum(state))
    {
        POS(state)++;

        return true;
    }

    return false;
}

static inline bool ConsumeName(DescentState* state)
{
    assert(state);

    if (PickName(state))
    {
        POS(state)++;

        return true;
    }

    return false;
}

static inline bool PickTokenOnPos(DescentState* state, const size_t pos, LangOpId langOpId)
{
    assert(state);

    if (state->tokens.data[pos].valueType     == TokenValueType::LANG_OP &&
        state->tokens.data[pos].value.langOpId == langOpId)
        return true;
    
    return false;
}

static inline LangOpId GetLastTokenId(DescentState* state)
{
    assert(state);
    assert(state->tokens.data[POS(state)].valueType == TokenValueType::LANG_OP);

    return state->tokens.data[POS(state)].value.langOpId;
}

Tree CodeParse(const char* code, SyntaxParserErrors* outErr)
{
    assert(code);

    Tree tree = {};
    TreeCtor(&tree);

    DescentState state = {};
    DescentStateCtor(&state, code);

    ParseOnTokens(code, &state.tokens);

    bool err = false;

    tree.root          = GetGrammar(&state, &err);
    tree.allNamesTable = state.allNamesTable;

    if (err)
        *outErr = SyntaxParserErrors::SYNTAX_ERR; 

    DescentStateDtor(&state);

    return tree;
}

static TreeNode* GetGrammar(DescentState* state, bool* outErr)
{
    assert(state);
    assert(outErr);

    TreeNode* root = GetFunc(state, outErr);
    IF_ERR_RET(outErr, root, nullptr);

    while (!PickToken(state, LangOpId::PROGRAM_END))
    {
        TreeNode* tmpNode = GetFunc(state, outErr);
        IF_ERR_RET(outErr, root, tmpNode);

        root = CREATE_NEW_FUNC_NODE(root, tmpNode);
    }

    SynAssert(state, PickToken(state, LangOpId::PROGRAM_END), outErr);
    IF_ERR_RET(outErr, root, nullptr);

    return root;
}

static TreeNode* GetFunc(DescentState* state, bool* outErr)
{
    TreeNode* node = GetFuncDef(state, outErr);
    IF_ERR_RET(outErr, node, nullptr);

    return node;
}

static TreeNode* GetFuncDef(DescentState* state, bool* outErr)
{
    NameTableType* localNameTable = nullptr;
    NameTableCtor(&localNameTable);

    TreeNode* func = nullptr;

    TreeNode* typeNode = GetType(state, outErr);
    IF_ERR_RET(outErr, typeNode, nullptr);

    TreeNode* funcName = CreateVar(state, outErr);
    IF_ERR_RET(outErr, typeNode, funcName);
    
    //TODO: create set table function
    assert(!state->globalTable->data[funcName->value.nameId].localNameTable);
    state->globalTable->data[funcName->value.nameId].localNameTable = (void*)localNameTable;
    state->currentLocalTable = localNameTable;
    func = CREATE_FUNC_NODE(funcName);

    TreeNode* funcVars = GetFuncVarsDef(state, outErr);
    funcName->left = funcVars;
    IF_ERR_RET(outErr, func, typeNode);

    SynAssert(state, PickToken(state, LangOpId::FIFTY_SEVEN), outErr);
    IF_ERR_RET(outErr, func, typeNode);
    
    TreeNode* funcCode = GetOp(state, outErr);
    funcName->right = funcCode;
    IF_ERR_RET(outErr, func, typeNode);

    func = CREATE_TYPE_NODE(typeNode, func);

    return func;
}

static TreeNode* GetType(DescentState* state, bool* outErr)
{
    ConsumeToken(state, LangOpId::TYPE_INT, outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);

    return TreeNodeCreate(TreeCreateOpVal(TreeOperationId::TYPE_INT), TreeNodeValueType::OPERATION);
}

static TreeNode* GetFuncVarsDef(DescentState* state, bool* outErr)
{
    TreeNode* varsDefNode = nullptr;
        
    if (!PickToken(state, LangOpId::TYPE_INT))
        return nullptr;
    
    TreeNode* varType = GetType(state, outErr);
    IF_ERR_RET(outErr, varType, nullptr);
    
    TreeNode* varName = CreateVar(state, outErr);
    IF_ERR_RET(outErr, varType, varName);

    varsDefNode = CREATE_TYPE_NODE(varType, varName);

    while (!PickToken(state, LangOpId::FIFTY_SEVEN))
    {
        varType = GetType(state, outErr);
        IF_ERR_RET(outErr, varsDefNode, varType);

        varName = CreateVar(state, outErr);
        TreeNode* tmpVar = CREATE_TYPE_NODE(varType, varName);
        IF_ERR_RET(outErr, tmpVar, varsDefNode);

        varsDefNode = CREATE_COMMA_NODE(varsDefNode, tmpVar);
    }

    return varsDefNode;
}

static TreeNode* GetOp(DescentState* state, bool* outErr)
{
    TreeNode* opNode = nullptr;

    if (PickToken(state, LangOpId::IF))
    {
        opNode = GetIf(state, outErr);
        IF_ERR_RET(outErr, opNode, nullptr);

        return opNode;
    }
    else if (PickToken(state, LangOpId::WHILE))
    {
        opNode = GetWhile(state, outErr);
        IF_ERR_RET(outErr, opNode, nullptr);

        return opNode;
    }
    else if (PickToken(state, LangOpId::PRINT))
        opNode = GetPrint(state, outErr);
    else if (PickToken(state, LangOpId::TYPE_INT))
        opNode = GetVarDef(state, outErr);
    else if (PickTokenOnPos(state, state->tokenPos + 1, LangOpId::ASSIGN))
        opNode = GetAssign(state, outErr);
    else if (PickToken(state, LangOpId::FIFTY_SEVEN))
    {
        ConsumeToken(state, LangOpId::FIFTY_SEVEN, outErr);
        IF_ERR_RET(outErr, opNode, nullptr);

        TreeNode* jointNode = CREATE_LINE_END_NODE(nullptr, nullptr);
        opNode = jointNode;
        while (true)
        {
            TreeNode* opInNode = GetOp(state, outErr);
            IF_ERR_RET(outErr, opNode, opInNode);

            jointNode->left = opInNode;

            if (PickToken(state, LangOpId::L_BRACE))
                break;

            jointNode->right = CREATE_LINE_END_NODE(nullptr, nullptr);
            jointNode        = jointNode->right;
        }

        ConsumeToken(state, LangOpId::L_BRACE, outErr);
        IF_ERR_RET(outErr, opNode, nullptr);

        return opNode;
    }
    else
        opNode = GetReturn(state, outErr);

    IF_ERR_RET(outErr, opNode, nullptr);

    ConsumeToken(state, LangOpId::FIFTY_SEVEN, outErr);
    IF_ERR_RET(outErr, opNode, nullptr);

    return opNode;
}

static TreeNode* GetReturn(DescentState* state, bool* outErr)
{
    return CREATE_RETURN_NODE(GetOr(state, outErr));
}

static TreeNode* GetIf(DescentState* state, bool* outErr)
{
    ConsumeToken(state, LangOpId::IF, outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);

    TreeNode* condition = GetOr(state, outErr);
    IF_ERR_RET(outErr, condition, nullptr);

    ConsumeToken(state, LangOpId::FIFTY_SEVEN, outErr);
    IF_ERR_RET(outErr, condition, nullptr);

    TreeNode* op = GetOp(state, outErr);
    IF_ERR_RET(outErr, condition, op);

    return CREATE_IF_NODE(condition, op);
}

static TreeNode* GetWhile(DescentState* state, bool* outErr)
{
    ConsumeToken(state, LangOpId::WHILE, outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);

    TreeNode* condition = GetOr(state, outErr);
    IF_ERR_RET(outErr, condition, nullptr);

    ConsumeToken(state, LangOpId::FIFTY_SEVEN, outErr);
    IF_ERR_RET(outErr, condition, nullptr);

    TreeNode* op = GetOp(state, outErr);
    IF_ERR_RET(outErr, condition, op);

    return CREATE_WHILE_NODE(condition, op);
}

static TreeNode* GetVarDef(DescentState* state, bool* outErr)
{
    TreeNode* typeNode = GetType(state, outErr);
    IF_ERR_RET(outErr, typeNode, nullptr);

    TreeNode* varName = CreateVar(state, outErr);
    IF_ERR_RET(outErr, typeNode, varName);

    ConsumeToken(state, LangOpId::ASSIGN, outErr);
    IF_ERR_RET(outErr, typeNode, varName);

    TreeNode* expr   = GetOr(state, outErr);
    TreeNode* assign = CREATE_ASSIGN_NODE(varName, expr);
    IF_ERR_RET(outErr, expr, typeNode);

    return CREATE_TYPE_NODE(typeNode, assign);
}

static TreeNode* GetExpr(DescentState* state, bool* outErr)
{
    if (!PickToken(state, LangOpId::L_BRACKET))
        return GetArg(state, outErr);

    ConsumeToken(state, LangOpId::L_BRACKET, outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);
    
    TreeNode* inBracketsExpr = GetOr(state, outErr);
    IF_ERR_RET(outErr, inBracketsExpr, nullptr);

    ConsumeToken(state, LangOpId::R_BRACKET, outErr); //Здесь изменение, раньше не было сдвига pos, предполагаю что баг
    IF_ERR_RET(outErr, inBracketsExpr, nullptr);

    return inBracketsExpr;
}

static TreeNode* GetPrint(DescentState* state, bool* outErr)
{
    ConsumeToken(state, LangOpId::PRINT, outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);

    TreeNode* arg = nullptr;
    if (PickLiteral(state))
        arg = GetLiteral(state, outErr);
    else
        arg = GetArg(state, outErr);

    IF_ERR_RET(outErr, arg, nullptr);

    return CREATE_PRINT_NODE(arg);
}

static TreeNode* GetRead(DescentState* state, bool* outErr)
{
    ConsumeToken(state, LangOpId::L_BRACE, outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);

    return CREATE_READ_NODE(nullptr, nullptr);
}

static TreeNode* GetAssign(DescentState* state, bool* outErr)
{
    TreeNode* var = GetVar(state, outErr);
    IF_ERR_RET(outErr, var, nullptr);

    ConsumeToken(state, LangOpId::ASSIGN, outErr);
    IF_ERR_RET(outErr, var, nullptr);

    TreeNode* rightExpr = GetOr(state, outErr);
    IF_ERR_RET(outErr, rightExpr, var);

    return CREATE_ASSIGN_NODE(var, rightExpr);
}

static TreeNode* GetMadeFuncCall(DescentState* state, bool* outErr)
{
    TreeNode* funcName = GetVar(state, outErr);
    IF_ERR_RET(outErr, funcName, nullptr);

    ConsumeToken(state, LangOpId::L_BRACE, outErr);
    IF_ERR_RET(outErr, funcName, nullptr);

    TreeNode* funcVars = GetFuncVarsCall(state, outErr);
    IF_ERR_RET(outErr, funcName, funcVars);
    funcName->left = funcVars;

    ConsumeToken(state, LangOpId::FIFTY_SEVEN, outErr);
    IF_ERR_RET(outErr, funcName, funcVars);

    return CREATE_FUNC_CALL_NODE(funcName);
}

static TreeNode* GetFuncVarsCall(DescentState* state, bool* outErr)
{
    if (PickToken(state, LangOpId::FIFTY_SEVEN))
        return nullptr;

    TreeNode* vars = GetOr(state, outErr);
    IF_ERR_RET(outErr, vars, nullptr);

    while (!PickToken(state, LangOpId::FIFTY_SEVEN))
    {
        TreeNode* tmpVar = GetOr(state, outErr);
        IF_ERR_RET(outErr, vars, tmpVar);

        vars = CREATE_COMMA_NODE(vars, tmpVar);
    }
    
    return vars;
}

static TreeNode* GetOr(DescentState* state, bool* outErr)
{
    TreeNode* allExpr = GetAnd(state, outErr);
    IF_ERR_RET(outErr, allExpr, nullptr);

    while (PickToken(state, LangOpId::OR))
    {
        ConsumeToken(state, LangOpId::OR, outErr);
        IF_ERR_RET(outErr, allExpr, nullptr);

        TreeNode* tmpExpr = GetAnd(state, outErr);
        IF_ERR_RET(outErr, tmpExpr, allExpr);

        allExpr = CREATE_OR_NODE(allExpr, tmpExpr);
    }

    return allExpr;
}

static TreeNode* GetAnd(DescentState* state, bool* outErr)
{
    TreeNode* allExpr = GetCmp(state, outErr);
    IF_ERR_RET(outErr, allExpr, nullptr);

    while (PickToken(state, LangOpId::AND))
    {
        ConsumeToken(state, LangOpId::AND, outErr);
        IF_ERR_RET(outErr, allExpr, nullptr);

        TreeNode* tmpExpr = GetCmp(state, outErr);
        IF_ERR_RET(outErr, tmpExpr, allExpr);

        allExpr = CREATE_AND_NODE(allExpr, tmpExpr);
    }

    return allExpr;
}

static TreeNode* GetCmp(DescentState* state, bool* outErr)
{
    TreeNode* allExpr = GetAddSub(state, outErr);
    IF_ERR_RET(outErr, allExpr, nullptr);

    while (PickToken(state, LangOpId::LESS)    || PickToken(state, LangOpId::LESS_EQ)     ||
           PickToken(state, LangOpId::GREATER) || PickToken(state, LangOpId::GREATER_EQ)  ||
           PickToken(state, LangOpId::EQ)      || PickToken(state, LangOpId::NOT_EQ))
    {
        LangOpId langOpId = GetLastTokenId(state);
        POS(state)++;

        TreeNode* newExpr = GetAddSub(state, outErr);
        IF_ERR_RET(outErr, allExpr, newExpr);

        switch (langOpId)
        {
            case LangOpId::LESS:
                allExpr = CREATE_LESS_NODE(allExpr, newExpr);
                break;

            case LangOpId::LESS_EQ:
                allExpr = CREATE_LESS_EQ_NODE(allExpr, newExpr);
                break;
            
            case LangOpId::GREATER:
                allExpr = CREATE_GREATER_NODE(allExpr, newExpr);
                break;

            case LangOpId::GREATER_EQ:
                allExpr = CREATE_GREATER_EQ_NODE(allExpr, newExpr);
                break;

            case LangOpId::EQ:
                allExpr = CREATE_EQ_NODE(allExpr, newExpr);
                break;

            case LangOpId::NOT_EQ:
                allExpr = CREATE_NOT_EQ_NODE(allExpr, newExpr);
                break;
            default:
                SynAssert(state, false, outErr);
                IF_ERR_RET(outErr, allExpr, newExpr);
                break;
        }
    }

    return allExpr;
}

static TreeNode* GetAddSub(DescentState* state, bool* outErr)
{
    TreeNode* allExpr = GetMulDiv(state, outErr);
    IF_ERR_RET(outErr, allExpr, nullptr);

    while (PickToken(state, LangOpId::ADD) || PickToken(state, LangOpId::SUB))
    {
        LangOpId langOpId = GetLastTokenId(state);
        POS(state)++;

        TreeNode* newExpr = GetMulDiv(state, outErr);
        IF_ERR_RET(outErr, allExpr, newExpr);

        switch (langOpId)
        {
            case LangOpId::ADD:
                allExpr = CREATE_ADD_NODE(allExpr, newExpr);
                break;
            
            case LangOpId::SUB:
                allExpr = CREATE_SUB_NODE(allExpr, newExpr);
                break;

            default:
                SynAssert(state, false, outErr);
                IF_ERR_RET(outErr, allExpr, newExpr);
                break;
        }
    }

    return allExpr;
}

static TreeNode* GetMulDiv(DescentState* state, bool* outErr)
{
    TreeNode* allExpr = GetPow(state, outErr);
    IF_ERR_RET(outErr, allExpr, nullptr);

    while (PickToken(state, LangOpId::MUL) || PickToken(state, LangOpId::DIV))
    {
        LangOpId langOpId = GetLastTokenId(state);
        POS(state)++;

        TreeNode* newExpr = GetPow(state, outErr);
        IF_ERR_RET(outErr, allExpr, newExpr);

        switch (langOpId)
        {
            case LangOpId::MUL:
                allExpr = CREATE_MUL_NODE(allExpr, newExpr);
                break;
            
            case LangOpId::DIV:
                allExpr = CREATE_DIV_NODE(allExpr, newExpr);
                break;

            default:
                SynAssert(state, false, outErr);
                IF_ERR_RET(outErr, allExpr, newExpr);
                break;
        }
    }

    return allExpr;
}

static TreeNode* GetPow(DescentState* state, bool* outErr)
{
    TreeNode* allExpr = GetFuncCall(state, outErr);
    IF_ERR_RET(outErr, allExpr, nullptr);

    while (PickToken(state, LangOpId::POW))
    {
        LangOpId langOpId = GetLastTokenId(state);
        POS(state)++;

        TreeNode* newExpr = GetFuncCall(state, outErr);
        IF_ERR_RET(outErr, allExpr, newExpr);
        
        assert(langOpId == LangOpId::POW);
        allExpr = CREATE_POW_NODE(allExpr, newExpr);
    }

    return allExpr;
}

static TreeNode* GetFuncCall(DescentState* state, bool* outErr)
{
    if (PickToken(state, LangOpId::SIN)  || PickToken(state, LangOpId::COS) ||
        PickToken(state, LangOpId::TAN)  || PickToken(state, LangOpId::COT) ||
        PickToken(state, LangOpId::SQRT) || PickToken(state, LangOpId::L_BRACE))
        return GetBuiltInFuncCall(state, outErr);

    if (PickTokenOnPos(state, state->tokenPos + 1, LangOpId::L_BRACE) && PickName(state))
        return GetMadeFuncCall(state, outErr);
    
    return GetExpr(state, outErr);
}

static TreeNode* GetBuiltInFuncCall(DescentState* state, bool* outErr)
{
    if (PickToken(state, LangOpId::L_BRACE))
        return GetRead(state, outErr);

    assert((PickToken(state, LangOpId::SIN) || PickToken(state, LangOpId::COS) ||
            PickToken(state, LangOpId::TAN) || PickToken(state, LangOpId::COT) ||
            PickToken(state, LangOpId::SQRT)));

    LangOpId langOpId = GetLastTokenId(state);
    POS(state)++;

    ConsumeToken(state, LangOpId::L_BRACKET, outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);

    TreeNode* expr = GetOr(state, outErr);
    IF_ERR_RET(outErr, expr, nullptr);

    ConsumeToken(state, LangOpId::R_BRACKET, outErr);
    IF_ERR_RET(outErr, expr, nullptr);

    switch (langOpId)
    {
        case LangOpId::SIN:
            expr = CREATE_SIN_NODE(expr);
            break;

        case LangOpId::COS:
            expr = CREATE_COS_NODE(expr);
            break;
        
        case LangOpId::TAN:
            expr = CREATE_TAN_NODE(expr);
            break;

        case LangOpId::COT:
            expr = CREATE_COT_NODE(expr);
            break;

        case LangOpId::SQRT:
            expr = CREATE_SQRT_NODE(expr);
            break;

        default:
            SynAssert(state, false, outErr);
            IF_ERR_RET(outErr, expr, nullptr);
            break;
    }

    return expr;
}

static TreeNode* GetArg(DescentState* state, bool* outErr)
{
    TreeNode* arg = nullptr;

    if (PickNum(state))
        arg = GetNum(state, outErr);
    else 
        arg = GetVar(state, outErr);

    IF_ERR_RET(outErr, arg, nullptr);

    return arg;
}

static TreeNode* GetNum(DescentState* state, bool* outErr)
{
    SynAssert(state, PickNum(state), outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);

    TreeNode* num = CREATE_NUM(state->tokens.data[POS(state)].value.num);
    POS(state)++;

    return num;
}

static TreeNode* CreateVar(DescentState* state, bool* outErr)
{
    SynAssert(state, PickName(state), outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);

    Name pushLocalName      = {};
    Name pushToAllNamesName = {};
    NameCtor(&pushLocalName,      state->tokens.data[POS(state)].value.name, nullptr, 0);
    NameCtor(&pushToAllNamesName, state->tokens.data[POS(state)].value.name, nullptr, 0);

    TreeNode* varNode = nullptr;

    NameTablePush(state->currentLocalTable, pushLocalName);
    NameTablePush(state->allNamesTable, pushToAllNamesName);
    varNode = CREATE_VAR(state->allNamesTable->size - 1);
    
    POS(state)++;

    return varNode;
}

static TreeNode* GetVar(DescentState* state, bool* outErr)
{
    SynAssert(state, state->tokens.data[POS(state)].value.name, outErr);
    IF_ERR_RET(outErr, nullptr, nullptr);

    assert(state->tokens.data);
    assert(state->tokens.data[POS(state)].value.name);

    Name pushName = {};
    NameCtor(&pushName, state->tokens.data[POS(state)].value.name, nullptr, 0);

    TreeNode* varNode = nullptr;

    Name* outName = nullptr;
    NameTableFind(state->allNamesTable, pushName.name, &outName);

    SynAssert(state, outName != nullptr, outErr);
    IF_ERR_RET(outErr, varNode, nullptr);
    
    //TODO: здесь пройтись по локали + глобали, проверить на существование переменную типо
    varNode = CREATE_VAR(outName - state->allNamesTable->data);
    
    POS(state)++;

    return varNode;
}

static TreeNode* GetLiteral(DescentState* state, bool* outErr)
{
    Name pushName = {};
    NameCtor(&pushName, state->tokens.data[POS(state)].value.name, nullptr, 0);

    //TODO: здесь проверки на то, что мы пушим (в плане того, чтобы не было конфликтов имен и т.д
    TreeNode* varNode = nullptr;

    NameTablePush(state->allNamesTable, pushName);

    //TODO: здесь пройтись по локали + глобали, проверить на существование переменную типо
    varNode = CREATE_STRING_LITERAL(state->allNamesTable->size - 1);

    POS(state)++;

    return varNode;
}

static void DescentStateCtor(DescentState* state, const char* str)
{
    TokensArrCtor(&state->tokens);
    NameTableCtor(&state->globalTable);
    NameTableCtor(&state->allNamesTable);

    state->currentLocalTable = state->globalTable;

    state->codeString       = str;
    state->tokenPos  = 0;
}

static void DescentStateDtor(DescentState* state)
{    
    NameTableDtor(state->globalTable);

    TokensArrDtor(&state->tokens);
    state->tokenPos = 0;
}
