#include <math.h>
#include <execinfo.h> 
#include <string.h>

#include "CodeArrayArrayFuncs.h"
#include "CodeArray.h"
#include "Common/Log.h"

//----------static functions------------

static CodeArrayErrors CodeArrayRealloc(CodeArrayType* codeArray, bool increase);

static inline CodeArrayValue* MovePtr(CodeArrayValue* const data, const size_t moveSz, const int times);

static inline size_t CodeArrayGetSizeForCalloc(CodeArrayType* const codeArray);

static void CodeArrayDataFill(CodeArrayType* const codeArray);

static inline bool CodeArrayIsFull(CodeArrayType* codeArray);

static inline bool CodeArrayIsTooBig(CodeArrayType* codeArray);

//--------CANARY PROTECTION----------

#ifdef CODE_ARRAY_CANARY_PROTECTION

    #define CanaryTypeFormat "%#0llx"
    const CanaryType Canary = 0xDEADBABE;

    const size_t Aligning = 8;

    static inline void CanaryCtor(void* storage)
    {
        assert(storage);
        CanaryType* strg = (CanaryType*) (storage);

        *strg = Canary;
    }

    static inline CodeArrayValue* GetAfterFirstCanaryAdr(const CodeArrayType* const codeArray)
    {
        return MovePtr(codeArray->data, sizeof(CanaryType), 1);
    }

    static inline CodeArrayValue* GetFirstCanaryAdr(const CodeArrayType* const codeArray)
    {
        return MovePtr(codeArray->data, sizeof(CanaryType), -1);
    }

    static inline CodeArrayValue* GetSecondCanaryAdr(const CodeArrayType* const codeArray)
    {
        return (CodeArrayValue*)((char*)(codeArray->data + codeArray->capacity) +
                           Aligning - (codeArray->capacity * sizeof(CodeArrayValue)) % Aligning);
    }

#endif

//-----------HASH PROTECTION---------------

#ifdef CODE_ARRAY_HASH_PROTECTION

    static inline HashType CalcDataHash(const CodeArrayType* codeArray)
    {
        return codeArray->HashFunc(codeArray->data, codeArray->capacity * sizeof(CodeArrayValue), 0);        
    }

    static inline void UpdateDataHash(CodeArrayType* codeArray)
    {
        codeArray->dataHash = CalcDataHash(codeArray);      
    }

    static inline void UpdateStructHash(CodeArrayType* codeArray)
    {
        codeArray->structHash = 0;                                  
        codeArray->structHash = CodeArrayMurmurHash(codeArray, sizeof(*codeArray));        
    }
    
#endif

//--------------Consts-----------------

static const size_t STANDARD_CAPACITY = 64;

//---------------

#ifndef NDEBUG

    #define CODE_ARRAY_CHECK(codeArray)                             \
    do                                                              \
    {                                                               \
        CodeArrayErrors codeArrayErr = CodeArrayVerify(codeArray);  \
                                                                    \
        if (codeArrayErr != CodeArrayErrors::NO_ERR)                \
        {                                                           \
            CODE_ARRAY_DUMP(codeArray);                             \
            return codeArrayErr;                    \
        }                                       \
    } while (0)

    #define CODE_ARRAY_CHECK_NO_RETURN(codeArray)         \
    do                                         \
    {                                          \
        CodeArrayErrors codeArrayErr = CodeArrayVerify(codeArray);     \
                                                        \
        if (codeArrayErr != CodeArrayErrors::NO_ERR)   \
        {                                               \
            CODE_ARRAY_DUMP(codeArray);                   \
        }                                      \
    } while (0)

#else

    #define CODE_ARRAY_CHECK(codeArray)           
    #define CODE_ARRAY_CHECK_NO_RETURN(codeArray) 

#endif

//---------------

#define IF_ERR_RETURN(ERR)              \
do                                      \
{                                       \
    if (ERR != CodeArrayErrors::NO_ERR)                            \
        return ERR;                     \
} while (0)

//---------------

#ifdef CODE_ARRAY_HASH_PROTECTION
CodeArrayErrors CodeArrayCtor(CodeArrayType* const codeArray, const size_t capacity, 
                          const HashFuncType HashFunc)
{
    assert(codeArray);

    ON_HASH
    (
        codeArray->HashFunc = HashFunc;
    )

    //--------SET STRUCT CANARY-------
    ON_CANARY
    (
        codeArray->structCanaryLeft  = Canary;
        codeArray->structCanaryRight = Canary;
    )

    CodeArrayErrors errors = CodeArrayErrors::NO_ERR;
    codeArray->size = 0;

    if (capacity > 0) codeArray->capacity = capacity;
    else              codeArray->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    codeArray->data = (CodeArrayValue*) calloc(CodeArrayGetSizeForCalloc(codeArray), sizeof(*codeArray->data));

    if (codeArray->data == nullptr)
    {
        CodeArrayPrintError(CodeArrayErrors::MEMORY_ALLOCATION_ERROR);  
        return CodeArrayErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    CodeArrayDataFill(codeArray);

    ON_CANARY
    (
        codeArray->data = GetAfterFirstCanaryAdr(codeArray);
    )

    ON_HASH
    (
        UpdateDataHash(codeArray);
        UpdateStructHash(codeArray);
    )

    CODE_ARRAY_CHECK(codeArray);

    return errors;
}
#else 
CodeArrayErrors CodeArrayCtor(CodeArrayType** const outCodeArray, const size_t capacity)
{
    assert(outCodeArray);

    //--------SET STRUCT CANARY-------

    CodeArrayType* codeArray = (CodeArrayType*) calloc(1, sizeof(*codeArray));

    ON_CANARY
    (
        codeArray->structCanaryLeft  = Canary;
        codeArray->structCanaryRight = Canary;
    )

    CodeArrayErrors errors = CodeArrayErrors::NO_ERR;
    codeArray->size = 0;

    if (capacity > 0) codeArray->capacity = capacity;
    else              codeArray->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    codeArray->data = (CodeArrayValue*) calloc(CodeArrayGetSizeForCalloc(codeArray), sizeof(*codeArray->data));

    if (codeArray->data == nullptr)
    {
        CodeArrayPrintError(CodeArrayErrors::MEMORY_ALLOCATION_ERROR);  
        return CodeArrayErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    CodeArrayDataFill(codeArray);

    ON_CANARY
    (
        codeArray->data = GetAfterFirstCanaryAdr(codeArray);
    )

    CODE_ARRAY_CHECK(codeArray);

    *outCodeArray = codeArray;
    
    return errors;    
}
#endif

CodeArrayErrors CodeArrayDtor(CodeArrayType* const codeArray)
{
    assert(codeArray);

    CODE_ARRAY_CHECK(codeArray);
    
    for (size_t i = 0; i < codeArray->size; ++i)
    {
        codeArray->data[i] = CODE_VALUE_POISON;
    }

    ON_CANARY
    (
        codeArray->data = GetFirstCanaryAdr(codeArray);
    )

    free(codeArray->data);
    codeArray->data = nullptr;

    codeArray->size     = 0;
    codeArray->capacity = 0;

    ON_HASH
    (
        codeArray->dataHash = 0;
    )

    ON_CANARY
    (
        codeArray->structCanaryLeft  = 0;
        codeArray->structCanaryRight = 0;
    )

    return CodeArrayErrors::NO_ERR;
}

CodeArrayErrors CodeArrayPush(CodeArrayType* codeArray, const CodeArrayValue val)
{
    assert(codeArray);
    
    CODE_ARRAY_CHECK(codeArray);

    CodeArrayErrors codeArrayReallocErr = CodeArrayErrors::NO_ERR;
    if (CodeArrayIsFull(codeArray)) codeArrayReallocErr = CodeArrayRealloc(codeArray, true);

    IF_ERR_RETURN(codeArrayReallocErr);

    codeArray->data[codeArray->size++] = val;

    ON_HASH
    (
        UpdateDataHash(codeArray);
        UpdateStructHash(codeArray);
    )

    CODE_ARRAY_CHECK(codeArray);

    return CodeArrayErrors::NO_ERR;
}

CodeArrayErrors CodeArrayFind(const CodeArrayType* table, const CodeArrayValue value, CodeArrayValue** outVal)
{
    assert(table);
    assert(outVal);

    CODE_ARRAY_CHECK(table);

    for (size_t i = 0; i < table->size; ++i)
    {
        if (table->data[i] == value)
        {
            *outVal = table->data + i;
            return CodeArrayErrors::NO_ERR;
        }
    }

    *outVal = nullptr;

    return CodeArrayErrors::NO_ERR;
}

CodeArrayErrors CodeArrayGetPos(const CodeArrayType* table, CodeArrayValue* namePtr, size_t* outPos)
{
    assert(table);
    assert(namePtr);
    assert(outPos);
    assert(namePtr >= table->data);

    *outPos = (size_t)(namePtr - table->data);

    return CodeArrayErrors::NO_ERR;
}

CodeArrayErrors CodeArrayVerify(const CodeArrayType* codeArray)
{
    assert(codeArray);

    if (codeArray->data == nullptr)
    {
        CodeArrayPrintError(CodeArrayErrors::CODE_ARRAY_IS_NULLPTR);
        return CodeArrayErrors::CODE_ARRAY_IS_NULLPTR;
    }

    if (codeArray->capacity <= 0)
    {  
        CodeArrayPrintError(CodeArrayErrors::CAPACITY_OUT_OF_RANGE);
        return CodeArrayErrors::CAPACITY_OUT_OF_RANGE;
    }

    if (codeArray->size > codeArray->capacity)
    {
        CodeArrayPrintError(CodeArrayErrors::SIZE_OUT_OF_RANGE);
        return CodeArrayErrors::SIZE_OUT_OF_RANGE;
    }

    //-----------Canary checking----------

    ON_CANARY
    (
        if (*(CanaryType*)(GetFirstCanaryAdr(codeArray)) != Canary)
        {
            CodeArrayPrintError(CodeArrayErrors::INVALID_CANARY);
            return CodeArrayErrors::INVALID_CANARY;
        }

        if (*(CanaryType*)(GetSecondCanaryAdr(codeArray)) != Canary)
        {
            CodeArrayPrintError(CodeArrayErrors::INVALID_CANARY);
            return CodeArrayErrors::INVALID_CANARY;
        }

        if (codeArray->structCanaryLeft != Canary)
        {
            CodeArrayPrintError(CodeArrayErrors::INVALID_CANARY);
            return CodeArrayErrors::INVALID_CANARY;
        }

        if (codeArray->structCanaryRight != Canary)
        {
            CodeArrayPrintError(CodeArrayErrors::INVALID_CANARY);
            return CodeArrayErrors::INVALID_CANARY;
        }

    )

    //------------Hash checking----------

    ON_HASH
    (
        if (CalcDataHash(codeArray) != codeArray->dataHash)
        {
            CodeArrayPrintError(CodeArrayErrors::INVALID_DATA_HASH);
            return CodeArrayErrors::INVALID_DATA_HASH;
        }
    )

    ON_HASH
    (
        if (CalcDataHash(codeArray) != codeArray->dataHash)
        {
            CodeArrayPrintError(CodeArrayErrors::INVALID_DATA_HASH);
            return CodeArrayErrors::INVALID_DATA_HASH;
        }

        HashType prevStructHash = codeArray->structHash;
        UpdateStructHash(codeArray);

        if (prevStructHash != codeArray->structHash)
        {
            CodeArrayPrintError(CodeArrayErrors::INVALID_STRUCT_HASH);
            codeArray->structHash = prevStructHash;
            return CodeArrayErrors::INVALID_STRUCT_HASH;
        }
    )


    return CodeArrayErrors::NO_ERR;
}

void CodeArrayDump(const CodeArrayType* codeArray, const char* const fileName, 
                                     const char* const funcName,
                                     const int lineNumber)
{
    assert(codeArray);
    assert(fileName);
    assert(funcName);
    assert(lineNumber > 0);
    
    LOG_BEGIN();

    Log("CodeArrayDump was called in file %s, function %s, line %d\n", fileName, funcName, lineNumber);
    Log("codeArray[%p]\n{\n", codeArray);
    Log("\tcodeArray capacity: %zu, \n"
        "\tcodeArray size    : %zu,\n",
        codeArray->capacity, codeArray->size);
    
    ON_CANARY
    (
        Log("\tLeft struct canary : " CanaryTypeFormat ",\n", 
            codeArray->structCanaryLeft);
        Log("\tRight struct canary: " CanaryTypeFormat ",\n", 
            codeArray->structCanaryRight);

        Log("\tLeft data canary : " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetFirstCanaryAdr(codeArray)));
        Log("\tRight data canary: " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetSecondCanaryAdr(codeArray)));
    )

    ON_HASH
    (
        Log("\tData hash  : %llu\n", codeArray->dataHash);
        Log("\tStruct hash: %llu\n", codeArray->structHash);
    )

    Log("\tdata data[%p]\n\t{\n", codeArray->data);

    /*
    if (codeArray->data != nullptr)
    {
        for (size_t i = 0; i < (codeArray->size < codeArray->capacity ? codeArray->size : codeArray->capacity); ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, codeArray->data[i]);

            if (Equal(&codeArray->data[i], &CODE_ARRAY_POISON)) Log(" (CODE_ARRAY_POISON)");

            Log("\n");
        }

        Log("\t\tNot used values:\n");

        for(size_t i = codeArray->size; i < codeArray->capacity; ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, codeArray->data[i]);
            
            //if (Equal(&codeArray->data[i], &CODE_ARRAY_POISON)) Log(" (CODE_ARRAY_POISON)");

            Log("\n");
        }
    }
    */

    Log("\t}\n}\n");

    LOG_END();
}

CodeArrayErrors CodeArrayRealloc(CodeArrayType* codeArray, bool increase)
{
    assert(codeArray);

    CODE_ARRAY_CHECK(codeArray);
    
    if (increase) codeArray->capacity <<= 1;
    else          codeArray->capacity >>= 1;

    if (!increase) 
        FillCodeArray(codeArray->data + codeArray->capacity, codeArray->data + codeArray->size, CODE_VALUE_POISON);

    //--------Moves data to the first canary-------
    ON_CANARY
    (
        codeArray->data = GetFirstCanaryAdr(codeArray);
    )

    CodeArrayValue* tmpCodeArray = (CodeArrayValue*) realloc(codeArray->data, 
                                             CodeArrayGetSizeForCalloc(codeArray) * sizeof(*codeArray->data));

    if (tmpCodeArray == nullptr)
    {
        CodeArrayPrintError(CodeArrayErrors::MEMORY_ALLOCATION_ERROR);

        assert(codeArray);
        if (increase) codeArray->capacity >>= 1;
        else          codeArray->capacity <<= 1;

        CODE_ARRAY_CHECK(codeArray);

        return CodeArrayErrors::NO_ERR;
    }

    codeArray->data = tmpCodeArray;

    // -------Moving forward after reallocing-------
    ON_CANARY
    (
        codeArray->data = GetAfterFirstCanaryAdr(codeArray);
    )

    if (increase)
        FillCodeArray(codeArray->data + codeArray->size, codeArray->data + codeArray->capacity, CODE_VALUE_POISON);

    // -------Putting canary at the end-----------
    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(codeArray));
    )

    ON_HASH
    (
        UpdateDataHash(codeArray);
        UpdateStructHash(codeArray);
    )
    
    CODE_ARRAY_CHECK(codeArray);

    return CodeArrayErrors::NO_ERR;
}

static inline bool CodeArrayIsFull(CodeArrayType* codeArray)
{
    assert(codeArray);

    CODE_ARRAY_CHECK_NO_RETURN(codeArray);

    return codeArray->size >= codeArray->capacity;
}

static inline bool CodeArrayIsTooBig(CodeArrayType* codeArray)
{
    assert(codeArray);

    CODE_ARRAY_CHECK_NO_RETURN(codeArray);

    return (codeArray->size * 4 <= codeArray->capacity) & (codeArray->capacity > STANDARD_CAPACITY);
}

static inline CodeArrayValue* MovePtr(CodeArrayValue* const data, const size_t moveSz, const int times)
{
    assert(data);
    assert(moveSz > 0);
    
    return (CodeArrayValue*)((char*)data + times * (long long)moveSz);
}

// NO codeArray check because doesn't fill hashes
static void CodeArrayDataFill(CodeArrayType* const codeArray)
{
    assert(codeArray);
    assert(codeArray->data);
    assert(codeArray->capacity > 0);

    ON_CANARY
    (
        CanaryCtor(codeArray->data);
        codeArray->data = GetAfterFirstCanaryAdr(codeArray);
    )

    FillCodeArray(codeArray->data, codeArray->data + codeArray->capacity, CODE_VALUE_POISON);

    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(codeArray));
        codeArray->data = GetFirstCanaryAdr(codeArray);
    )

    // No codeArray check because doesn't fill hashes
}

// no CODE_ARRAY_CHECK because can be used for callocing memory (data could be nullptr at this moment)
static inline size_t CodeArrayGetSizeForCalloc(CodeArrayType* const codeArray)
{
    assert(codeArray);
    assert(codeArray->capacity > 0);

    ON_CANARY
    (
        return codeArray->capacity + 3 * sizeof(CanaryType) / sizeof(*codeArray->data);
    )

    return codeArray->capacity;
}

#undef CODE_ARRAY_CHECK
#undef IF_ERR_RETURN
#undef GetAfterFirstCanaryAdr
#undef GetFirstCanaryAdr
#undef GetSecondCanaryAdr

#define LOG_ERR(X) Log(HTML_RED_HEAD_BEGIN "\n" X "\n" HTML_HEAD_END "\n")
void CodeArrayPrintError(CodeArrayErrors error)
{
    LOG_BEGIN();

    switch(error)
    {
        case CodeArrayErrors::CAPACITY_OUT_OF_RANGE:
            LOG_ERR("CodeArray capacity is out of range.\n");
            break;
        case CodeArrayErrors::CODE_ARRAY_IS_NULLPTR:
            LOG_ERR("CodeArray is nullptr.\n");
            break;
        case CodeArrayErrors::CODE_ARRAY_EMPTY_ERR:
            LOG_ERR("Trying to pop from empty codeArray.\n");
            break;
        case CodeArrayErrors::SIZE_OUT_OF_RANGE:
            LOG_ERR("CodeArray size is out of range.\n");
            break;
        case CodeArrayErrors::MEMORY_ALLOCATION_ERROR:
            LOG_ERR("Couldn't allocate more memory for codeArray.\n");
            break;
        case CodeArrayErrors::INVALID_CANARY:
            LOG_ERR("CodeArray canary is invalid.\n");
            break;
        case CodeArrayErrors::INVALID_DATA_HASH:
            LOG_ERR("CodeArray data hash is invalid.\n");
            break;
        case CodeArrayErrors::INVALID_STRUCT_HASH:
            LOG_ERR("CodeArray struct hash is invalid.\n");

        case CodeArrayErrors::NO_ERR:
        default:
            break;
    }

    LOG_END();
}

#undef PRINT_ERR
