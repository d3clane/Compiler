#include <math.h>
#include <execinfo.h> 
#include <string.h>

#include "RodataStringsArrayFuncs.h"
#include "RodataStrings.h"
#include "Common/Log.h"

//----------static functions------------

static RodataStringsErrors RodataStringsRealloc(RodataStringsType* rodataStrings, bool increase);

static inline RodataStringsValue* MovePtr(RodataStringsValue* const data, const size_t moveSz, const int times);

static inline size_t RodataStringsGetSizeForCalloc(RodataStringsType* const rodataStrings);

static void RodataStringsDataFill(RodataStringsType* const rodataStrings);

static inline bool RodataStringsIsFull(RodataStringsType* rodataStrings);

static inline bool RodataStringsIsTooBig(RodataStringsType* rodataStrings);

//--------CANARY PROTECTION----------

#ifdef RODATA_STRINGS_CANARY_PROTECTION

    #define CanaryTypeFormat "%#0llx"
    const CanaryType Canary = 0xDEADBABE;

    const size_t Aligning = 8;

    static inline void CanaryCtor(void* storage)
    {
        assert(storage);
        CanaryType* strg = (CanaryType*) (storage);

        *strg = Canary;
    }

    static inline RodataStringsValue* GetAfterFirstCanaryAdr(const RodataStringsType* const rodataStrings)
    {
        return MovePtr(rodataStrings->data, sizeof(CanaryType), 1);
    }

    static inline RodataStringsValue* GetFirstCanaryAdr(const RodataStringsType* const rodataStrings)
    {
        return MovePtr(rodataStrings->data, sizeof(CanaryType), -1);
    }

    static inline RodataStringsValue* GetSecondCanaryAdr(const RodataStringsType* const rodataStrings)
    {
        return (RodataStringsValue*)((char*)(rodataStrings->data + rodataStrings->capacity) +
                           Aligning - (rodataStrings->capacity * sizeof(RodataStringsValue)) % Aligning);
    }

#endif

//-----------HASH PROTECTION---------------

#ifdef RODATA_STRINGS_HASH_PROTECTION

    static inline HashType CalcDataHash(const RodataStringsType* rodataStrings)
    {
        return rodataStrings->HashFunc(rodataStrings->data, rodataStrings->capacity * sizeof(RodataStringsValue), 0);        
    }

    static inline void UpdateDataHash(RodataStringsType* rodataStrings)
    {
        rodataStrings->dataHash = CalcDataHash(rodataStrings);      
    }

    static inline void UpdateStructHash(RodataStringsType* rodataStrings)
    {
        rodataStrings->structHash = 0;                                  
        rodataStrings->structHash = RodataStringsMurmurHash(rodataStrings, sizeof(*rodataStrings));        
    }
    
#endif

//--------------Consts-----------------

static const size_t STANDARD_CAPACITY = 64;

//---------------

#ifndef NDEBUG

    #define RODATA_STRINGS_CHECK(rodataStrings)                             \
    do                                                              \
    {                                                               \
        RodataStringsErrors rodataStringsErr = RodataStringsVerify(rodataStrings);  \
                                                                    \
        if (rodataStringsErr != RodataStringsErrors::NO_ERR)                \
        {                                                           \
            RODATA_STRINGS_DUMP(rodataStrings);                             \
            return rodataStringsErr;                    \
        }                                       \
    } while (0)

    #define RODATA_STRINGS_CHECK_NO_RETURN(rodataStrings)         \
    do                                         \
    {                                          \
        RodataStringsErrors rodataStringsErr = RodataStringsVerify(rodataStrings);     \
                                                        \
        if (rodataStringsErr != RodataStringsErrors::NO_ERR)   \
        {                                               \
            RODATA_STRINGS_DUMP(rodataStrings);                   \
        }                                      \
    } while (0)

#else

    #define RODATA_STRINGS_CHECK(rodataStrings)           
    #define RODATA_STRINGS_CHECK_NO_RETURN(rodataStrings) 

#endif

//---------------

#define IF_ERR_RETURN(ERR)              \
do                                      \
{                                       \
    if (ERR != RodataStringsErrors::NO_ERR)                            \
        return ERR;                     \
} while (0)

//---------------

#ifdef RODATA_STRINGS_HASH_PROTECTION
RodataStringsErrors RodataStringsCtor(RodataStringsType* const rodataStrings, const size_t capacity, 
                          const HashFuncType HashFunc)
{
    assert(rodataStrings);

    ON_HASH
    (
        rodataStrings->HashFunc = HashFunc;
    )

    //--------SET STRUCT CANARY-------
    ON_CANARY
    (
        rodataStrings->structCanaryLeft  = Canary;
        rodataStrings->structCanaryRight = Canary;
    )

    RodataStringsErrors errors = RodataStringsErrors::NO_ERR;
    rodataStrings->size = 0;

    if (capacity > 0) rodataStrings->capacity = capacity;
    else              rodataStrings->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    rodataStrings->data = (RodataStringsValue*) calloc(RodataStringsGetSizeForCalloc(rodataStrings), sizeof(*rodataStrings->data));

    if (rodataStrings->data == nullptr)
    {
        RodataStringsPrintError(RodataStringsErrors::MEMORY_ALLOCATION_ERROR);  
        return RodataStringsErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    RodataStringsDataFill(rodataStrings);

    ON_CANARY
    (
        rodataStrings->data = GetAfterFirstCanaryAdr(rodataStrings);
    )

    ON_HASH
    (
        UpdateDataHash(rodataStrings);
        UpdateStructHash(rodataStrings);
    )

    RODATA_STRINGS_CHECK(rodataStrings);

    return errors;
}
#else 
RodataStringsErrors RodataStringsCtor(RodataStringsType** const outRodataStrings, const size_t capacity)
{
    assert(outRodataStrings);

    //--------SET STRUCT CANARY-------

    RodataStringsType* rodataStrings = (RodataStringsType*) calloc(1, sizeof(*rodataStrings));

    ON_CANARY
    (
        rodataStrings->structCanaryLeft  = Canary;
        rodataStrings->structCanaryRight = Canary;
    )

    RodataStringsErrors errors = RodataStringsErrors::NO_ERR;
    rodataStrings->size = 0;

    if (capacity > 0) rodataStrings->capacity = capacity;
    else              rodataStrings->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    rodataStrings->data = (RodataStringsValue*) calloc(RodataStringsGetSizeForCalloc(rodataStrings), sizeof(*rodataStrings->data));

    if (rodataStrings->data == nullptr)
    {
        RodataStringsPrintError(RodataStringsErrors::MEMORY_ALLOCATION_ERROR);  
        return RodataStringsErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    RodataStringsDataFill(rodataStrings);

    ON_CANARY
    (
        rodataStrings->data = GetAfterFirstCanaryAdr(rodataStrings);
    )

    RODATA_STRINGS_CHECK(rodataStrings);

    *outRodataStrings = rodataStrings;
    
    return errors;    
}
#endif

RodataStringsErrors RodataStringsDtor(RodataStringsType* const rodataStrings)
{
    assert(rodataStrings);

    RODATA_STRINGS_CHECK(rodataStrings);
    
    for (size_t i = 0; i < rodataStrings->size; ++i)
    {
        free(rodataStrings->data[i].string);
        free(rodataStrings->data[i].label);
        rodataStrings->data[i] = RODATA_STRINGS_POISON;
    }

    ON_CANARY
    (
        rodataStrings->data = GetFirstCanaryAdr(rodataStrings);
    )

    free(rodataStrings->data);
    rodataStrings->data = nullptr;

    rodataStrings->size     = 0;
    rodataStrings->capacity = 0;

    ON_HASH
    (
        rodataStrings->dataHash = 0;
    )

    ON_CANARY
    (
        rodataStrings->structCanaryLeft  = 0;
        rodataStrings->structCanaryRight = 0;
    )

    return RodataStringsErrors::NO_ERR;
}

RodataStringsErrors RodataStringsPush(RodataStringsType* rodataStrings, const RodataStringsValue val)
{
    assert(rodataStrings);
    
    RODATA_STRINGS_CHECK(rodataStrings);

    RodataStringsErrors rodataStringsReallocErr = RodataStringsErrors::NO_ERR;
    if (RodataStringsIsFull(rodataStrings)) rodataStringsReallocErr = RodataStringsRealloc(rodataStrings, true);

    IF_ERR_RETURN(rodataStringsReallocErr);

    rodataStrings->data[rodataStrings->size++] = val;

    ON_HASH
    (
        UpdateDataHash(rodataStrings);
        UpdateStructHash(rodataStrings);
    )

    RODATA_STRINGS_CHECK(rodataStrings);

    return RodataStringsErrors::NO_ERR;
}

RodataStringsErrors RodataStringsFind(const RodataStringsType* table, const char* name, RodataStringsValue** outString)
{
    assert(table);
    assert(outString);

    RODATA_STRINGS_CHECK(table);

    for (size_t i = 0; i < table->size; ++i)
    {
        if (strcmp(table->data[i].string, name) == 0)
        {
            *outString = table->data + i;
            return RodataStringsErrors::NO_ERR;
        }
    }

    *outString = nullptr;

    return RodataStringsErrors::NO_ERR;
}

RodataStringsErrors RodataStringsGetPos(const RodataStringsType* table, RodataStringsValue* namePtr, size_t* outPos)
{
    assert(table);
    assert(namePtr);
    assert(outPos);
    assert(namePtr >= table->data);

    *outPos = (size_t)(namePtr - table->data);

    return RodataStringsErrors::NO_ERR;
}

RodataStringsErrors RodataStringsVerify(const RodataStringsType* rodataStrings)
{
    assert(rodataStrings);

    if (rodataStrings->data == nullptr)
    {
        RodataStringsPrintError(RodataStringsErrors::RODATA_STRINGS_IS_NULLPTR);
        return RodataStringsErrors::RODATA_STRINGS_IS_NULLPTR;
    }

    if (rodataStrings->capacity <= 0)
    {  
        RodataStringsPrintError(RodataStringsErrors::CAPACITY_OUT_OF_RANGE);
        return RodataStringsErrors::CAPACITY_OUT_OF_RANGE;
    }

    if (rodataStrings->size > rodataStrings->capacity)
    {
        RodataStringsPrintError(RodataStringsErrors::SIZE_OUT_OF_RANGE);
        return RodataStringsErrors::SIZE_OUT_OF_RANGE;
    }

    //-----------Canary checking----------

    ON_CANARY
    (
        if (*(CanaryType*)(GetFirstCanaryAdr(rodataStrings)) != Canary)
        {
            RodataStringsPrintError(RodataStringsErrors::INVALID_CANARY);
            return RodataStringsErrors::INVALID_CANARY;
        }

        if (*(CanaryType*)(GetSecondCanaryAdr(rodataStrings)) != Canary)
        {
            RodataStringsPrintError(RodataStringsErrors::INVALID_CANARY);
            return RodataStringsErrors::INVALID_CANARY;
        }

        if (rodataStrings->structCanaryLeft != Canary)
        {
            RodataStringsPrintError(RodataStringsErrors::INVALID_CANARY);
            return RodataStringsErrors::INVALID_CANARY;
        }

        if (rodataStrings->structCanaryRight != Canary)
        {
            RodataStringsPrintError(RodataStringsErrors::INVALID_CANARY);
            return RodataStringsErrors::INVALID_CANARY;
        }

    )

    //------------Hash checking----------

    ON_HASH
    (
        if (CalcDataHash(rodataStrings) != rodataStrings->dataHash)
        {
            RodataStringsPrintError(RodataStringsErrors::INVALID_DATA_HASH);
            return RodataStringsErrors::INVALID_DATA_HASH;
        }
    )

    ON_HASH
    (
        if (CalcDataHash(rodataStrings) != rodataStrings->dataHash)
        {
            RodataStringsPrintError(RodataStringsErrors::INVALID_DATA_HASH);
            return RodataStringsErrors::INVALID_DATA_HASH;
        }

        HashType prevStructHash = rodataStrings->structHash;
        UpdateStructHash(rodataStrings);

        if (prevStructHash != rodataStrings->structHash)
        {
            RodataStringsPrintError(RodataStringsErrors::INVALID_STRUCT_HASH);
            rodataStrings->structHash = prevStructHash;
            return RodataStringsErrors::INVALID_STRUCT_HASH;
        }
    )


    return RodataStringsErrors::NO_ERR;
}

void RodataStringsDump(const RodataStringsType* rodataStrings, const char* const fileName, 
                                     const char* const funcName,
                                     const int lineNumber)
{
    assert(rodataStrings);
    assert(fileName);
    assert(funcName);
    assert(lineNumber > 0);
    
    LOG_BEGIN();

    Log("RodataStringsDump was called in file %s, function %s, line %d\n", fileName, funcName, lineNumber);
    Log("rodataStrings[%p]\n{\n", rodataStrings);
    Log("\trodataStrings capacity: %zu, \n"
        "\trodataStrings size    : %zu,\n",
        rodataStrings->capacity, rodataStrings->size);
    
    ON_CANARY
    (
        Log("\tLeft struct canary : " CanaryTypeFormat ",\n", 
            rodataStrings->structCanaryLeft);
        Log("\tRight struct canary: " CanaryTypeFormat ",\n", 
            rodataStrings->structCanaryRight);

        Log("\tLeft data canary : " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetFirstCanaryAdr(rodataStrings)));
        Log("\tRight data canary: " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetSecondCanaryAdr(rodataStrings)));
    )

    ON_HASH
    (
        Log("\tData hash  : %llu\n", rodataStrings->dataHash);
        Log("\tStruct hash: %llu\n", rodataStrings->structHash);
    )

    Log("\tdata data[%p]\n\t{\n", rodataStrings->data);

    /*
    if (rodataStrings->data != nullptr)
    {
        for (size_t i = 0; i < (rodataStrings->size < rodataStrings->capacity ? rodataStrings->size : rodataStrings->capacity); ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, rodataStrings->data[i]);

            if (Equal(&rodataStrings->data[i], &RODATA_STRINGS_POISON)) Log(" (RODATA_STRINGS_POISON)");

            Log("\n");
        }

        Log("\t\tNot used values:\n");

        for(size_t i = rodataStrings->size; i < rodataStrings->capacity; ++i)
        {
            //Log("\t\t*[%zu] = " NameFormat, i, rodataStrings->data[i]);
            
            //if (Equal(&rodataStrings->data[i], &RODATA_STRINGS_POISON)) Log(" (RODATA_STRINGS_POISON)");

            Log("\n");
        }
    }
    */

    Log("\t}\n}\n");

    LOG_END();
}

RodataStringsErrors RodataStringsRealloc(RodataStringsType* rodataStrings, bool increase)
{
    assert(rodataStrings);

    RODATA_STRINGS_CHECK(rodataStrings);
    
    if (increase) rodataStrings->capacity <<= 1;
    else          rodataStrings->capacity >>= 1;

    if (!increase) 
        FillRodataStrings(rodataStrings->data + rodataStrings->capacity, rodataStrings->data + rodataStrings->size, RODATA_STRINGS_POISON);

    //--------Moves data to the first canary-------
    ON_CANARY
    (
        rodataStrings->data = GetFirstCanaryAdr(rodataStrings);
    )

    RodataStringsValue* tmpRodataStrings = (RodataStringsValue*) realloc(rodataStrings->data, 
                                             RodataStringsGetSizeForCalloc(rodataStrings) * sizeof(*rodataStrings->data));

    if (tmpRodataStrings == nullptr)
    {
        RodataStringsPrintError(RodataStringsErrors::MEMORY_ALLOCATION_ERROR);

        assert(rodataStrings);
        if (increase) rodataStrings->capacity >>= 1;
        else          rodataStrings->capacity <<= 1;

        RODATA_STRINGS_CHECK(rodataStrings);

        return RodataStringsErrors::NO_ERR;
    }

    rodataStrings->data = tmpRodataStrings;

    // -------Moving forward after reallocing-------
    ON_CANARY
    (
        rodataStrings->data = GetAfterFirstCanaryAdr(rodataStrings);
    )

    if (increase)
        FillRodataStrings(rodataStrings->data + rodataStrings->size, rodataStrings->data + rodataStrings->capacity, RODATA_STRINGS_POISON);

    // -------Putting canary at the end-----------
    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(rodataStrings));
    )

    ON_HASH
    (
        UpdateDataHash(rodataStrings);
        UpdateStructHash(rodataStrings);
    )
    
    RODATA_STRINGS_CHECK(rodataStrings);

    return RodataStringsErrors::NO_ERR;
}

static inline bool RodataStringsIsFull(RodataStringsType* rodataStrings)
{
    assert(rodataStrings);

    RODATA_STRINGS_CHECK_NO_RETURN(rodataStrings);

    return rodataStrings->size >= rodataStrings->capacity;
}

static inline bool RodataStringsIsTooBig(RodataStringsType* rodataStrings)
{
    assert(rodataStrings);

    RODATA_STRINGS_CHECK_NO_RETURN(rodataStrings);

    return (rodataStrings->size * 4 <= rodataStrings->capacity) & (rodataStrings->capacity > STANDARD_CAPACITY);
}

static inline RodataStringsValue* MovePtr(RodataStringsValue* const data, const size_t moveSz, const int times)
{
    assert(data);
    assert(moveSz > 0);
    
    return (RodataStringsValue*)((char*)data + times * (long long)moveSz);
}

// NO rodataStrings check because doesn't fill hashes
static void RodataStringsDataFill(RodataStringsType* const rodataStrings)
{
    assert(rodataStrings);
    assert(rodataStrings->data);
    assert(rodataStrings->capacity > 0);

    ON_CANARY
    (
        CanaryCtor(rodataStrings->data);
        rodataStrings->data = GetAfterFirstCanaryAdr(rodataStrings);
    )

    FillRodataStrings(rodataStrings->data, rodataStrings->data + rodataStrings->capacity, RODATA_STRINGS_POISON);

    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(rodataStrings));
        rodataStrings->data = GetFirstCanaryAdr(rodataStrings);
    )

    // No rodataStrings check because doesn't fill hashes
}

// no RODATA_STRINGS_CHECK because can be used for callocing memory (data could be nullptr at this moment)
static inline size_t RodataStringsGetSizeForCalloc(RodataStringsType* const rodataStrings)
{
    assert(rodataStrings);
    assert(rodataStrings->capacity > 0);

    ON_CANARY
    (
        return rodataStrings->capacity + 3 * sizeof(CanaryType) / sizeof(*rodataStrings->data);
    )

    return rodataStrings->capacity;
}

#undef RODATA_STRINGS_CHECK
#undef IF_ERR_RETURN
#undef GetAfterFirstCanaryAdr
#undef GetFirstCanaryAdr
#undef GetSecondCanaryAdr

#define LOG_ERR(X) Log(HTML_RED_HEAD_BEGIN "\n" X "\n" HTML_HEAD_END "\n")
void RodataStringsPrintError(RodataStringsErrors error)
{
    LOG_BEGIN();

    switch(error)
    {
        case RodataStringsErrors::CAPACITY_OUT_OF_RANGE:
            LOG_ERR("RodataStrings capacity is out of range.\n");
            break;
        case RodataStringsErrors::RODATA_STRINGS_IS_NULLPTR:
            LOG_ERR("RodataStrings is nullptr.\n");
            break;
        case RodataStringsErrors::RODATA_STRINGS_EMPTY_ERR:
            LOG_ERR("Trying to pop from empty rodataStrings.\n");
            break;
        case RodataStringsErrors::SIZE_OUT_OF_RANGE:
            LOG_ERR("RodataStrings size is out of range.\n");
            break;
        case RodataStringsErrors::MEMORY_ALLOCATION_ERROR:
            LOG_ERR("Couldn't allocate more memory for rodataStrings.\n");
            break;
        case RodataStringsErrors::INVALID_CANARY:
            LOG_ERR("RodataStrings canary is invalid.\n");
            break;
        case RodataStringsErrors::INVALID_DATA_HASH:
            LOG_ERR("RodataStrings data hash is invalid.\n");
            break;
        case RodataStringsErrors::INVALID_STRUCT_HASH:
            LOG_ERR("RodataStrings struct hash is invalid.\n");

        case RodataStringsErrors::NO_ERR:
        default:
            break;
    }

    LOG_END();
}

#undef PRINT_ERR

void RodataStringsValueCtor(RodataStringsValue* value, const char* string, const char* label)
{
    value->string = strdup(string);
    value->label  = strdup(label);
}
