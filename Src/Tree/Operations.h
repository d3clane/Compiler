#ifndef GENERATE_OPERATION_CMD
#define GENERATE_OPERATION_CMD(...)
#endif

// GENERATE_OPERATION_CMD(NAME, CALC_FUNC, NUMBER_OF_SONS_IN_AST, ...)

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
2)

GENERATE_OPERATION_CMD(SUB,
{
    CALC_CHECK();

    return val1 - val2;
},
2)

GENERATE_OPERATION_CMD(UNARY_SUB,
{
    assert(isfinite(val1));

    return -val1;
},
1)

GENERATE_OPERATION_CMD(MUL,
{
    CALC_CHECK();

    return val1 * val2;
},
2)

GENERATE_OPERATION_CMD(DIV,
{
    CALC_CHECK();
    assert(!DoubleEqual(val2, 0));

    return val1 / val2;
},
2)

GENERATE_OPERATION_CMD(POW,
{
    CALC_CHECK();

    return pow(val1, val2);
},
2)

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
1)

GENERATE_OPERATION_CMD(SIN,
{
    CALC_CHECK();

    return sin(val1);
},
1)

GENERATE_OPERATION_CMD(COS,
{
    CALC_CHECK();

    return cos(val1);
},
1)

GENERATE_OPERATION_CMD(TAN,
{
    CALC_CHECK();

    return tan(val1);
},
1)

GENERATE_OPERATION_CMD(COT,
{
    CALC_CHECK();

    double tan_val1 = tan(val1);

    assert(!DoubleEqual(tan_val1, 0));
    assert(isfinite(tan_val1));

    return 1 / tan_val1;
},
1)

GENERATE_OPERATION_CMD(ASSIGN,
{
    assert(false);

    return -1;
},
2)

GENERATE_OPERATION_CMD(LINE_END, 
{
    assert(false);

    return -1; //TODO: 
},
2)

GENERATE_OPERATION_CMD(IF, 
{
    assert(false);

    return -1;
},
2)

GENERATE_OPERATION_CMD(WHILE,
{
    assert(false);

    return -1;
},
2)

GENERATE_OPERATION_CMD(LESS, 
{

},
2)

GENERATE_OPERATION_CMD(GREATER, 
{

},
2)

GENERATE_OPERATION_CMD(LESS_EQ, 
{

},
2)

GENERATE_OPERATION_CMD(GREATER_EQ,
{

},
2)

GENERATE_OPERATION_CMD(EQ, 
{

},
2)

GENERATE_OPERATION_CMD(NOT_EQ,
{

},
2)

GENERATE_OPERATION_CMD(AND,
{

},
2)

GENERATE_OPERATION_CMD(OR,
{

},
2)

GENERATE_OPERATION_CMD(PRINT,
{

},
1)

GENERATE_OPERATION_CMD(READ,
{

},
0)

GENERATE_OPERATION_CMD(COMMA,
{

},
2)

GENERATE_OPERATION_CMD(TYPE_INT,
{

},
0)

GENERATE_OPERATION_CMD(TYPE,
{

},
2)

GENERATE_OPERATION_CMD(NEW_FUNC,
{

},
2)

GENERATE_OPERATION_CMD(FUNC,
{

},
1)

GENERATE_OPERATION_CMD(FUNC_CALL,
{

},
1)

GENERATE_OPERATION_CMD(RETURN,
{

},
1)

#undef CALC_CHECK
