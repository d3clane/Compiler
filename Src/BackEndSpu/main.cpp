#include <assert.h>

#include "BackEndSpu.h"
#include "Tree/Tree.h"
#include "Common/Log.h"

int main(int argc, char* argv[])
{
    assert(argc > 3);

    LogOpen(argv[0]);
    setbuf(stdout, nullptr);
    FILE* inStream     = fopen(argv[1], "r");
    FILE* outStream    = fopen(argv[2], "w");
    FILE* outBinStream = fopen(argv[3], "w");

    Tree tree = {};
    TreeCtor(&tree);

    TreeReadPrefixFormat(&tree, inStream);

    TreeGraphicDump(&tree, true);

    AsmCodeBuild(&tree, outStream, outBinStream);

    TreeDtor(&tree);
}