#include <assert.h>
#include <fcntl.h>
#include <string.h>

#include "Tree/Tree.h"
#include "Tree/NameTable/NameTable.h"
#include "IR/IRBuild/IRBuild.h"
#include "TranslateFromIR/x64/x64Translate.h"
#include "Common/Log.h"

#include "TranslateFromIR/x64/x64Encode.h"

static void ReadArgs(int argc, const char* argv[], 
                     const char** inFileName, const char** outBinFileName, 
                     const char** outAsmFileName);

int main(int argc, const char* argv[])
{
    LogOpen(argv[0]);

    const char* inFileName      = nullptr;
    const char* outAsmFileName  = nullptr;
    const char* outBinFileName  = nullptr;

    ReadArgs(argc, argv, &inFileName, &outBinFileName, &outAsmFileName);

    FILE* inStream     = fopen(inFileName, "r");
    assert(inStream);

    FILE* outAsmStream = nullptr;
    if (outAsmFileName) 
    {
        outAsmStream = fopen(outAsmFileName, "w");
        assert(outAsmStream);
    }

    int outBinStreamDescriptor = open(outBinFileName, O_WRONLY | O_CREAT, S_IRWXU);
    assert(outBinStreamDescriptor != -1);
    FILE* outBinStream = fdopen(outBinStreamDescriptor, "wb");
    assert(outBinStream);

    Tree tree = {};
    TreeCtor(&tree);

    TreeReadPrefixFormat(&tree, inStream);

    IR* ir = IRBuild(&tree);
    TranslateToX64(ir, outAsmStream, outBinStream);

    TreeDtor(&tree);
    IRDtor(ir);

    fclose(inStream);
    fclose(outBinStream);
    if (outAsmStream) fclose(outAsmStream);
}

static void ReadArgs(int argc, const char* argv[], 
                     const char** inFileName, const char** outBinFileName, 
                     const char** outAsmFileName)
{
    if (argc != 3 && argc != 5)
    {
        printf("Usage: %s [file with AST] [out binary file] [optional...]\n", argv[0]);
        printf("Optional - -S [out asm file name]\n");

        exit(0);
    }

    *inFileName     = argv[1];
    *outBinFileName = argv[2];
    
    assert(strcmp(argv[3], "-S") == 0);
    if (argc == 4)
        *outAsmFileName = argv[4];
}