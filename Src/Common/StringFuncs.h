#ifndef STRING_FUNCS_H
#define STRING_FUNCS_H

const char* SkipSymbolsUntilStopChar (const char* string, const char stopChar);
const char* SkipSymbolsWhileStatement(const char* string, int (*statementFunc)(int));
const char* SkipSymbolsWhileChar     (const char* string, const char skippingChar);

#endif