#include <assert.h>

#include "Tree/Tree.h"
#include "Tree/NameTable/NameTable.h"
#include "IR/IRBuild/IRBuild.h"
#include "TranslateFromIR/x64/x64Translate.h"
#include "Common/Log.h"

#include "TranslateFromIR/x64/x64Encode.h"

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
    //TranslateToX64(ir, outStream);

    X64Operation op = X64Operation::PUSH;
    X64Operand op1  = {};
    op1.type = X64OperandType::REG;
    op1.value.reg = X64Register::RAX;

    X64Operand op2  = {};
    op2.type = X64OperandType::IMM;
    op2.value.imm = 133;

    size_t outInstructionLen = 0;
    uint8_t* instruction = EncodeX64(op, op1, &outInstructionLen);

    for (size_t i = 0; i < outInstructionLen; ++i)
    {
        printf("%02x ", instruction[i]);
    }
}
