#include <assert.h>

#include "Tree/Tree.h"
#include "Tree/NameTable/NameTable.h"
#include "IR/IRBuild/IRBuild.h"
#include "TranslateFromIR/x64/x64Translate.h"
#include "Common/Log.h"
#include "TranslateFromIR/x64/CodeArray/CodeArray.h"
#include "TranslateFromIR/x64/RodataInfo/Rodata.h"
#include "TranslateFromIR/x64/x64Elf.h"

#include "TranslateFromIR/x64/x64Encode.h"

int main(int argc, char* argv[])
{
    assert(argc > 3);

    LogOpen(argv[0]);
    setbuf(stdout, nullptr);
    FILE* inStream     = fopen(argv[1], "r");
    FILE* outStream    = fopen(argv[2], "w");
    FILE* outBinStream = fopen(argv[3], "wb");
    assert(inStream);
    assert(outStream);
    assert(outBinStream);

    Tree tree = {};
    TreeCtor(&tree);

    NameTableType* allNamesTable = nullptr;
    TreeReadPrefixFormat(&tree, &allNamesTable, inStream);

    //TreeGraphicDump(&tree, true, allNamesTable);

    IR* ir = IRBuild(&tree, allNamesTable);
    IR_TEXT_DUMP(ir);
    TranslateToX64(ir, outStream, outBinStream);

/*
    CodeArrayType* code = nullptr;
    CodeArrayCtor(&code, 0);

    union 
    {
        int value;
        uint8_t bytes[4];
    }tmp;

    RodataInfo rodata = RodataInfoCtor();
    
    RodataStringsValue value = {};
    RodataStringsValueCtor(&value, "enter value: ", "STR_1");
    RodataStringsPush(rodata.rodataStrings, value);
    LoadRodata(&rodata, outBinStream);

    int addr = (int)SegmentAddress::PROGRAM_CODE;
    tmp.value = rodata.rodataStrings->data[0].asmAddr;
    fprintf(stderr, "TMP VALUE - %d\n", tmp.value);

    CodeArrayPush(code, 0x48); CodeArrayPush(code, 0x8d); CodeArrayPush(code, 0x04); CodeArrayPush(code, 0x25); 
    CodeArrayPush(code, tmp.bytes[0]); CodeArrayPush(code, tmp.bytes[1]); CodeArrayPush(code, tmp.bytes[2]); CodeArrayPush(code, tmp.bytes[3]);
    CodeArrayPush(code, 0x50);
    addr += 9;
    addr += 5;
    tmp.value = (int)StdLibAddresses::OUT_STRING - addr;
    CodeArrayPush(code, 0xE8); CodeArrayPush(code, tmp.bytes[0]); CodeArrayPush(code, tmp.bytes[1]); CodeArrayPush(code, tmp.bytes[2]); CodeArrayPush(code, tmp.bytes[3]);

    addr += 5;
    tmp.value = (int)StdLibAddresses::IN_FLOAT - addr;
    CodeArrayPush(code, 0xE8); CodeArrayPush(code, tmp.bytes[0]); CodeArrayPush(code, tmp.bytes[1]); CodeArrayPush(code, tmp.bytes[2]); CodeArrayPush(code, tmp.bytes[3]);

    CodeArrayPush(code, 0x48), CodeArrayPush(code, 0x83),CodeArrayPush(code, 0xec), CodeArrayPush(code, 0x10);
    CodeArrayPush(code, 0xf2), CodeArrayPush(code, 0x0f),CodeArrayPush(code, 0x11), CodeArrayPush(code, 0x04), CodeArrayPush(code, 0x24);
    
    addr += 9;
    addr += 5;
    tmp.value = (int)StdLibAddresses::OUT_FLOAT - addr;
    CodeArrayPush(code, 0xE8); CodeArrayPush(code, tmp.bytes[0]); CodeArrayPush(code, tmp.bytes[1]); CodeArrayPush(code, tmp.bytes[2]); CodeArrayPush(code, tmp.bytes[3]);

    CodeArrayPush(code, 0x48), CodeArrayPush(code, 0x83),CodeArrayPush(code, 0xc4), CodeArrayPush(code, 0x10);
    addr += 4;

    addr += 5;
    tmp.value = (int)StdLibAddresses::HLT - addr;
    CodeArrayPush(code, 0xE8); CodeArrayPush(code, tmp.bytes[0]); CodeArrayPush(code, tmp.bytes[1]); CodeArrayPush(code, tmp.bytes[2]); CodeArrayPush(code, tmp.bytes[3]);

    LoadCode(code, outBinStream);

    RodataInfoDtor(&rodata);

*/
    /*
    X64Operation op = X64Operation::LEA;
    X64Operand op1  = {};
    op1.type = X64OperandType::REG;
    op1.value.imm = 256;
    op1.value.reg = X64Register::RAX;

    X64Operand op2  = {};
    op2.type = X64OperandType::MEM;
    op2.value.imm = -1;
    op2.value.reg = X64Register::RIP;

    size_t outInstructionLen = 0;
    uint8_t* instruction = EncodeX64(op, op1, op2, &outInstructionLen);

    for (size_t i = 0; i < outInstructionLen; ++i)
    {
        printf("%02x ", instruction[i]);
    }
    */
}
