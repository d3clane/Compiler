#include <math.h>
#include <execinfo.h> 
#include <string.h>

#include "ArrayFuncs.h"
#include "TokensArr.h"
#include "Common/Log.h"
#include "../../Common/Log.h"

//----------static functions------------

static TokensArrErrors TokensArrRealloc(TokensArr* tokensArr, bool increase);

static inline ElemType* MovePtr(ElemType* const data, const size_t moveSz, const int times);

static inline size_t TokensArrGetSizeForCalloc(TokensArr* const tokensArr);

static void TokensArrDataFill(TokensArr* const tokensArr);

static inline bool TokensArrIsFull(TokensArr* tokensArr);

static inline bool TokensArrIsTooBig(TokensArr* tokensArr);

//--------CANARY PROTECTION----------

#ifdef TOKENS_ARR_CANARY_PROTECTION

    #define CanaryTypeFormat "%#0llx"
    const CanaryType Canary = 0xDEADBABE;

    const size_t Aligning = 8;

    static inline void CanaryCtor(void* storage)
    {
        assert(storage);
        CanaryType* strg = (CanaryType*) (storage);

        *strg = Canary;
    }

    static inline ElemType* GetAfterFirstCanaryAdr(const TokensArr* const tokensArr)
    {
        return MovePtr(tokensArr->data, sizeof(CanaryType), 1);
    }

    static inline ElemType* GetFirstCanaryAdr(const TokensArr* const tokensArr)
    {
        return MovePtr(tokensArr->data, sizeof(CanaryType), -1);
    }

    static inline ElemType* GetSecondCanaryAdr(const TokensArr* const tokensArr)
    {
        return (ElemType*)((char*)(tokensArr->data + tokensArr->capacity) +
                           Aligning - (tokensArr->capacity * sizeof(ElemType)) % Aligning);
    }

#endif

//-----------HASH PROTECTION---------------

#ifdef TOKENS_ARR_HASH_PROTECTION

    static inline HashType CalcDataHash(const TokensArr* tokensArr)
    {
        return tokensArr->HashFunc(tokensArr->data, tokensArr->capacity * sizeof(ElemType), 0);        
    }

    static inline void UpdateDataHash(TokensArr* tokensArr)
    {
        tokensArr->dataHash = CalcDataHash(tokensArr);      
    }

    static inline void UpdateStructHash(TokensArr* tokensArr)
    {
        tokensArr->structHash = 0;                                  
        tokensArr->structHash = TokensArrMurmurHash(tokensArr, sizeof(*tokensArr));        
    }
    
#endif

//--------------Consts-----------------

static const size_t STANDARD_CAPACITY = 64;

//---------------

#ifndef NDEBUG

    #define TOKENS_ARR_CHECK(tokensArr)                    \
    do                                          \
    {                                           \
        TokensArrErrors tokensArrErr = TokensArrVerify(tokensArr); \
                                                \
        if (tokensArrErr != TokensArrErrors::NO_ERR)                      \
        {                                       \
            TOKENS_ARR_DUMP(tokensArr);                    \
            return tokensArrErr;                    \
        }                                       \
    } while (0)

    #define TOKENS_ARR_CHECK_NO_RETURN(tokensArr)         \
    do                                         \
    {                                          \
        TokensArrErrors tokensArrErr = TokensArrVerify(tokensArr);     \
                                                        \
        if (tokensArrErr != TokensArrErrors::NO_ERR)   \
        {                                               \
            TOKENS_ARR_DUMP(tokensArr);                   \
        }                                      \
    } while (0)

#else

    #define TOKENS_ARR_CHECK(tokensArr)           
    #define TOKENS_ARR_CHECK_NO_RETURN(tokensArr) 

#endif

//---------------

#define IF_ERR_RETURN(ERR)              \
do                                      \
{                                       \
    if (ERR != TokensArrErrors::NO_ERR)                            \
        return ERR;                     \
} while (0)

//---------------

#ifdef TOKENS_ARR_HASH_PROTECTION
TokensArrErrors TokensArrCtor(TokensArr* const tokensArr, const size_t capacity, 
                          const HashFuncType HashFunc)
{
    assert(tokensArr);

    ON_HASH
    (
        tokensArr->HashFunc = HashFunc;
    )

    //--------SET STRUCT CANARY-------
    ON_CANARY
    (
        tokensArr->structCanaryLeft  = Canary;
        tokensArr->structCanaryRight = Canary;
    )

    TokensArrErrors errors = TokensArrErrors::NO_ERR;
    tokensArr->size = 0;

    if (capacity > 0) tokensArr->capacity = capacity;
    else              tokensArr->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    tokensArr->data = (ElemType*) calloc(TokensArrGetSizeForCalloc(tokensArr), sizeof(*tokensArr->data));

    if (tokensArr->data == nullptr)
    {
        TokensArrPrintError(TokensArrErrors::MEMORY_ALLOCATION_ERROR);  
        return TokensArrErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    TokensArrDataFill(tokensArr);

    ON_CANARY
    (
        tokensArr->data = GetAfterFirstCanaryAdr(tokensArr);
    )

    ON_HASH
    (
        UpdateDataHash(tokensArr);
        UpdateStructHash(tokensArr);
    )

    TOKENS_ARR_CHECK(tokensArr);

    return errors;
}
#else 
TokensArrErrors TokensArrCtor(TokensArr* const tokensArr, const size_t capacity)
{
    assert(tokensArr);

    //--------SET STRUCT CANARY-------
    ON_CANARY
    (
        tokensArr->structCanaryLeft  = Canary;
        tokensArr->structCanaryRight = Canary;
    )

    TokensArrErrors errors = TokensArrErrors::NO_ERR;
    tokensArr->size = 0;

    if (capacity > 0) tokensArr->capacity = capacity;
    else              tokensArr->capacity = STANDARD_CAPACITY;

    //----Callocing Memory-------

    tokensArr->data = (ElemType*) calloc(TokensArrGetSizeForCalloc(tokensArr), sizeof(*tokensArr->data));

    if (tokensArr->data == nullptr)
    {
        TokensArrPrintError(TokensArrErrors::MEMORY_ALLOCATION_ERROR);  
        return TokensArrErrors::MEMORY_ALLOCATION_ERROR;   
    }

    //-----------------------

    TokensArrDataFill(tokensArr);

    ON_CANARY
    (
        tokensArr->data = GetAfterFirstCanaryAdr(tokensArr);
    )

    TOKENS_ARR_CHECK(tokensArr);

    return errors;    
}
#endif

TokensArrErrors TokensArrDtor(TokensArr* const tokensArr)
{
    assert(tokensArr);

    TOKENS_ARR_CHECK(tokensArr);
    
    for (size_t i = 0; i < tokensArr->size; ++i)
    {
        if (tokensArr->data[i].valueType == TokenValueType::NAME && tokensArr->data[i].value.name)
            free(tokensArr->data[i].value.name);

        tokensArr->data[i] = TOKENS_ARR_POISON;
    }

    ON_CANARY
    (
        tokensArr->data = GetFirstCanaryAdr(tokensArr);
    )

    free(tokensArr->data);
    tokensArr->data = nullptr;

    tokensArr->size     = 0;
    tokensArr->capacity = 0;

    ON_HASH
    (
        tokensArr->dataHash = 0;
    )

    ON_CANARY
    (
        tokensArr->structCanaryLeft  = 0;
        tokensArr->structCanaryRight = 0;
    )

    return TokensArrErrors::NO_ERR;
}

TokensArrErrors TokensArrPush(TokensArr* tokensArr, const ElemType val)
{
    assert(tokensArr);
    
    TOKENS_ARR_CHECK(tokensArr);

    TokensArrErrors tokensArrReallocErr = TokensArrErrors::NO_ERR;
    if (TokensArrIsFull(tokensArr)) tokensArrReallocErr = TokensArrRealloc(tokensArr, true);

    IF_ERR_RETURN(tokensArrReallocErr);

    tokensArr->data[tokensArr->size++] = val;

    ON_HASH
    (
        UpdateDataHash(tokensArr);
        UpdateStructHash(tokensArr);
    )

    TOKENS_ARR_CHECK(tokensArr);

    return TokensArrErrors::NO_ERR;
}

TokensArrErrors TokensArrVerify(TokensArr* tokensArr)
{
    assert(tokensArr);

    if (tokensArr->data == nullptr)
    {
        TokensArrPrintError(TokensArrErrors::TOKENS_ARR_IS_NULLPTR);
        return TokensArrErrors::TOKENS_ARR_IS_NULLPTR;
    }

    if (tokensArr->capacity <= 0)
    {  
        TokensArrPrintError(TokensArrErrors::CAPACITY_OUT_OF_RANGE);
        return TokensArrErrors::CAPACITY_OUT_OF_RANGE;
    }

    if (tokensArr->size > tokensArr->capacity)
    {
        TokensArrPrintError(TokensArrErrors::SIZE_OUT_OF_RANGE);
        return TokensArrErrors::SIZE_OUT_OF_RANGE;
    }

    //-----------Canary checking----------

    ON_CANARY
    (
        if (*(CanaryType*)(GetFirstCanaryAdr(tokensArr)) != Canary)
        {
            TokensArrPrintError(TokensArrErrors::INVALID_CANARY);
            return TokensArrErrors::INVALID_CANARY;
        }

        if (*(CanaryType*)(GetSecondCanaryAdr(tokensArr)) != Canary)
        {
            TokensArrPrintError(TokensArrErrors::INVALID_CANARY);
            return TokensArrErrors::INVALID_CANARY;
        }

        if (tokensArr->structCanaryLeft != Canary)
        {
            TokensArrPrintError(TokensArrErrors::INVALID_CANARY);
            return TokensArrErrors::INVALID_CANARY;
        }

        if (tokensArr->structCanaryRight != Canary)
        {
            TokensArrPrintError(TokensArrErrors::INVALID_CANARY);
            return TokensArrErrors::INVALID_CANARY;
        }

    )

    //------------Hash checking----------

    ON_HASH
    (
        if (CalcDataHash(tokensArr) != tokensArr->dataHash)
        {
            TokensArrPrintError(TokensArrErrors::INVALID_DATA_HASH);
            return TokensArrErrors::INVALID_DATA_HASH;
        }
    )

    ON_HASH
    (
        if (CalcDataHash(tokensArr) != tokensArr->dataHash)
        {
            TokensArrPrintError(TokensArrErrors::INVALID_DATA_HASH);
            return TokensArrErrors::INVALID_DATA_HASH;
        }

        HashType prevStructHash = tokensArr->structHash;
        UpdateStructHash(tokensArr);

        if (prevStructHash != tokensArr->structHash)
        {
            TokensArrPrintError(TokensArrErrors::INVALID_STRUCT_HASH);
            tokensArr->structHash = prevStructHash;
            return TokensArrErrors::INVALID_STRUCT_HASH;
        }
    )


    return TokensArrErrors::NO_ERR;
}

void TokensArrDump(const TokensArr* tokensArr, const char* const fileName, 
                                     const char* const funcName,
                                     const int lineNumber)
{
    assert(tokensArr);
    assert(fileName);
    assert(funcName);
    assert(lineNumber > 0);
    
    LOG_BEGIN();

    Log("TokensArrDump was called in file %s, function %s, line %d\n", fileName, funcName, lineNumber);
    Log("tokensArr[%p]\n{\n", tokensArr);
    Log("\ttokensArr capacity: %zu, \n"
        "\ttokensArr size    : %zu,\n",
        tokensArr->capacity, tokensArr->size);
    
    ON_CANARY
    (
        Log("\tLeft struct canary : " CanaryTypeFormat ",\n", 
            tokensArr->structCanaryLeft);
        Log("\tRight struct canary: " CanaryTypeFormat ",\n", 
            tokensArr->structCanaryRight);

        Log("\tLeft data canary : " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetFirstCanaryAdr(tokensArr)));
        Log("\tRight data canary: " CanaryTypeFormat ",\n", 
            *(CanaryType*)(GetSecondCanaryAdr(tokensArr)));
    )

    ON_HASH
    (
        Log("\tData hash  : %llu\n", tokensArr->dataHash);
        Log("\tStruct hash: %llu\n", tokensArr->structHash);
    )

    Log("\tdata data[%p]\n\t{\n", tokensArr->data);

    /*
    if (tokensArr->data != nullptr)
    {
        for (size_t i = 0; i < (tokensArr->size < tokensArr->capacity ? tokensArr->size : tokensArr->capacity); ++i)
        {
            //Log("\t\t*[%zu] = " ElemTypeFormat, i, tokensArr->data[i]);

            if (Equal(&tokensArr->data[i], &TOKENS_ARR_POISON)) Log(" (TOKENS_ARR_POISON)");

            Log("\n");
        }

        Log("\t\tNot used values:\n");

        for(size_t i = tokensArr->size; i < tokensArr->capacity; ++i)
        {
            //Log("\t\t*[%zu] = " ElemTypeFormat, i, tokensArr->data[i]);
            
            //if (Equal(&tokensArr->data[i], &TOKENS_ARR_POISON)) Log(" (TOKENS_ARR_POISON)");

            Log("\n");
        }
    }
    */

    Log("\t}\n}\n");

    LOG_END();
}

TokensArrErrors TokensArrRealloc(TokensArr* tokensArr, bool increase)
{
    assert(tokensArr);

    TOKENS_ARR_CHECK(tokensArr);
    
    if (increase) tokensArr->capacity <<= 1;
    else          tokensArr->capacity >>= 1;

    if (!increase) 
        TokensArrFillArray(tokensArr->data + tokensArr->capacity, tokensArr->data + tokensArr->size, TOKENS_ARR_POISON);

    //--------Moves data to the first canary-------
    ON_CANARY
    (
        tokensArr->data = GetFirstCanaryAdr(tokensArr);
    )

    ElemType* tmpTokensArr = (ElemType*) realloc(tokensArr->data, 
                                             TokensArrGetSizeForCalloc(tokensArr) * sizeof(*tokensArr->data));

    if (tmpTokensArr == nullptr)
    {
        TokensArrPrintError(TokensArrErrors::MEMORY_ALLOCATION_ERROR);

        assert(tokensArr);
        if (increase) tokensArr->capacity >>= 1;
        else          tokensArr->capacity <<= 1;

        TOKENS_ARR_CHECK(tokensArr);

        return TokensArrErrors::NO_ERR;
    }

    tokensArr->data = tmpTokensArr;

    // -------Moving forward after reallocing-------
    ON_CANARY
    (
        tokensArr->data = GetAfterFirstCanaryAdr(tokensArr);
    )

    if (increase)
        TokensArrFillArray(tokensArr->data + tokensArr->size, tokensArr->data + tokensArr->capacity, TOKENS_ARR_POISON);

    // -------Putting canary at the end-----------
    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(tokensArr));
    )

    ON_HASH
    (
        UpdateDataHash(tokensArr);
        UpdateStructHash(tokensArr);
    )
    
    TOKENS_ARR_CHECK(tokensArr);

    return TokensArrErrors::NO_ERR;
}

static inline bool TokensArrIsFull(TokensArr* tokensArr)
{
    assert(tokensArr);

    TOKENS_ARR_CHECK_NO_RETURN(tokensArr);

    return tokensArr->size >= tokensArr->capacity;
}

static inline bool TokensArrIsTooBig(TokensArr* tokensArr)
{
    assert(tokensArr);

    TOKENS_ARR_CHECK_NO_RETURN(tokensArr);

    return (tokensArr->size * 4 <= tokensArr->capacity) & (tokensArr->capacity > STANDARD_CAPACITY);
}

static inline ElemType* MovePtr(ElemType* const data, const size_t moveSz, const int times)
{
    assert(data);
    assert(moveSz > 0);
    
    return (ElemType*)((char*)data + times * (long long)moveSz);
}

// NO tokensArr check because doesn't fill hashes
static void TokensArrDataFill(TokensArr* const tokensArr)
{
    assert(tokensArr);
    assert(tokensArr->data);
    assert(tokensArr->capacity > 0);

    ON_CANARY
    (
        CanaryCtor(tokensArr->data);
        tokensArr->data = GetAfterFirstCanaryAdr(tokensArr);
    )

    TokensArrFillArray(tokensArr->data, tokensArr->data + tokensArr->capacity, TOKENS_ARR_POISON);

    ON_CANARY
    (
        CanaryCtor(GetSecondCanaryAdr(tokensArr));
        tokensArr->data = GetFirstCanaryAdr(tokensArr);
    )

    // No tokensArr check because doesn't fill hashes
}

// no TOKENS_ARR_CHECK because can be used for callocing memory (data could be nullptr at this moment)
static inline size_t TokensArrGetSizeForCalloc(TokensArr* const tokensArr)
{
    assert(tokensArr);
    assert(tokensArr->capacity > 0);

    ON_CANARY
    (
        return tokensArr->capacity + 3 * sizeof(CanaryType) / sizeof(*tokensArr->data);
    )

    return tokensArr->capacity;
}

#undef TOKENS_ARR_CHECK
#undef IF_ERR_RETURN
#undef GetAfterFirstCanaryAdr
#undef GetFirstCanaryAdr
#undef GetSecondCanaryAdr

#define LOG_ERR(X) Log(HTML_RED_HEAD_BEGIN "\n" X "\n" HTML_HEAD_END "\n")
void TokensArrPrintError(TokensArrErrors error)
{
    LOG_BEGIN();

    switch(error)
    {
        case TokensArrErrors::CAPACITY_OUT_OF_RANGE:
            LOG_ERR("TokensArr capacity is out of range.\n");
            break;
        case TokensArrErrors::TOKENS_ARR_IS_NULLPTR:
            LOG_ERR("TokensArr is nullptr.\n");
            break;
        case TokensArrErrors::TOKENS_ARR_EMPTY_ERR:
            LOG_ERR("Trying to pop from empty tokensArr.\n");
            break;
        case TokensArrErrors::SIZE_OUT_OF_RANGE:
            LOG_ERR("TokensArr size is out of range.\n");
            break;
        case TokensArrErrors::MEMORY_ALLOCATION_ERROR:
            LOG_ERR("Couldn't allocate more memory for tokensArr.\n");
            break;
        case TokensArrErrors::INVALID_CANARY:
            LOG_ERR("TokensArr canary is invalid.\n");
            break;
        case TokensArrErrors::INVALID_DATA_HASH:
            LOG_ERR("TokensArr data hash is invalid.\n");
            break;
        case TokensArrErrors::INVALID_STRUCT_HASH:
            LOG_ERR("TokensArr struct hash is invalid.\n");

        case TokensArrErrors::NO_ERR:
        default:
            break;
    }

    LOG_END();
}
#undef PRINT_ERR
