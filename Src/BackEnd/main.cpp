#include <assert.h>

#include "Tree/Tree.h"
#include "Tree/NameTable/NameTable.h"
#include "IR/IRBuild/IRBuild.h"
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

    NameTableType* allNamesTable = nullptr;
    TreeReadPrefixFormat(&tree, &allNamesTable, inStream);

    //TreeGraphicDump(&tree, true, allNamesTable);

    IR* ir = IRBuild(&tree, allNamesTable);
    IR_TEXT_DUMP(ir);
    
}
