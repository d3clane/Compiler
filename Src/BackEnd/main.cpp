#include <assert.h>
#include <string.h>

#include "Tree/Tree.h"
#include "Tree/NameTable/NameTable.h"
#include "IR/IRBuild/IRBuild.h"
#include "TranslateFromIR/x64/x64Translate.h"
#include "Common/Log.h"
#include "Common/CommandLineArgsParser.h"

#include "TranslateFromIR/x64/x64Encode.h"

static void GetFileNames(int argc, const char* argv[], 
                         char** inFileName, char** outBinFileName, char** outAsmFileName);

int main(int argc, const char* argv[])
{
    LogOpen(argv[0]);

    char* inFileName      = nullptr;
    char* outAsmFileName  = nullptr;
    char* outBinFileName  = nullptr;

    GetFileNames(argc, argv, &inFileName, &outBinFileName, &outAsmFileName);

    FILE* inStream     = fopen(inFileName, "r");
    free(inFileName);
    assert(inStream);
    FILE* outBinStream = fopen(outBinFileName, "wb");
    free(outBinFileName);
    assert(outBinStream);

    FILE* outAsmStream = nullptr;
    if (outAsmFileName) 
    {
        outAsmStream = fopen(outAsmFileName, "w");
        free(outAsmFileName);
        assert(outAsmStream);
    }

    Tree tree = {};
    TreeCtor(&tree);

    TreeReadPrefixFormat(&tree, inStream);

    TreeGraphicDump(&tree, true);
    IR* ir = IRBuild(&tree);
    TranslateToX64(ir, outAsmStream, outBinStream);

    TreeDtor(&tree);
    IRDtor(ir);

    fclose(inStream);
    fclose(outBinStream);
    if (outAsmStream) fclose(outAsmStream);
}

static void GetFileNames(int argc, const char* argv[], 
                         char** inFileName, char** outBinFileName, char** outAsmFileName)
{
    static const char* asmOutputOption = "-S";

    if (argc < 3)
    {
        printf("Usage: %s [file with AST] [out binary file] [optional...]\n", argv[0]);
        printf("Optional: %s (asm file output)\n", asmOutputOption);

        exit(0);
    }

    *inFileName     = strdup(argv[1]);    
    *outBinFileName = strdup(argv[2]);
    
    if (argc == 3)
        return;
    
    if (GetCommandLineArgPos(argc, argv, asmOutputOption) != NO_COMMAND_LINE_ARG)
    {
        static const size_t maxAsmFileName  = 256;
        char    asmFileName[maxAsmFileName] = "";

        snprintf(asmFileName, maxAsmFileName, "%s.s", *inFileName);

        *outAsmFileName = strdup(asmFileName);
    }
}