#include <math.h>
#include <execinfo.h> 
#include <string.h>

#include "LabelTableArrayFuncs.h"
#include "LabelTable.h"
#include "Common/Log.h"

//----------static functions------------

static LabelTableErrors LabelTableRealloc(LabelTableType* labelTable, bool increase);

static inline LabelTableValue* MovePtr(LabelTableValue* const data, const size_t moveSz, const int times);

static inline size_t LabelTableGetSizeForCalloc(LabelTableType* const labelTable);

static void LabelTableDataFill(LabelTableType* const labelTable);

static inline bool LabelTableIsFull(LabelTableType* labelTable);

static inline bool LabelTableIsTooBig(LabelTableType* labelTable);

//--------CANARY PROTECTION----------

#ifdef LABEL_TABLE_CANARY_PROTECTION

    #define CanaryTypeFormat "%#0llx"
    const CanaryType Canary = 0xDEADBABE;

    const size_t Aligning = 8;

    static inline void CanaryCtor(void* storage)
    {
        assert(storage);
        CanaryType* strg = (CanaryType*) (storage);

        *strg = Canary;
    }

    static inline LabelTableValue* GetAfterFirstCanaryAdr(const LabelTableType* const labelTable)
    {
        return MovePtr(labelTable->data, sizeof(CanaryType), 1);
    }

    static inline LabelTableValue* GetFirstCanaryAdr(const LabelTableType* const labelTable)
    {
        return MovePtr(labelTable->data, sizeof(CanaryType), -1);
    }

    static inline LabelTableValue* GetSecondCanaryAdr(const LabelTableType* const labelTable)
    {
        return (LabelTableValue*)((char*)(labelTable->data + labelTable->capacity) +
                           Aligning - (labelTable->capacity * sizeof(LabelTableValue)) % Aligning);
    }

#endif

//-----------HASH PROTECTION---------------

#ifdef LABEL_TABLE_HASH_PROTECTION

    static inline HashType CalcDataHash(const LabelTableType* labelTable)
    {
        return labelTable->HashFunc(labelTable->data, labelTable->capacity * sizeof(LabelTableValue), 0);        
    }

    static inline void UpdateDataHash(LabelTableType* labelTable)
    {
        labelTable->dataHash = CalcDataHash(labelTable);      
    }

    static inline void UpdateStructHash(LabelTableType* labelTable)
    {
        labelTable->structHash = 0;                                  
        labelTable->structHash = LabelTableMurmurHash(labelTable, sizeof(*labelTable));        
    }
    
#endif

//--------------Consts-----------------

static const size_t STANDARD_CAPACITY = 64;

//---------------

#ifndef NDEBUG

    #define LABEL_TABLE_CHECK(labelTable)                             \
    do                                                              \
    {                                                               \
        LabelTableErrors labelTableErr = LabelTableVerify(labelTable);  \
                                                                    \
        if (labelTableErr != LabelTableErrors::NO_ERR)                \
        {                                                           \
            LABEL_TABLE_DUMP(labelTable);                             \
            return labelTableErr;                    \
        }                                       \
    } while (0)

    #define LABEL_TABLE_CHECK_NO_RETURN(labelTable)         \
    do                                         \
    {                                          \
        LabelTableErrors labelTableErr = LabelTableVerify(labelTable);     \
                                                        \
        if (labelTableErr != LabelTableErrors::NO_ERR)   \
        {                                               \
            LABEL_TABLE_DUMP(labelTable);                   \
        }                                      \
    } while (0)

#else

    #define LABEL_TABLE_CHECK(labelTable)           
    #define LABEL_TABLE_CHECK_NO_RETURN(labelTable) 

#endif

//---------------

#define IF_ERR_RETURN(ERR)              \
do                                      \
{                                       \
    if (ERR != LabelTableErrors::NO_ERR)                            \
        return ERR;                     \
} while (0)

//---------------

#ifdef LABEL_TABLE_HASH_PROTECTION
LabelTableErrors LabelTableCtor(LabelTableType* const labelTable, const size_t capacity, 
                          const HashFuncType HashFunc)
{
    assert(labelTable);

    ON_HASH
    (
        labelTable->HashFunc = HashFunc;
    )

    //--------SET STRUCT CANARY-------
    ON_CANARY
    (
        labelTable->structCanaryLeft  = Canary;
        labelTable->structCanaryRight = Canary;
    )

    LabelTableErrors errors = LabelTableErrors::NO_ERR;
    labelTable->size = 0;

    if (capacity > 0) labelTable->capacity = capacity;
    else              labelTable->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    labelTable->data = (LabelTableValue*) calloc(LabelTableGetSizeForCalloc(labelTable), sizeof(*labelTable->data));

    if (labelTable->data == nullptr)
    {
        LabelTablePrintError(LabelTableErrors::MEMORY_ALLOCATION_ERROR);  
        return LabelTableErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    LabelTableDataFill(labelTable);

    ON_CANARY
    (
        labelTable->data = GetAfterFirstCanaryAdr(labelTable);
    )

    ON_HASH
    (
        UpdateDataHash(labelTable);
        UpdateStructHash(labelTable);
    )

    LABEL_TABLE_CHECK(labelTable);

    return errors;
}
#else 
LabelTableErrors LabelTableCtor(LabelTableType** const outLabelTable, const size_t capacity)
{
    assert(outLabelTable);

    //--------SET STRUCT CANARY-------

    LabelTableType* labelTable = (LabelTableType*) calloc(1, sizeof(*labelTable));

    ON_CANARY
    (
        labelTable->structCanaryLeft  = Canary;
        labelTable->structCanaryRight = Canary;
    )

    LabelTableErrors errors = LabelTableErrors::NO_ERR;
    labelTable->size = 0;

    if (capacity > 0) labelTable->capacity = capacity;
    else              labelTable->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    labelTable->data = (LabelTableValue*) calloc(LabelTableGetSizeForCalloc(labelTable), sizeof(*labelTable->data));

    if (labelTable->data == nullptr)
    {
        LabelTablePrintError(LabelTableErrors::MEMORY_ALLOCATION_ERROR);  
        return LabelTableErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    LabelTableDataFill(labelTable);

    ON_CANARY
    (
        labelTable->data = GetAfterFirstCanaryAdr(labelTable);
    )

    LABEL_TABLE_CHECK(labelTable);

    *outLabelTable = labelTable;
    
    return errors;    
}
#endif

LabelTableErrors LabelTableDtor(LabelTableType* const labelTable)
{
    assert(labelTable);

    LABEL_TABLE_CHECK(labelTable);
    
    for (size_t i = 0; i < labelTable->size; ++i)
    {
        free(labelTable->data[i].label);
        labelTable->data[i] = LABEL_TABLE_POISON;
    }

    ON_CANARY
    (
        labelTable->data = GetFirstCanaryAdr(labelTable);
    )

    free(labelTable->data);
    labelTable->data = nullptr;

    labelTable->size     = 0;
    labelTable->capacity = 0;

    ON_HASH
    (
        labelTable->dataHash = 0;
    )

    ON_CANARY
    (
        labelTable->structCanaryLeft  = 0;
        labelTable->structCanaryRight = 0;
    )

    return LabelTableErrors::NO_ERR;
}

LabelTableErrors LabelTablePush(LabelTableType* labelTable, const LabelTableValue val)
{
    assert(labelTable);
    
    LABEL_TABLE_CHECK(labelTable);

    LabelTableErrors labelTableReallocErr = LabelTableErrors::NO_ERR;
    if (LabelTableIsFull(labelTable)) labelTableReallocErr = LabelTableRealloc(labelTable, true);

    IF_ERR_RETURN(labelTableReallocErr);

    labelTable->data[labelTable->size++] = val;

    ON_HASH
    (
        UpdateDataHash(labelTable);
        UpdateStructHash(labelTable);
    )

    LABEL_TABLE_CHECK(labelTable);

    return LabelTableErrors::NO_ERR;
}

LabelTableErrors LabelTableFind(const LabelTableType* table, const char* name, LabelTableValue** outLabel)
{
    assert(table);
    assert(outLabel);

    LABEL_TABLE_CHECK(table);

    for (size_t i = 0; i < table->size; ++i)
    {
        if (strcmp(table->data[i].label, name) == 0)
        {
            *outLabel = table->data + i;
            return LabelTableErrors::NO_ERR;
        }
    }

    *outLabel = nullptr;

    return LabelTableErrors::NO_ERR;
}

LabelTableErrors LabelTableGetPos(const LabelTableType* table, LabelTableValue* namePtr, size_t* outPos)
{
    assert(table);
    assert(namePtr);
    assert(outPos);
    assert(namePtr >= table->data);

    *outPos = (size_t)(namePtr - table->data);

    return LabelTableErrors::NO_ERR;
}

LabelTableErrors LabelTableVerify(const LabelTableType* labelTable)
{
    assert(labelTable);

    if (labelTable->data == nullptr)
    {
        LabelTablePrintError(LabelTableErrors::LABEL_TABLE_IS_NULLPTR);
        return LabelTableErrors::LABEL_TABLE_IS_NULLPTR;
    }

    if (labelTable->capacity <= 0)
    {  
        LabelTablePrintError(LabelTableErrors::CAPACITY_OUT_OF_RANGE);
        return LabelTableErrors::CAPACITY_OUT_OF_RANGE;
    }

    if (labelTable->size > labelTable->capacity)
    {
        LabelTablePrintError(LabelTableErrors::SIZE_OUT_OF_RANGE);
        return LabelTableErrors::SIZE_OUT_OF_RANGE;
    }

    //-----------Canary checking----------

    ON_CANARY
    (
        if (*(CanaryType*)(GetFirstCanaryAdr(labelTable)) != Canary)
        {
            LabelTablePrintError(LabelTableErrors::INVALID_CANARY);
            return LabelTableErrors::INVALID_CANARY;
        }

        if (*(CanaryType*)(GetSecondCanaryAdr(labelTable)) != Canary)
        {
            LabelTablePrintError(LabelTableErrors::INVALID_CANARY);
            return LabelTableErrors::INVALID_CANARY;
        }

        if (labelTable->structCanaryLeft != Canary)
        {
            LabelTablePrintError(LabelTableErrors::INVALID_CANARY);
            return LabelTableErrors::INVALID_CANARY;
        }

        if (labelTable->structCanaryRight != Canary)
        {
            LabelTablePrintError(LabelTableErrors::INVALID_CANARY);
            return LabelTableErrors::INVALID_CANARY;
        }

    )

    //------------Hash checking----------

    ON_HASH
    (
        if (CalcDataHash(labelTable) != labelTable->dataHash)
        {
            LabelTablePrintError(LabelTableErrors::INVALID_DATA_HASH);
            return LabelTableErrors::INVALID_DATA_HASH;
        }
    )

    ON_HASH
    (
        if (CalcDataHash(labelTable) != labelTable->dataHash)
        {
            LabelTablePrintError(LabelTableErrors::INVALID_DATA_HASH);
            return LabelTableErrors::INVALID_DATA_HASH;
        }

        HashType prevStructHash = labelTable->structHash;
        UpdateStructHash(labelTable);

        if (prevStructHash != labelTable->structHash)
        {
            LabelTablePrintError(LabelTableErrors::INVALID_STRUCT_HASH);
            labelTable->structHash = prevStructHash;
            return LabelTableErrors::INVALID_STRUCT_HASH;
        }
    )


    return LabelTableErrors::NO_ERR;
}

void LabelTableDump(const LabelTableType* labelTable, const char* const fileName, 
                                     const char* const funcName,
                                     const int lineNumber)
{
    assert(labelTable);
    assert(fileName);
    assert(funcName);
    assert(lineNumber > 0);
    
    LOG_BEGIN();

    Log("LabelTableDump was called in file %s, function %s, line %d\n", fileName, funcName, lineNumber);
    Log("labelTable[%p]\n{\n", labelTable);
    Log("\tlabelTable capacity: %zu, \n"
        "\tlabelTable size    : %zu,\n",
        labelTable->capacity, labelTable->size);
    
    ON_CANARY
    (
        Log("\tLeft struct canary : " CanaryTypeFormat ",\n", 
            labelTable->structCanaryLeft);
        Log("\tRight struct canary: " CanaryTypeFormat ",\n", 
            labelTable->structCanaryRight);

        Log("\tLeft data canary : " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetFirstCanaryAdr(labelTable)));
        Log("\tRight data canary: " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetSecondCanaryAdr(labelTable)));
    )

    ON_HASH
    (
        Log("\tData hash  : %llu\n", labelTable->dataHash);
        Log("\tStruct hash: %llu\n", labelTable->structHash);
    )

    Log("\tdata data[%p]\n\t{\n", labelTable->data);

    /*
    if (labelTable->data != nullptr)
    {
        for (size_t i = 0; i < (labelTable->size < labelTable->capacity ? labelTable->size : labelTable->capacity); ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, labelTable->data[i]);

            if (Equal(&labelTable->data[i], &LABEL_TABLE_POISON)) Log(" (LABEL_TABLE_POISON)");

            Log("\n");
        }

        Log("\t\tNot used values:\n");

        for(size_t i = labelTable->size; i < labelTable->capacity; ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, labelTable->data[i]);
            
            //if (Equal(&labelTable->data[i], &LABEL_TABLE_POISON)) Log(" (LABEL_TABLE_POISON)");

            Log("\n");
        }
    }
    */

    Log("\t}\n}\n");

    LOG_END();
}

LabelTableErrors LabelTableRealloc(LabelTableType* labelTable, bool increase)
{
    assert(labelTable);

    LABEL_TABLE_CHECK(labelTable);
    
    if (increase) labelTable->capacity <<= 1;
    else          labelTable->capacity >>= 1;

    if (!increase) 
        FillLabelTable(labelTable->data + labelTable->capacity, labelTable->data + labelTable->size, LABEL_TABLE_POISON);

    //--------Moves data to the first canary-------
    ON_CANARY
    (
        labelTable->data = GetFirstCanaryAdr(labelTable);
    )

    LabelTableValue* tmpLabelTable = (LabelTableValue*) realloc(labelTable->data, 
                                             LabelTableGetSizeForCalloc(labelTable) * sizeof(*labelTable->data));

    if (tmpLabelTable == nullptr)
    {
        LabelTablePrintError(LabelTableErrors::MEMORY_ALLOCATION_ERROR);

        assert(labelTable);
        if (increase) labelTable->capacity >>= 1;
        else          labelTable->capacity <<= 1;

        LABEL_TABLE_CHECK(labelTable);

        return LabelTableErrors::NO_ERR;
    }

    labelTable->data = tmpLabelTable;

    // -------Moving forward after reallocing-------
    ON_CANARY
    (
        labelTable->data = GetAfterFirstCanaryAdr(labelTable);
    )

    if (increase)
        FillLabelTable(labelTable->data + labelTable->size, labelTable->data + labelTable->capacity, LABEL_TABLE_POISON);

    // -------Putting canary at the end-----------
    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(labelTable));
    )

    ON_HASH
    (
        UpdateDataHash(labelTable);
        UpdateStructHash(labelTable);
    )
    
    LABEL_TABLE_CHECK(labelTable);

    return LabelTableErrors::NO_ERR;
}

static inline bool LabelTableIsFull(LabelTableType* labelTable)
{
    assert(labelTable);

    LABEL_TABLE_CHECK_NO_RETURN(labelTable);

    return labelTable->size >= labelTable->capacity;
}

static inline bool LabelTableIsTooBig(LabelTableType* labelTable)
{
    assert(labelTable);

    LABEL_TABLE_CHECK_NO_RETURN(labelTable);

    return (labelTable->size * 4 <= labelTable->capacity) & (labelTable->capacity > STANDARD_CAPACITY);
}

static inline LabelTableValue* MovePtr(LabelTableValue* const data, const size_t moveSz, const int times)
{
    assert(data);
    assert(moveSz > 0);
    
    return (LabelTableValue*)((char*)data + times * (long long)moveSz);
}

// NO labelTable check because doesn't fill hashes
static void LabelTableDataFill(LabelTableType* const labelTable)
{
    assert(labelTable);
    assert(labelTable->data);
    assert(labelTable->capacity > 0);

    ON_CANARY
    (
        CanaryCtor(labelTable->data);
        labelTable->data = GetAfterFirstCanaryAdr(labelTable);
    )

    FillLabelTable(labelTable->data, labelTable->data + labelTable->capacity, LABEL_TABLE_POISON);

    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(labelTable));
        labelTable->data = GetFirstCanaryAdr(labelTable);
    )

    // No labelTable check because doesn't fill hashes
}

// no LABEL_TABLE_CHECK because can be used for callocing memory (data could be nullptr at this moment)
static inline size_t LabelTableGetSizeForCalloc(LabelTableType* const labelTable)
{
    assert(labelTable);
    assert(labelTable->capacity > 0);

    ON_CANARY
    (
        return labelTable->capacity + 3 * sizeof(CanaryType) / sizeof(*labelTable->data);
    )

    return labelTable->capacity;
}

#undef LABEL_TABLE_CHECK
#undef IF_ERR_RETURN
#undef GetAfterFirstCanaryAdr
#undef GetFirstCanaryAdr
#undef GetSecondCanaryAdr

#define LOG_ERR(X) Log(HTML_RED_HEAD_BEGIN "\n" X "\n" HTML_HEAD_END "\n")
void LabelTablePrintError(LabelTableErrors error)
{
    LOG_BEGIN();

    switch(error)
    {
        case LabelTableErrors::CAPACITY_OUT_OF_RANGE:
            LOG_ERR("LabelTable capacity is out of range.\n");
            break;
        case LabelTableErrors::LABEL_TABLE_IS_NULLPTR:
            LOG_ERR("LabelTable is nullptr.\n");
            break;
        case LabelTableErrors::LABEL_TABLE_EMPTY_ERR:
            LOG_ERR("Trying to pop from empty labelTable.\n");
            break;
        case LabelTableErrors::SIZE_OUT_OF_RANGE:
            LOG_ERR("LabelTable size is out of range.\n");
            break;
        case LabelTableErrors::MEMORY_ALLOCATION_ERROR:
            LOG_ERR("Couldn't allocate more memory for labelTable.\n");
            break;
        case LabelTableErrors::INVALID_CANARY:
            LOG_ERR("LabelTable canary is invalid.\n");
            break;
        case LabelTableErrors::INVALID_DATA_HASH:
            LOG_ERR("LabelTable data hash is invalid.\n");
            break;
        case LabelTableErrors::INVALID_STRUCT_HASH:
            LOG_ERR("LabelTable struct hash is invalid.\n");

        case LabelTableErrors::NO_ERR:
        default:
            break;
    }

    LOG_END();
}
#undef PRINT_ERR

void LabelTableValueCtor(LabelTableValue* value, const char* string, IRNode* connectedNode)
{
    value->label         = strdup(string);
    value->connectedNode = connectedNode;
}