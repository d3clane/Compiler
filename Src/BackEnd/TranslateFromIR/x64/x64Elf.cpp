#include <assert.h>
#include <elf.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "x64Elf.h"
#include "FastInput/InputOutput.h"
#include "StdLib/StdLib.h"

static void LoadStdLibCode(FILE* outBinary);

static void LoadStdLibRodata(FILE* outBinary, uint64_t* asmAddr);

static void LoadRodataImmediates(RodataImmediatesType* immediates, FILE* outBinary, uint64_t* asmAddr);
static void LoadRodataStrings   (RodataStringsType*    strings,    FILE* outBinary, uint64_t* asmAddr);

enum class HeaderPos
{
    ELF_HEADER = 0,

    STDLIB_PHEADER = sizeof(Elf64_Ehdr),
    
    RODATA_PHEADER = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr),
    
    CODE_PHEADER   = sizeof(Elf64_Ehdr) + 2 * sizeof(Elf64_Phdr),
};

enum class SegmentFilePos
{
    STDLIB_CODE  = 0x1000,
    RODATA       = 0x2000,
    
    PROGRAM_CODE = 0x3000,
};

static const Elf64_Ehdr ElfHeader = 
{
    .e_ident = 
    {
        ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, // Magic 
        ELFCLASS64,                         // 64-bit system
        ELFDATA2LSB,                        // Little-endian
        EV_CURRENT,                         // Version = Current(1)
        ELFOSABI_SYSV,                      // Unix SYSTEM V
        0                                   // filling other part with zeroes
    },

    .e_type    = ET_EXEC,                   // executable
    .e_machine = EM_X86_64,                 // x86_64
    .e_version = EV_CURRENT,                // current version

    .e_entry   = (Elf64_Addr)SegmentAddress::PROGRAM_CODE,
    .e_phoff    = sizeof(Elf64_Ehdr),          // program header table right after elf header

    .e_shoff    = 0,                           // segment header table offset - not used

    .e_flags    = 0,                           // no flags
    .e_ehsize   = sizeof(Elf64_Ehdr),	       // header size

    .e_phentsize = sizeof(Elf64_Phdr),         // program header table one entry size
    .e_phnum     = 3,                          // Number of program header entries.

    .e_shentsize = sizeof(Elf64_Shdr),         // section header size in bytes
    .e_shnum     = 0,                          // number of entries in section header table (not used)
    .e_shstrndx  = SHN_UNDEF,                  // no section header string table
};

static const Elf64_Phdr ProgramCodePheader = 
{
    .p_type   = PT_LOAD,             
    .p_flags  = PF_R | PF_X,                              // read and execute
    .p_offset = (Elf64_Off) SegmentFilePos::PROGRAM_CODE, // code offset in file
    .p_vaddr  = (Elf64_Addr)SegmentAddress::PROGRAM_CODE, // virtual addr
    .p_paddr  = (Elf64_Addr)SegmentAddress::PROGRAM_CODE, // physical addr

    // Can't specify at this moment
    .p_filesz = 0,                                        // number of bytes to load from file
    .p_memsz  = 0,                                        // number of bytes to load in mem

    .p_align  = 0x1000,                                   // 1 page alignment
};

static const Elf64_Phdr StdLibPheader = 
{
    .p_type   = PT_LOAD,             
    .p_flags  = PF_R | PF_X,                             // read and execute
    .p_offset = (Elf64_Off) SegmentFilePos::STDLIB_CODE, // code offset in file
    .p_vaddr  = (Elf64_Addr)SegmentAddress::STDLIB_CODE, // virtual addr
    .p_paddr  = (Elf64_Addr)SegmentAddress::STDLIB_CODE, // physical addr

    // Can't specify at this moment
    .p_filesz = 0,                                       // number of bytes to load from file
    .p_memsz  = 0,                                       // number of bytes to load in mem

    .p_align  = 0x1000,                                  // 1 page alignment
};

static const Elf64_Phdr RodataPheader = 
{
    .p_type   = PT_LOAD,             
    .p_flags  = PF_R,                               // read
    .p_offset = (Elf64_Off) SegmentFilePos::RODATA,  // code offset in file
    .p_vaddr  = (Elf64_Addr)SegmentAddress::RODATA, // virtual addr
    .p_paddr  = (Elf64_Addr)SegmentAddress::RODATA, // physical addr

    // Can't specify at the moment
    .p_filesz = 0,                                  // number of bytes to load from file
    .p_memsz  = 0,                                  // number of bytes to load in mem

    .p_align  = 0x1000,                             // 1 page alignment
};

void LoadCode(CodeArrayType* code, FILE* outBinary) 
{
    assert(code);
    assert(outBinary);

    Elf64_Phdr codePheader = ProgramCodePheader;
    codePheader.p_filesz   = code->size;
    codePheader.p_memsz    = code->size;

    fseek(outBinary, 0, SEEK_SET);
    fwrite(&ElfHeader,   sizeof(ElfHeader),   1, outBinary);

    fseek(outBinary, (long)HeaderPos::CODE_PHEADER, SEEK_SET);
    fwrite(&codePheader, sizeof(codePheader), 1, outBinary);

    LoadStdLibCode(outBinary);

    // Write code
    fseek(outBinary, (long)SegmentFilePos::PROGRAM_CODE, SEEK_SET);
    fwrite(code->data, code->size, 1, outBinary);
}

static void LoadStdLibCode(FILE* outBinary)
{
    assert(outBinary);

    FILE* stdLibStream = fopen(StdLibCodeName, "rb");

    uint8_t* text = (uint8_t*)ReadText(stdLibStream);

    Elf64_Ehdr* elfHeader = (Elf64_Ehdr*)text;
    
    assert(elfHeader->e_phnum == 3);
    assert(elfHeader->e_entry == (Elf64_Addr)StdLibAddresses::ENTRY);
    Elf64_Phdr* codePheader = (Elf64_Phdr*) (text + elfHeader->e_phoff + sizeof(Elf64_Phdr));
    // assuming that code program header is the second one in program header table
    assert(codePheader->p_vaddr  == (Elf64_Addr)SegmentAddress::STDLIB_CODE &&
           codePheader->p_vaddr  == (Elf64_Addr)StdLibAddresses::ENTRY);

    // Print pheader
    Elf64_Phdr stdLibPheader = StdLibPheader;
    stdLibPheader.p_filesz = codePheader->p_filesz;
    stdLibPheader.p_memsz  = codePheader->p_memsz;

    fseek(outBinary, (long)HeaderPos::STDLIB_PHEADER, SEEK_SET);
    fwrite(&stdLibPheader, sizeof(stdLibPheader), 1, outBinary);

    // Print code
    fseek(outBinary, (long)SegmentFilePos::STDLIB_CODE, SEEK_SET);
    fwrite(text + codePheader->p_offset, codePheader->p_filesz, 1, outBinary);

    free(text);
    fclose(stdLibStream);
}

void LoadRodata(RodataInfo* rodata, FILE* outBinary)
{
    assert(rodata);
    assert(outBinary);

    uint64_t asmAddr = (uint64_t)SegmentAddress::RODATA;

    fseek(outBinary, (long)SegmentFilePos::RODATA, SEEK_SET);
    LoadStdLibRodata    (outBinary, &asmAddr);
    LoadRodataImmediates(rodata->rodataImmediates, outBinary, &asmAddr);
    LoadRodataStrings   (rodata->rodataStrings,    outBinary, &asmAddr);

    // Load rodata pheader 
    Elf64_Phdr rodataPheader = RodataPheader;

    uint64_t fileSize = asmAddr - (uint64_t)SegmentAddress::RODATA;
    rodataPheader.p_filesz = fileSize;
    rodataPheader.p_memsz  = fileSize;

    fseek(outBinary, (long)HeaderPos::RODATA_PHEADER, SEEK_SET);
    fwrite(&rodataPheader, sizeof(rodataPheader), 1, outBinary);
}

static void LoadRodataImmediates(RodataImmediatesType* immediates, FILE* outBinary, uint64_t* asmAddr)
{
    assert(immediates);
    assert(outBinary);
    assert(asmAddr);
    
    for (size_t i = 0; i < immediates->size; ++i)
    {
        long long imm = immediates->data[i].imm;
        double value = (double)imm;

        fwrite(&value, sizeof(value), 1, outBinary);

        immediates->data[i].asmAddr = *asmAddr;
        *asmAddr += sizeof(value);
    }
}

static void LoadRodataStrings(RodataStringsType* strings, FILE* outBinary, uint64_t* asmAddr)
{
    assert(strings);
    assert(outBinary);
    assert(asmAddr);

    for (size_t i = 0; i < strings->size; ++i)
    {
        const char* string = strings->data[i].string;
        
        int putsRes = fputs(string, outBinary);
        assert(putsRes != EOF);
        putsRes     = fputc('\0', outBinary);
        assert(putsRes != EOF);

        strings->data[i].asmAddr = *asmAddr;
        *asmAddr += strlen(string) + 1;
    }
}

static void LoadStdLibRodata(FILE* outBinary, uint64_t* asmAddr)
{
    assert(outBinary);
    assert(asmAddr);

    FILE* stdLibStream = fopen(StdLibCodeName, "rb");

    uint8_t* text = (uint8_t*)ReadText(stdLibStream);

    Elf64_Ehdr* elfHeader = (Elf64_Ehdr*)text;
    
    assert(elfHeader->e_phnum == 3);
    assert(elfHeader->e_entry == (Elf64_Addr)StdLibAddresses::ENTRY);
    Elf64_Phdr* rodataPheader = (Elf64_Phdr*) (text + elfHeader->e_phoff + 2 * sizeof(Elf64_Phdr));
    // assuming that code program header is the third one in program header table
    assert(rodataPheader->p_vaddr == (Elf64_Addr)SegmentAddress::RODATA);

    // Print code
    fwrite(text + rodataPheader->p_offset, rodataPheader->p_filesz, 1, outBinary);
    *asmAddr += rodataPheader->p_filesz;

    free(text);
    fclose(stdLibStream);
}