#include <math.h>
#include <execinfo.h> 
#include <string.h>

#include "RodataImmediatesArrayFuncs.h"
#include "RodataImmediates.h"
#include "Common/Log.h"

//----------static functions------------

static RodataImmediatesErrors RodataImmediatesRealloc(RodataImmediatesType* rodataImmediates, bool increase);

static inline RodataImmediatesValue* MovePtr(RodataImmediatesValue* const data, const size_t moveSz, const int times);

static inline size_t RodataImmediatesGetSizeForCalloc(RodataImmediatesType* const rodataImmediates);

static void RodataImmediatesDataFill(RodataImmediatesType* const rodataImmediates);

static inline bool RodataImmediatesIsFull(RodataImmediatesType* rodataImmediates);

static inline bool RodataImmediatesIsTooBig(RodataImmediatesType* rodataImmediates);

//--------CANARY PROTECTION----------

#ifdef RODATA_IMMEDIATES_CANARY_PROTECTION

    #define CanaryTypeFormat "%#0llx"
    const CanaryType Canary = 0xDEADBABE;

    const size_t Aligning = 8;

    static inline void CanaryCtor(void* storage)
    {
        assert(storage);
        CanaryType* strg = (CanaryType*) (storage);

        *strg = Canary;
    }

    static inline RodataImmediatesValue* GetAfterFirstCanaryAdr(const RodataImmediatesType* const rodataImmediates)
    {
        return MovePtr(rodataImmediates->data, sizeof(CanaryType), 1);
    }

    static inline RodataImmediatesValue* GetFirstCanaryAdr(const RodataImmediatesType* const rodataImmediates)
    {
        return MovePtr(rodataImmediates->data, sizeof(CanaryType), -1);
    }

    static inline RodataImmediatesValue* GetSecondCanaryAdr(const RodataImmediatesType* const rodataImmediates)
    {
        return (RodataImmediatesValue*)((char*)(rodataImmediates->data + rodataImmediates->capacity) +
                           Aligning - (rodataImmediates->capacity * sizeof(RodataImmediatesValue)) % Aligning);
    }

#endif

//-----------HASH PROTECTION---------------

#ifdef RODATA_IMMEDIATES_HASH_PROTECTION

    static inline HashType CalcDataHash(const RodataImmediatesType* rodataImmediates)
    {
        return rodataImmediates->HashFunc(rodataImmediates->data, rodataImmediates->capacity * sizeof(RodataImmediatesValue), 0);        
    }

    static inline void UpdateDataHash(RodataImmediatesType* rodataImmediates)
    {
        rodataImmediates->dataHash = CalcDataHash(rodataImmediates);      
    }

    static inline void UpdateStructHash(RodataImmediatesType* rodataImmediates)
    {
        rodataImmediates->structHash = 0;                                  
        rodataImmediates->structHash = RodataImmediatesMurmurHash(rodataImmediates, sizeof(*rodataImmediates));        
    }
    
#endif

//--------------Consts-----------------

static const size_t STANDARD_CAPACITY = 64;

//---------------

#ifndef NDEBUG

    #define RODATA_IMMEDIATES_CHECK(rodataImmediates)                             \
    do                                                              \
    {                                                               \
        RodataImmediatesErrors rodataImmediatesErr = RodataImmediatesVerify(rodataImmediates);  \
                                                                    \
        if (rodataImmediatesErr != RodataImmediatesErrors::NO_ERR)                \
        {                                                           \
            RODATA_IMMEDIATES_DUMP(rodataImmediates);                             \
            return rodataImmediatesErr;                    \
        }                                       \
    } while (0)

    #define RODATA_IMMEDIATES_CHECK_NO_RETURN(rodataImmediates)         \
    do                                         \
    {                                          \
        RodataImmediatesErrors rodataImmediatesErr = RodataImmediatesVerify(rodataImmediates);     \
                                                        \
        if (rodataImmediatesErr != RodataImmediatesErrors::NO_ERR)   \
        {                                               \
            RODATA_IMMEDIATES_DUMP(rodataImmediates);                   \
        }                                      \
    } while (0)

#else

    #define RODATA_IMMEDIATES_CHECK(rodataImmediates)           
    #define RODATA_IMMEDIATES_CHECK_NO_RETURN(rodataImmediates) 

#endif

//---------------

#define IF_ERR_RETURN(ERR)              \
do                                      \
{                                       \
    if (ERR != RodataImmediatesErrors::NO_ERR)                            \
        return ERR;                     \
} while (0)

//---------------

#ifdef RODATA_IMMEDIATES_HASH_PROTECTION
RodataImmediatesErrors RodataImmediatesCtor(RodataImmediatesType* const rodataImmediates, const size_t capacity, 
                          const HashFuncType HashFunc)
{
    assert(rodataImmediates);

    ON_HASH
    (
        rodataImmediates->HashFunc = HashFunc;
    )

    //--------SET STRUCT CANARY-------
    ON_CANARY
    (
        rodataImmediates->structCanaryLeft  = Canary;
        rodataImmediates->structCanaryRight = Canary;
    )

    RodataImmediatesErrors errors = RodataImmediatesErrors::NO_ERR;
    rodataImmediates->size = 0;

    if (capacity > 0) rodataImmediates->capacity = capacity;
    else              rodataImmediates->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    rodataImmediates->data = (RodataImmediatesValue*) calloc(RodataImmediatesGetSizeForCalloc(rodataImmediates), sizeof(*rodataImmediates->data));

    if (rodataImmediates->data == nullptr)
    {
        RodataImmediatesPrintError(RodataImmediatesErrors::MEMORY_ALLOCATION_ERROR);  
        return RodataImmediatesErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    RodataImmediatesDataFill(rodataImmediates);

    ON_CANARY
    (
        rodataImmediates->data = GetAfterFirstCanaryAdr(rodataImmediates);
    )

    ON_HASH
    (
        UpdateDataHash(rodataImmediates);
        UpdateStructHash(rodataImmediates);
    )

    RODATA_IMMEDIATES_CHECK(rodataImmediates);

    return errors;
}
#else 
RodataImmediatesErrors RodataImmediatesCtor(RodataImmediatesType** const outRodataImmediates, const size_t capacity)
{
    assert(outRodataImmediates);

    //--------SET STRUCT CANARY-------

    RodataImmediatesType* rodataImmediates = (RodataImmediatesType*) calloc(1, sizeof(*rodataImmediates));

    ON_CANARY
    (
        rodataImmediates->structCanaryLeft  = Canary;
        rodataImmediates->structCanaryRight = Canary;
    )

    RodataImmediatesErrors errors = RodataImmediatesErrors::NO_ERR;
    rodataImmediates->size = 0;

    if (capacity > 0) rodataImmediates->capacity = capacity;
    else              rodataImmediates->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    rodataImmediates->data = (RodataImmediatesValue*) calloc(RodataImmediatesGetSizeForCalloc(rodataImmediates), sizeof(*rodataImmediates->data));

    if (rodataImmediates->data == nullptr)
    {
        RodataImmediatesPrintError(RodataImmediatesErrors::MEMORY_ALLOCATION_ERROR);  
        return RodataImmediatesErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    RodataImmediatesDataFill(rodataImmediates);

    ON_CANARY
    (
        rodataImmediates->data = GetAfterFirstCanaryAdr(rodataImmediates);
    )

    RODATA_IMMEDIATES_CHECK(rodataImmediates);

    *outRodataImmediates = rodataImmediates;
    
    return errors;    
}
#endif

RodataImmediatesErrors RodataImmediatesDtor(RodataImmediatesType* const rodataImmediates)
{
    assert(rodataImmediates);

    RODATA_IMMEDIATES_CHECK(rodataImmediates);
    
    for (size_t i = 0; i < rodataImmediates->size; ++i)
    {
        free(rodataImmediates->data[i].label);
        rodataImmediates->data[i] = RODATA_IMMEDIATES_POISON;
    }

    ON_CANARY
    (
        rodataImmediates->data = GetFirstCanaryAdr(rodataImmediates);
    )

    free(rodataImmediates->data);
    rodataImmediates->data = nullptr;

    rodataImmediates->size     = 0;
    rodataImmediates->capacity = 0;

    ON_HASH
    (
        rodataImmediates->dataHash = 0;
    )

    ON_CANARY
    (
        rodataImmediates->structCanaryLeft  = 0;
        rodataImmediates->structCanaryRight = 0;
    )

    return RodataImmediatesErrors::NO_ERR;
}

RodataImmediatesErrors RodataImmediatesPush(RodataImmediatesType* rodataImmediates, const RodataImmediatesValue val)
{
    assert(rodataImmediates);
    
    RODATA_IMMEDIATES_CHECK(rodataImmediates);

    RodataImmediatesErrors rodataImmediatesReallocErr = RodataImmediatesErrors::NO_ERR;
    if (RodataImmediatesIsFull(rodataImmediates)) rodataImmediatesReallocErr = RodataImmediatesRealloc(rodataImmediates, true);

    IF_ERR_RETURN(rodataImmediatesReallocErr);

    rodataImmediates->data[rodataImmediates->size++] = val;

    ON_HASH
    (
        UpdateDataHash(rodataImmediates);
        UpdateStructHash(rodataImmediates);
    )

    RODATA_IMMEDIATES_CHECK(rodataImmediates);

    return RodataImmediatesErrors::NO_ERR;
}

RodataImmediatesErrors RodataImmediatesFind(const RodataImmediatesType* table, const long long imm, RodataImmediatesValue** outImmediate)
{
    assert(table);
    assert(outImmediate);

    RODATA_IMMEDIATES_CHECK(table);

    for (size_t i = 0; i < table->size; ++i)
    {
        if (table->data[i].imm == imm)
        {
            *outImmediate = table->data + i;
            return RodataImmediatesErrors::NO_ERR;
        }
    }

    *outImmediate = nullptr;

    return RodataImmediatesErrors::NO_ERR;
}

RodataImmediatesErrors RodataImmediatesGetPos(const RodataImmediatesType* table, RodataImmediatesValue* namePtr, size_t* outPos)
{
    assert(table);
    assert(namePtr);
    assert(outPos);
    assert(namePtr >= table->data);

    *outPos = (size_t)(namePtr - table->data);

    return RodataImmediatesErrors::NO_ERR;
}

RodataImmediatesErrors RodataImmediatesVerify(const RodataImmediatesType* rodataImmediates)
{
    assert(rodataImmediates);

    if (rodataImmediates->data == nullptr)
    {
        RodataImmediatesPrintError(RodataImmediatesErrors::RODATA_IMMEDIATES_IS_NULLPTR);
        return RodataImmediatesErrors::RODATA_IMMEDIATES_IS_NULLPTR;
    }

    if (rodataImmediates->capacity <= 0)
    {  
        RodataImmediatesPrintError(RodataImmediatesErrors::CAPACITY_OUT_OF_RANGE);
        return RodataImmediatesErrors::CAPACITY_OUT_OF_RANGE;
    }

    if (rodataImmediates->size > rodataImmediates->capacity)
    {
        RodataImmediatesPrintError(RodataImmediatesErrors::SIZE_OUT_OF_RANGE);
        return RodataImmediatesErrors::SIZE_OUT_OF_RANGE;
    }

    //-----------Canary checking----------

    ON_CANARY
    (
        if (*(CanaryType*)(GetFirstCanaryAdr(rodataImmediates)) != Canary)
        {
            RodataImmediatesPrintError(RodataImmediatesErrors::INVALID_CANARY);
            return RodataImmediatesErrors::INVALID_CANARY;
        }

        if (*(CanaryType*)(GetSecondCanaryAdr(rodataImmediates)) != Canary)
        {
            RodataImmediatesPrintError(RodataImmediatesErrors::INVALID_CANARY);
            return RodataImmediatesErrors::INVALID_CANARY;
        }

        if (rodataImmediates->structCanaryLeft != Canary)
        {
            RodataImmediatesPrintError(RodataImmediatesErrors::INVALID_CANARY);
            return RodataImmediatesErrors::INVALID_CANARY;
        }

        if (rodataImmediates->structCanaryRight != Canary)
        {
            RodataImmediatesPrintError(RodataImmediatesErrors::INVALID_CANARY);
            return RodataImmediatesErrors::INVALID_CANARY;
        }

    )

    //------------Hash checking----------

    ON_HASH
    (
        if (CalcDataHash(rodataImmediates) != rodataImmediates->dataHash)
        {
            RodataImmediatesPrintError(RodataImmediatesErrors::INVALID_DATA_HASH);
            return RodataImmediatesErrors::INVALID_DATA_HASH;
        }
    )

    ON_HASH
    (
        if (CalcDataHash(rodataImmediates) != rodataImmediates->dataHash)
        {
            RodataImmediatesPrintError(RodataImmediatesErrors::INVALID_DATA_HASH);
            return RodataImmediatesErrors::INVALID_DATA_HASH;
        }

        HashType prevStructHash = rodataImmediates->structHash;
        UpdateStructHash(rodataImmediates);

        if (prevStructHash != rodataImmediates->structHash)
        {
            RodataImmediatesPrintError(RodataImmediatesErrors::INVALID_STRUCT_HASH);
            rodataImmediates->structHash = prevStructHash;
            return RodataImmediatesErrors::INVALID_STRUCT_HASH;
        }
    )


    return RodataImmediatesErrors::NO_ERR;
}

void RodataImmediatesDump(const RodataImmediatesType* rodataImmediates, const char* const fileName, 
                                     const char* const funcName,
                                     const int lineNumber)
{
    assert(rodataImmediates);
    assert(fileName);
    assert(funcName);
    assert(lineNumber > 0);
    
    LOG_BEGIN();

    Log("RodataImmediatesDump was called in file %s, function %s, line %d\n", fileName, funcName, lineNumber);
    Log("rodataImmediates[%p]\n{\n", rodataImmediates);
    Log("\trodataImmediates capacity: %zu, \n"
        "\trodataImmediates size    : %zu,\n",
        rodataImmediates->capacity, rodataImmediates->size);
    
    ON_CANARY
    (
        Log("\tLeft struct canary : " CanaryTypeFormat ",\n", 
            rodataImmediates->structCanaryLeft);
        Log("\tRight struct canary: " CanaryTypeFormat ",\n", 
            rodataImmediates->structCanaryRight);

        Log("\tLeft data canary : " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetFirstCanaryAdr(rodataImmediates)));
        Log("\tRight data canary: " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetSecondCanaryAdr(rodataImmediates)));
    )

    ON_HASH
    (
        Log("\tData hash  : %llu\n", rodataImmediates->dataHash);
        Log("\tStruct hash: %llu\n", rodataImmediates->structHash);
    )

    Log("\tdata data[%p]\n\t{\n", rodataImmediates->data);

    /*
    if (rodataImmediates->data != nullptr)
    {
        for (size_t i = 0; i < (rodataImmediates->size < rodataImmediates->capacity ? rodataImmediates->size : rodataImmediates->capacity); ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, rodataImmediates->data[i]);

            if (Equal(&rodataImmediates->data[i], &RODATA_IMMEDIATES_POISON)) Log(" (RODATA_IMMEDIATES_POISON)");

            Log("\n");
        }

        Log("\t\tNot used values:\n");

        for(size_t i = rodataImmediates->size; i < rodataImmediates->capacity; ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, rodataImmediates->data[i]);
            
            //if (Equal(&rodataImmediates->data[i], &RODATA_IMMEDIATES_POISON)) Log(" (RODATA_IMMEDIATES_POISON)");

            Log("\n");
        }
    }
    */

    Log("\t}\n}\n");

    LOG_END();
}

RodataImmediatesErrors RodataImmediatesRealloc(RodataImmediatesType* rodataImmediates, bool increase)
{
    assert(rodataImmediates);

    RODATA_IMMEDIATES_CHECK(rodataImmediates);
    
    if (increase) rodataImmediates->capacity <<= 1;
    else          rodataImmediates->capacity >>= 1;

    if (!increase) 
        FillRodataImmediates(rodataImmediates->data + rodataImmediates->capacity, rodataImmediates->data + rodataImmediates->size, RODATA_IMMEDIATES_POISON);

    //--------Moves data to the first canary-------
    ON_CANARY
    (
        rodataImmediates->data = GetFirstCanaryAdr(rodataImmediates);
    )

    RodataImmediatesValue* tmpRodataImmediates = (RodataImmediatesValue*) realloc(rodataImmediates->data, 
                                             RodataImmediatesGetSizeForCalloc(rodataImmediates) * sizeof(*rodataImmediates->data));

    if (tmpRodataImmediates == nullptr)
    {
        RodataImmediatesPrintError(RodataImmediatesErrors::MEMORY_ALLOCATION_ERROR);

        assert(rodataImmediates);
        if (increase) rodataImmediates->capacity >>= 1;
        else          rodataImmediates->capacity <<= 1;

        RODATA_IMMEDIATES_CHECK(rodataImmediates);

        return RodataImmediatesErrors::NO_ERR;
    }

    rodataImmediates->data = tmpRodataImmediates;

    // -------Moving forward after reallocing-------
    ON_CANARY
    (
        rodataImmediates->data = GetAfterFirstCanaryAdr(rodataImmediates);
    )

    if (increase)
        FillRodataImmediates(rodataImmediates->data + rodataImmediates->size, rodataImmediates->data + rodataImmediates->capacity, RODATA_IMMEDIATES_POISON);

    // -------Putting canary at the end-----------
    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(rodataImmediates));
    )

    ON_HASH
    (
        UpdateDataHash(rodataImmediates);
        UpdateStructHash(rodataImmediates);
    )
    
    RODATA_IMMEDIATES_CHECK(rodataImmediates);

    return RodataImmediatesErrors::NO_ERR;
}

static inline bool RodataImmediatesIsFull(RodataImmediatesType* rodataImmediates)
{
    assert(rodataImmediates);

    RODATA_IMMEDIATES_CHECK_NO_RETURN(rodataImmediates);

    return rodataImmediates->size >= rodataImmediates->capacity;
}

static inline bool RodataImmediatesIsTooBig(RodataImmediatesType* rodataImmediates)
{
    assert(rodataImmediates);

    RODATA_IMMEDIATES_CHECK_NO_RETURN(rodataImmediates);

    return (rodataImmediates->size * 4 <= rodataImmediates->capacity) & (rodataImmediates->capacity > STANDARD_CAPACITY);
}

static inline RodataImmediatesValue* MovePtr(RodataImmediatesValue* const data, const size_t moveSz, const int times)
{
    assert(data);
    assert(moveSz > 0);
    
    return (RodataImmediatesValue*)((char*)data + times * (long long)moveSz);
}

// NO rodataImmediates check because doesn't fill hashes
static void RodataImmediatesDataFill(RodataImmediatesType* const rodataImmediates)
{
    assert(rodataImmediates);
    assert(rodataImmediates->data);
    assert(rodataImmediates->capacity > 0);

    ON_CANARY
    (
        CanaryCtor(rodataImmediates->data);
        rodataImmediates->data = GetAfterFirstCanaryAdr(rodataImmediates);
    )

    FillRodataImmediates(rodataImmediates->data, rodataImmediates->data + rodataImmediates->capacity, RODATA_IMMEDIATES_POISON);

    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(rodataImmediates));
        rodataImmediates->data = GetFirstCanaryAdr(rodataImmediates);
    )

    // No rodataImmediates check because doesn't fill hashes
}

// no RODATA_IMMEDIATES_CHECK because can be used for callocing memory (data could be nullptr at this moment)
static inline size_t RodataImmediatesGetSizeForCalloc(RodataImmediatesType* const rodataImmediates)
{
    assert(rodataImmediates);
    assert(rodataImmediates->capacity > 0);

    ON_CANARY
    (
        return rodataImmediates->capacity + 3 * sizeof(CanaryType) / sizeof(*rodataImmediates->data);
    )

    return rodataImmediates->capacity;
}

#undef RODATA_IMMEDIATES_CHECK
#undef IF_ERR_RETURN
#undef GetAfterFirstCanaryAdr
#undef GetFirstCanaryAdr
#undef GetSecondCanaryAdr

#define LOG_ERR(X) Log(HTML_RED_HEAD_BEGIN "\n" X "\n" HTML_HEAD_END "\n")
void RodataImmediatesPrintError(RodataImmediatesErrors error)
{
    LOG_BEGIN();

    switch(error)
    {
        case RodataImmediatesErrors::CAPACITY_OUT_OF_RANGE:
            LOG_ERR("RodataImmediates capacity is out of range.\n");
            break;
        case RodataImmediatesErrors::RODATA_IMMEDIATES_IS_NULLPTR:
            LOG_ERR("RodataImmediates is nullptr.\n");
            break;
        case RodataImmediatesErrors::RODATA_IMMEDIATES_EMPTY_ERR:
            LOG_ERR("Trying to pop from empty rodataImmediates.\n");
            break;
        case RodataImmediatesErrors::SIZE_OUT_OF_RANGE:
            LOG_ERR("RodataImmediates size is out of range.\n");
            break;
        case RodataImmediatesErrors::MEMORY_ALLOCATION_ERROR:
            LOG_ERR("Couldn't allocate more memory for rodataImmediates.\n");
            break;
        case RodataImmediatesErrors::INVALID_CANARY:
            LOG_ERR("RodataImmediates canary is invalid.\n");
            break;
        case RodataImmediatesErrors::INVALID_DATA_HASH:
            LOG_ERR("RodataImmediates data hash is invalid.\n");
            break;
        case RodataImmediatesErrors::INVALID_STRUCT_HASH:
            LOG_ERR("RodataImmediates struct hash is invalid.\n");

        case RodataImmediatesErrors::NO_ERR:
        default:
            break;
    }

    LOG_END();
}

#undef PRINT_ERR

void RodataImmediatesValueCtor(RodataImmediatesValue* value, const long long imm, const char* label)
{
    value->imm   = imm;
    value->label = strdup(label);
}
