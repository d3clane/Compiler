#ifndef GENERATE_OPERATION_CMD
#define GENERATE_OPERATION_CMD(...)
#endif

// GENERATE_OPERATION_CMD(NAME, CALC_FUNC, BUILD_IR_CODE, ...)

#define CALC_CHECK()            \
do                              \
{                               \
    assert(isfinite(val1));     \
    assert(isfinite(val2));     \
} while (0)

GENERATE_OPERATION_CMD(ADD,
{
    CALC_CHECK();

    return val1 + val2;
},
{
    BuildALUOp(OP(F_ADD), 2, node, info);
})

GENERATE_OPERATION_CMD(SUB,
{
    CALC_CHECK();

    return val1 - val2;
},
{
    BuildALUOp(OP(F_SUB), 2, node, info);
})

GENERATE_OPERATION_CMD(UNARY_SUB,
{
    assert(false);

    assert(isfinite(val1));

    return -val1;
},
{
    assert(false);
})

GENERATE_OPERATION_CMD(MUL,
{
    CALC_CHECK();

    return val1 * val2;
},
{
    BuildALUOp(OP(F_MUL), 2, node, info);
})

GENERATE_OPERATION_CMD(DIV,
{
    CALC_CHECK();
    assert(!DoubleEqual(val2, 0));

    return val1 / val2;
},
{
    BuildALUOp(OP(F_DIV), 2, node, info);
})

GENERATE_OPERATION_CMD(POW,
{
    CALC_CHECK();

    return pow(val1, val2);
},
{
    assert(false);
})

#undef  CALC_CHECK
#define CALC_CHECK()        \
do                          \
{                           \
    assert(isfinite(val1)); \
} while (0)

GENERATE_OPERATION_CMD(SQRT,
{
    CALC_CHECK();

    assert(val1 > 0); //TODO: надо бы сравнение даблов сделать

    return sqrt(val1);
},
{
    BuildALUOp(OP(F_SQRT), 1, node, info);
})

GENERATE_OPERATION_CMD(SIN,
{
    CALC_CHECK();

    return sin(val1);
},
{
    assert(false);
})

GENERATE_OPERATION_CMD(COS,
{
    CALC_CHECK();

    return cos(val1);
},
{
    assert(false);
})

GENERATE_OPERATION_CMD(TAN,
{
    CALC_CHECK();

    return tan(val1);
},
{
    assert(false);
})

GENERATE_OPERATION_CMD(COT,
{
    CALC_CHECK();

    double tan_val1 = tan(val1);

    assert(!DoubleEqual(tan_val1, 0));
    assert(isfinite(tan_val1));

    return 1 / tan_val1;
},
{
    assert(false);
})

GENERATE_OPERATION_CMD(ASSIGN,
{
    assert(false);

    return -1;
},
{
    assert(info->localTable);
    assert(info->allNamesTable);
    assert(info->ir);

    assert(node->left->valueType == TreeNodeValueType::NAME);
    
    Name* varName = nullptr;
    printf("TRYING TO FIND - %s, id - %d\n", NameTableGetName(info->allNamesTable, node->left->value.nameId), node->left->value.nameId);
    NameTableFind(info->localTable, 
                  NameTableGetName(info->allNamesTable, node->left->value.nameId), &varName);

    assert(varName);

    Build(node->right, info);

    IR_PUSH(IRNodeCreate(OP(F_POP), IROperandRegCreate(IR_REG(XMM0))));
    IR_PUSH(IRNodeCreate(OP(F_MOV), IROperandMemCreate(varName->memShift, varName->reg),
                                    IROperandRegCreate(IR_REG(XMM0))));
})

GENERATE_OPERATION_CMD(LINE_END, 
{
    assert(false);

    return -1;
},
{
    Build(node->left,  info);
    Build(node->right, info);
})

GENERATE_OPERATION_CMD(IF, 
{
    assert(false);

    return -1;
},
{
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
})

GENERATE_OPERATION_CMD(WHILE,
{
    assert(false);

    return -1;
},
{
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
})

GENERATE_OPERATION_CMD(LESS, 
{

},
{
    BuildComparison(OP(JB), node, info);
})

GENERATE_OPERATION_CMD(GREATER, 
{

},
{
    BuildComparison(OP(JA), node, info);
})

GENERATE_OPERATION_CMD(LESS_EQ, 
{

},
{
    BuildComparison(OP(JBE), node, info);
})

GENERATE_OPERATION_CMD(GREATER_EQ,
{

},
{
    BuildComparison(OP(JAE), node, info);
})

GENERATE_OPERATION_CMD(EQ, 
{

},
{
    BuildComparison(OP(JE), node, info);
})

GENERATE_OPERATION_CMD(NOT_EQ,
{

},
{
    BuildComparison(OP(JNE), node, info);
})

GENERATE_OPERATION_CMD(AND,
{
    
},
{
    BuildALUOp(OP(F_AND), 2, node, info);
})

GENERATE_OPERATION_CMD(OR,
{

},
{
    BuildALUOp(OP(F_OR), 2, node, info);
})

GENERATE_OPERATION_CMD(PRINT,
{

},
{
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
})

GENERATE_OPERATION_CMD(READ,
{

},
{
    assert(info->ir);

    IR_PUSH(IRNodeCreate(OP(F_IN)));
    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandRegCreate(IR_REG(XMM0))));
})

GENERATE_OPERATION_CMD(COMMA,
{

},
{
    assert(false); // Unreachable 
})

GENERATE_OPERATION_CMD(TYPE_INT,
{

},
{
    /* EMPTY */
})

GENERATE_OPERATION_CMD(TYPE,
{

},
{
    Build(node->left,  info);
    Build(node->right, info);
})

GENERATE_OPERATION_CMD(NEW_FUNC,
{

},
{
    Build(node->left,  info);
    Build(node->right, info);
})

GENERATE_OPERATION_CMD(FUNC,
{

},
{
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
})

GENERATE_OPERATION_CMD(FUNC_CALL,
{

},
{
    assert(node->left->valueType == TreeNodeValueType::NAME);

    // No registers saving because they are used only temporary
    PushFuncCallArgs(node->left->left, info);

    const char* funcName = NameTableGetName(info->allNamesTable, node->left->value.nameId);

    IR_PUSH(IRNodeCreate(OP(CALL), IROperandLabelCreate(funcName), true));

    // pushing ret value on stack
    IR_PUSH(IRNodeCreate(OP(F_PUSH), IROperandRegCreate(IR_REG(XMM0))));
})

GENERATE_OPERATION_CMD(RETURN,
{

},
{
    assert(info->ir);

    Build(node->left, info);
    
    BuildFuncQuit(info);
})

#undef CALC_CHECK
