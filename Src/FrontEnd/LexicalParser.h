#ifndef LEXICAL_PARSER_H
#define LEXICAL_PARSER_H

#include <stddef.h>
#include "TokensArr/TokensArr.h"

enum class LexicalParserErrors
{
    NO_ERR,

    SYNTAX_ERR,
};

Token TokenCopy(const Token* token);
Token TokenCreate(TokenValue value, TokenValueType valueType,   const size_t line, 
                                                                const size_t pos);

TokenValue TokenValueCreateName   (const char* name);
TokenValue TokenValueCreateNum    (const int value);
TokenValue TokenValueCreateLangOp (const LangOpId tokenId);

TokenValue TokenValueCreateLiteral(const char* string);

LexicalParserErrors ParseOnTokens(const char* code, TokensArr* tokens);

#endif 