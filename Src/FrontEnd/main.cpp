#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "Common/Log.h"
#include "SyntaxParser.h"
#include "FastInput/InputOutput.h"

int main(int argc, char* argv[])
{
    assert(argc > 2);
    LogOpen(argv[0]);
    setbuf(stdout, nullptr);

    FILE* inStream  = fopen(argv[1], "r");
    FILE* outStream = fopen(argv[2], "w");

    char* inputTxt = ReadText(inStream);
    
    SyntaxParserErrors err = SyntaxParserErrors::NO_ERR;
    Tree ast = CodeParse(inputTxt, &err);

    if (err == SyntaxParserErrors::NO_ERR)
        TreePrintPrefixFormat(&ast, outStream);

    TreeGraphicDump(&ast, true);

    free(inputTxt);
    TreeDtor(&ast);

    fclose(inStream);
    fclose(outStream);

    return (int)err;
}
