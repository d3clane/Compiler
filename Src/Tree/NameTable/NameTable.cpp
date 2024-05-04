#include <math.h>
#include <execinfo.h> 
#include <string.h>

#include "ArrayFuncs.h"
#include "NameTable.h"
#include "Common/Log.h"

//----------static functions------------

static NameTableErrors NameTableRealloc(NameTableType* nameTable, bool increase);

static inline Name* MovePtr(Name* const data, const size_t moveSz, const int times);

static inline size_t NameTableGetSizeForCalloc(NameTableType* const nameTable);

static void NameTableDataFill(NameTableType* const nameTable);

static inline bool NameTableIsFull(NameTableType* nameTable);

static inline bool NameTableIsTooBig(NameTableType* nameTable);

//--------CANARY PROTECTION----------

#ifdef NAME_TABLE_CANARY_PROTECTION

    #define CanaryTypeFormat "%#0llx"
    const CanaryType Canary = 0xDEADBABE;

    const size_t Aligning = 8;

    static inline void CanaryCtor(void* storage)
    {
        assert(storage);
        CanaryType* strg = (CanaryType*) (storage);

        *strg = Canary;
    }

    static inline Name* GetAfterFirstCanaryAdr(const NameTableType* const nameTable)
    {
        return MovePtr(nameTable->data, sizeof(CanaryType), 1);
    }

    static inline Name* GetFirstCanaryAdr(const NameTableType* const nameTable)
    {
        return MovePtr(nameTable->data, sizeof(CanaryType), -1);
    }

    static inline Name* GetSecondCanaryAdr(const NameTableType* const nameTable)
    {
        return (Name*)((char*)(nameTable->data + nameTable->capacity) +
                           Aligning - (nameTable->capacity * sizeof(Name)) % Aligning);
    }

#endif

//-----------HASH PROTECTION---------------

#ifdef NAME_TABLE_HASH_PROTECTION

    static inline HashType CalcDataHash(const NameTableType* nameTable)
    {
        return nameTable->HashFunc(nameTable->data, nameTable->capacity * sizeof(Name), 0);        
    }

    static inline void UpdateDataHash(NameTableType* nameTable)
    {
        nameTable->dataHash = CalcDataHash(nameTable);      
    }

    static inline void UpdateStructHash(NameTableType* nameTable)
    {
        nameTable->structHash = 0;                                  
        nameTable->structHash = NameTableMurmurHash(nameTable, sizeof(*nameTable));        
    }
    
#endif

//--------------Consts-----------------

static const size_t STANDARD_CAPACITY = 64;

//---------------

#ifndef NDEBUG

    #define NAME_TABLE_CHECK(nameTable)                             \
    do                                                              \
    {                                                               \
        NameTableErrors nameTableErr = NameTableVerify(nameTable);  \
                                                                    \
        if (nameTableErr != NameTableErrors::NO_ERR)                \
        {                                                           \
            NAME_TABLE_DUMP(nameTable);                             \
            return nameTableErr;                    \
        }                                       \
    } while (0)

    #define NAME_TABLE_CHECK_NO_RETURN(nameTable)         \
    do                                         \
    {                                          \
        NameTableErrors nameTableErr = NameTableVerify(nameTable);     \
                                                        \
        if (nameTableErr != NameTableErrors::NO_ERR)   \
        {                                               \
            NAME_TABLE_DUMP(nameTable);                   \
        }                                      \
    } while (0)

#else

    #define NAME_TABLE_CHECK(nameTable)           
    #define NAME_TABLE_CHECK_NO_RETURN(nameTable) 

#endif

//---------------

#define IF_ERR_RETURN(ERR)              \
do                                      \
{                                       \
    if (ERR != NameTableErrors::NO_ERR)                            \
        return ERR;                     \
} while (0)

//---------------

#ifdef NAME_TABLE_HASH_PROTECTION
NameTableErrors NameTableCtor(NameTableType* const nameTable, const size_t capacity, 
                          const HashFuncType HashFunc)
{
    assert(nameTable);

    ON_HASH
    (
        nameTable->HashFunc = HashFunc;
    )

    //--------SET STRUCT CANARY-------
    ON_CANARY
    (
        nameTable->structCanaryLeft  = Canary;
        nameTable->structCanaryRight = Canary;
    )

    NameTableErrors errors = NameTableErrors::NO_ERR;
    nameTable->size = 0;

    if (capacity > 0) nameTable->capacity = capacity;
    else              nameTable->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    nameTable->data = (Name*) calloc(NameTableGetSizeForCalloc(nameTable), sizeof(*nameTable->data));

    if (nameTable->data == nullptr)
    {
        NameTablePrintError(NameTableErrors::MEMORY_ALLOCATION_ERROR);  
        return NameTableErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    NameTableDataFill(nameTable);

    ON_CANARY
    (
        nameTable->data = GetAfterFirstCanaryAdr(nameTable);
    )

    ON_HASH
    (
        UpdateDataHash(nameTable);
        UpdateStructHash(nameTable);
    )

    NAME_TABLE_CHECK(nameTable);

    return errors;
}
#else 
NameTableErrors NameTableCtor(NameTableType** const outNameTable, const size_t capacity)
{
    assert(outNameTable);

    //--------SET STRUCT CANARY-------

    NameTableType* nameTable = (NameTableType*) calloc(1, sizeof(*nameTable));

    ON_CANARY
    (
        nameTable->structCanaryLeft  = Canary;
        nameTable->structCanaryRight = Canary;
    )

    NameTableErrors errors = NameTableErrors::NO_ERR;
    nameTable->size = 0;

    if (capacity > 0) nameTable->capacity = capacity;
    else              nameTable->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    nameTable->data = (Name*) calloc(NameTableGetSizeForCalloc(nameTable), sizeof(*nameTable->data));

    if (nameTable->data == nullptr)
    {
        NameTablePrintError(NameTableErrors::MEMORY_ALLOCATION_ERROR);  
        return NameTableErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    NameTableDataFill(nameTable);

    ON_CANARY
    (
        nameTable->data = GetAfterFirstCanaryAdr(nameTable);
    )

    NAME_TABLE_CHECK(nameTable);

    *outNameTable = nameTable;
    
    return errors;    
}
#endif

NameTableErrors NameTableDtor(NameTableType* const nameTable)
{
    assert(nameTable);

    NAME_TABLE_CHECK(nameTable);
    
    for (size_t i = 0; i < nameTable->size; ++i)
    {
        free(nameTable->data[i].name);
        nameTable->data[i] = NAME_TABLE_POISON;
    }

    ON_CANARY
    (
        nameTable->data = GetFirstCanaryAdr(nameTable);
    )

    free(nameTable->data);
    nameTable->data = nullptr;

    nameTable->size     = 0;
    nameTable->capacity = 0;

    ON_HASH
    (
        nameTable->dataHash = 0;
    )

    ON_CANARY
    (
        nameTable->structCanaryLeft  = 0;
        nameTable->structCanaryRight = 0;
    )

    return NameTableErrors::NO_ERR;
}

NameTableErrors NameTablePush(NameTableType* nameTable, const Name val)
{
    assert(nameTable);
    
    NAME_TABLE_CHECK(nameTable);

    NameTableErrors nameTableReallocErr = NameTableErrors::NO_ERR;
    if (NameTableIsFull(nameTable)) nameTableReallocErr = NameTableRealloc(nameTable, true);

    IF_ERR_RETURN(nameTableReallocErr);

    nameTable->data[nameTable->size++] = val;

    ON_HASH
    (
        UpdateDataHash(nameTable);
        UpdateStructHash(nameTable);
    )

    NAME_TABLE_CHECK(nameTable);

    return NameTableErrors::NO_ERR;
}

NameTableErrors NameTableFind(const NameTableType* table, const char* name, Name** outName)
{
    assert(table);
    assert(outName);

    NAME_TABLE_CHECK(table);

    for (size_t i = 0; i < table->size; ++i)
    {
        if (strcmp(table->data[i].name, name) == 0)
        {
            *outName = table->data + i;
            return NameTableErrors::NO_ERR;
        }
    }

    *outName = nullptr;

    return NameTableErrors::NO_ERR;
}

NameTableErrors NameTableGetPos(const NameTableType* table, Name* namePtr, size_t* outPos)
{
    assert(table);
    assert(namePtr);
    assert(outPos);
    assert(namePtr >= table->data);

    *outPos = (size_t)(namePtr - table->data);

    return NameTableErrors::NO_ERR;
}

NameTableErrors NameTableVerify(const NameTableType* nameTable)
{
    assert(nameTable);

    if (nameTable->data == nullptr)
    {
        NameTablePrintError(NameTableErrors::NAME_TABLE_IS_NULLPTR);
        return NameTableErrors::NAME_TABLE_IS_NULLPTR;
    }

    if (nameTable->capacity <= 0)
    {  
        NameTablePrintError(NameTableErrors::CAPACITY_OUT_OF_RANGE);
        return NameTableErrors::CAPACITY_OUT_OF_RANGE;
    }

    if (nameTable->size > nameTable->capacity)
    {
        NameTablePrintError(NameTableErrors::SIZE_OUT_OF_RANGE);
        return NameTableErrors::SIZE_OUT_OF_RANGE;
    }

    //-----------Canary checking----------

    ON_CANARY
    (
        if (*(CanaryType*)(GetFirstCanaryAdr(nameTable)) != Canary)
        {
            NameTablePrintError(NameTableErrors::INVALID_CANARY);
            return NameTableErrors::INVALID_CANARY;
        }

        if (*(CanaryType*)(GetSecondCanaryAdr(nameTable)) != Canary)
        {
            NameTablePrintError(NameTableErrors::INVALID_CANARY);
            return NameTableErrors::INVALID_CANARY;
        }

        if (nameTable->structCanaryLeft != Canary)
        {
            NameTablePrintError(NameTableErrors::INVALID_CANARY);
            return NameTableErrors::INVALID_CANARY;
        }

        if (nameTable->structCanaryRight != Canary)
        {
            NameTablePrintError(NameTableErrors::INVALID_CANARY);
            return NameTableErrors::INVALID_CANARY;
        }

    )

    //------------Hash checking----------

    ON_HASH
    (
        if (CalcDataHash(nameTable) != nameTable->dataHash)
        {
            NameTablePrintError(NameTableErrors::INVALID_DATA_HASH);
            return NameTableErrors::INVALID_DATA_HASH;
        }
    )

    ON_HASH
    (
        if (CalcDataHash(nameTable) != nameTable->dataHash)
        {
            NameTablePrintError(NameTableErrors::INVALID_DATA_HASH);
            return NameTableErrors::INVALID_DATA_HASH;
        }

        HashType prevStructHash = nameTable->structHash;
        UpdateStructHash(nameTable);

        if (prevStructHash != nameTable->structHash)
        {
            NameTablePrintError(NameTableErrors::INVALID_STRUCT_HASH);
            nameTable->structHash = prevStructHash;
            return NameTableErrors::INVALID_STRUCT_HASH;
        }
    )


    return NameTableErrors::NO_ERR;
}

void NameTableDump(const NameTableType* nameTable, const char* const fileName, 
                                     const char* const funcName,
                                     const int lineNumber)
{
    assert(nameTable);
    assert(fileName);
    assert(funcName);
    assert(lineNumber > 0);
    
    LOG_BEGIN();

    Log("NameTableDump was called in file %s, function %s, line %d\n", fileName, funcName, lineNumber);
    Log("nameTable[%p]\n{\n", nameTable);
    Log("\tnameTable capacity: %zu, \n"
        "\tnameTable size    : %zu,\n",
        nameTable->capacity, nameTable->size);
    
    ON_CANARY
    (
        Log("\tLeft struct canary : " CanaryTypeFormat ",\n", 
            nameTable->structCanaryLeft);
        Log("\tRight struct canary: " CanaryTypeFormat ",\n", 
            nameTable->structCanaryRight);

        Log("\tLeft data canary : " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetFirstCanaryAdr(nameTable)));
        Log("\tRight data canary: " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetSecondCanaryAdr(nameTable)));
    )

    ON_HASH
    (
        Log("\tData hash  : %llu\n", nameTable->dataHash);
        Log("\tStruct hash: %llu\n", nameTable->structHash);
    )

    Log("\tdata data[%p]\n\t{\n", nameTable->data);

    /*
    if (nameTable->data != nullptr)
    {
        for (size_t i = 0; i < (nameTable->size < nameTable->capacity ? nameTable->size : nameTable->capacity); ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, nameTable->data[i]);

            if (Equal(&nameTable->data[i], &NAME_TABLE_POISON)) Log(" (NAME_TABLE_POISON)");

            Log("\n");
        }

        Log("\t\tNot used values:\n");

        for(size_t i = nameTable->size; i < nameTable->capacity; ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, nameTable->data[i]);
            
            //if (Equal(&nameTable->data[i], &NAME_TABLE_POISON)) Log(" (NAME_TABLE_POISON)");

            Log("\n");
        }
    }
    */

    Log("\t}\n}\n");

    LOG_END();
}

NameTableErrors NameTableRealloc(NameTableType* nameTable, bool increase)
{
    assert(nameTable);

    NAME_TABLE_CHECK(nameTable);
    
    if (increase) nameTable->capacity <<= 1;
    else          nameTable->capacity >>= 1;

    if (!increase) 
        FillNameTable(nameTable->data + nameTable->capacity, nameTable->data + nameTable->size, NAME_TABLE_POISON);

    //--------Moves data to the first canary-------
    ON_CANARY
    (
        nameTable->data = GetFirstCanaryAdr(nameTable);
    )

    Name* tmpNameTable = (Name*) realloc(nameTable->data, 
                                             NameTableGetSizeForCalloc(nameTable) * sizeof(*nameTable->data));

    if (tmpNameTable == nullptr)
    {
        NameTablePrintError(NameTableErrors::MEMORY_ALLOCATION_ERROR);

        assert(nameTable);
        if (increase) nameTable->capacity >>= 1;
        else          nameTable->capacity <<= 1;

        NAME_TABLE_CHECK(nameTable);

        return NameTableErrors::NO_ERR;
    }

    nameTable->data = tmpNameTable;

    // -------Moving forward after reallocing-------
    ON_CANARY
    (
        nameTable->data = GetAfterFirstCanaryAdr(nameTable);
    )

    if (increase)
        FillNameTable(nameTable->data + nameTable->size, nameTable->data + nameTable->capacity, NAME_TABLE_POISON);

    // -------Putting canary at the end-----------
    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(nameTable));
    )

    ON_HASH
    (
        UpdateDataHash(nameTable);
        UpdateStructHash(nameTable);
    )
    
    NAME_TABLE_CHECK(nameTable);

    return NameTableErrors::NO_ERR;
}

static inline bool NameTableIsFull(NameTableType* nameTable)
{
    assert(nameTable);

    NAME_TABLE_CHECK_NO_RETURN(nameTable);

    return nameTable->size >= nameTable->capacity;
}

static inline bool NameTableIsTooBig(NameTableType* nameTable)
{
    assert(nameTable);

    NAME_TABLE_CHECK_NO_RETURN(nameTable);

    return (nameTable->size * 4 <= nameTable->capacity) & (nameTable->capacity > STANDARD_CAPACITY);
}

static inline Name* MovePtr(Name* const data, const size_t moveSz, const int times)
{
    assert(data);
    assert(moveSz > 0);
    
    return (Name*)((char*)data + times * (long long)moveSz);
}

// NO nameTable check because doesn't fill hashes
static void NameTableDataFill(NameTableType* const nameTable)
{
    assert(nameTable);
    assert(nameTable->data);
    assert(nameTable->capacity > 0);

    ON_CANARY
    (
        CanaryCtor(nameTable->data);
        nameTable->data = GetAfterFirstCanaryAdr(nameTable);
    )

    FillNameTable(nameTable->data, nameTable->data + nameTable->capacity, NAME_TABLE_POISON);

    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(nameTable));
        nameTable->data = GetFirstCanaryAdr(nameTable);
    )

    // No nameTable check because doesn't fill hashes
}

// no NAME_TABLE_CHECK because can be used for callocing memory (data could be nullptr at this moment)
static inline size_t NameTableGetSizeForCalloc(NameTableType* const nameTable)
{
    assert(nameTable);
    assert(nameTable->capacity > 0);

    ON_CANARY
    (
        return nameTable->capacity + 3 * sizeof(CanaryType) / sizeof(*nameTable->data);
    )

    return nameTable->capacity;
}

#undef NAME_TABLE_CHECK
#undef IF_ERR_RETURN
#undef GetAfterFirstCanaryAdr
#undef GetFirstCanaryAdr
#undef GetSecondCanaryAdr

#define LOG_ERR(X) Log(HTML_RED_HEAD_BEGIN "\n" X "\n" HTML_HEAD_END "\n")
void NameTablePrintError(NameTableErrors error)
{
    LOG_BEGIN();

    switch(error)
    {
        case NameTableErrors::CAPACITY_OUT_OF_RANGE:
            LOG_ERR("NameTable capacity is out of range.\n");
            break;
        case NameTableErrors::NAME_TABLE_IS_NULLPTR:
            LOG_ERR("NameTable is nullptr.\n");
            break;
        case NameTableErrors::NAME_TABLE_EMPTY_ERR:
            LOG_ERR("Trying to pop from empty nameTable.\n");
            break;
        case NameTableErrors::SIZE_OUT_OF_RANGE:
            LOG_ERR("NameTable size is out of range.\n");
            break;
        case NameTableErrors::MEMORY_ALLOCATION_ERROR:
            LOG_ERR("Couldn't allocate more memory for nameTable.\n");
            break;
        case NameTableErrors::INVALID_CANARY:
            LOG_ERR("NameTable canary is invalid.\n");
            break;
        case NameTableErrors::INVALID_DATA_HASH:
            LOG_ERR("NameTable data hash is invalid.\n");
            break;
        case NameTableErrors::INVALID_STRUCT_HASH:
            LOG_ERR("NameTable struct hash is invalid.\n");

        case NameTableErrors::NO_ERR:
        default:
            break;
    }

    LOG_END();
}
#undef PRINT_ERR

void NameCtor(Name* name, const char* string, void* localNameTablePtr, 
              int memShift, IRRegister reg)
{
    name->name           = strdup(string);
    name->localNameTable = localNameTablePtr;
    name->memShift       = memShift;
    name->reg          = reg;
}