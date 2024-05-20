#ifndef LEXICAL_PARSER_TOKEN_TYPE
#define LEXICAL_PARSER_TOKEN_TYPE

#include <stddef.h>

enum class LangOpId
{
    ADD,
    SUB,
    MUL,
    DIV,

    POW,
    SQRT,
    SIN,
    COS,
    TAN,
    COT,

    L_BRACKET, 
    R_BRACKET,

    ASSIGN,
    
    IF,
    WHILE,

    PROGRAM_END, 

    LESS,
    GREATER,
    LESS_EQ,
    GREATER_EQ,
    EQ,
    NOT_EQ,
    
    AND,
    OR,
    
    TYPE_INT,

    FIFTY_SEVEN,

    L_BRACE,
    PRINT,
};

union TokenValue
{
    LangOpId        langOpId;
    char*           name;
    char*           stringLiteral;
    int             num;
};

enum class TokenValueType
{
    LANG_OP,
    NAME,
    STRING_LITERAL,
    NUM,
};

struct Token
{
    TokenValue     value;
    TokenValueType valueType;

    size_t line;
    size_t pos;
};

#endif