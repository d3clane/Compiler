#include <assert.h>
#include <fcntl.h>

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

    NameTableType* allNamesTable = nullptr;
    TreeReadPrefixFormat(&tree, &allNamesTable, inStream);

    //TreeGraphicDump(&tree, true, allNamesTable);

    IR* ir = IRBuild(&tree, allNamesTable);
    TranslateToX64(ir, outAsmStream, outBinStream);

    TreeDtor(&tree);
    NameTableDtor(allNamesTable);
    IRDtor(ir);

    fclose(inStream);
    fclose(outBinStream);
    if (outAsmStream) fclose(outAsmStream);
}

static void ReadArgs(int argc, const char* argv[], 
                     const char** inFileName, const char** outBinFileName, 
                     const char** outAsmFileName)
{
    if (argc != 3 && argc != 4)
    {
        printf("Usage: %s [file with AST] [out binary file] [optional...]\n", argv[0]);
        printf("Optional - [out asm file name]\n");

        exit(0);
    }

    *inFileName     = argv[1];
    *outBinFileName = argv[2];
    
    if (argc == 4)
        *outAsmFileName = argv[3];
}