#ifndef GENERATE_OPERATION_CMD
#define GENERATE_OPERATION_CMD(...)
#endif

// GENERATE_OPERATION_CMD(NAME, CALC_FUNC, ...)

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
})

GENERATE_OPERATION_CMD(SUB,
{
    CALC_CHECK();

    return val1 - val2;
})

GENERATE_OPERATION_CMD(UNARY_SUB,
{
    assert(isfinite(val1));

    return -val1;
})

GENERATE_OPERATION_CMD(MUL,
{
    CALC_CHECK();

    return val1 * val2;
})

GENERATE_OPERATION_CMD(DIV,
{
    CALC_CHECK();
    assert(!DoubleEqual(val2, 0));

    return val1 / val2;
})

GENERATE_OPERATION_CMD(POW,
{
    CALC_CHECK();

    return pow(val1, val2);
})

GENERATE_OPERATION_CMD(SQRT,
{
    CALC_CHECK();

    assert(val1 > 0); //TODO: надо бы сравнение даблов сделать

    return sqrt(val1);
})

#undef  CALC_CHECK
#define CALC_CHECK()        \
do                          \
{                           \
    assert(isfinite(val1)); \
} while (0)

GENERATE_OPERATION_CMD(SIN,
{
    CALC_CHECK();

    return sin(val1);
})

GENERATE_OPERATION_CMD(COS,
{
    CALC_CHECK();

    return cos(val1);
})

GENERATE_OPERATION_CMD(TAN,
{
    CALC_CHECK();

    return tan(val1);
})

GENERATE_OPERATION_CMD(COT,
{
    CALC_CHECK();

    double tan_val1 = tan(val1);

    assert(!DoubleEqual(tan_val1, 0));
    assert(isfinite(tan_val1));

    return 1 / tan_val1;
})

GENERATE_OPERATION_CMD(ASSIGN,
{
    assert(false);

    return -1;
})

GENERATE_OPERATION_CMD(LINE_END, 
{
    assert(false);

    return -1; //TODO: 
})

GENERATE_OPERATION_CMD(IF, 
{
    assert(false);

    return -1;
})

GENERATE_OPERATION_CMD(WHILE,
{
    assert(false);

    return -1;
})

GENERATE_OPERATION_CMD(LESS, 
{

})

GENERATE_OPERATION_CMD(GREATER, 
{

})

GENERATE_OPERATION_CMD(LESS_EQ, 
{

})

GENERATE_OPERATION_CMD(GREATER_EQ,
{

})

GENERATE_OPERATION_CMD(EQ, 
{

})

GENERATE_OPERATION_CMD(NOT_EQ,
{

})

GENERATE_OPERATION_CMD(AND,
{

})

GENERATE_OPERATION_CMD(OR,
{

})

//TODO: PRINT -> '{'
GENERATE_OPERATION_CMD(PRINT,
{

})

//TODO: READ -> '{'
GENERATE_OPERATION_CMD(READ,
{

})

GENERATE_OPERATION_CMD(COMMA,
{

})

GENERATE_OPERATION_CMD(TYPE_INT,
{

})

GENERATE_OPERATION_CMD(TYPE,
{

})

GENERATE_OPERATION_CMD(NEW_FUNC,
{

})

GENERATE_OPERATION_CMD(FUNC,
{

})

GENERATE_OPERATION_CMD(FUNC_CALL,
{

})

GENERATE_OPERATION_CMD(RETURN,
{

})

#undef CALC_CHECK
