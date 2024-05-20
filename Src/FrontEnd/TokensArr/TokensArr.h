#ifndef TOKENS_ARR_H
#define TOKENS_ARR_H

/// @file
/// @brief Contains functions to work with tokensArr

#include <assert.h>

#include "TokenTypes.h"
#include "HashFuncs.h"

//#define TOKENS_ARR_CANARY_PROTECTION
//#define TOKENS_ARR_HASH_PROTECTION

#ifdef TOKENS_ARR_CANARY_PROTECTION

    #define ON_CANARY(...) __VA_ARGS__
    typedef unsigned long long CanaryType;

#else
    
    #define ON_CANARY(...)

#endif

#ifdef TOKENS_ARR_HASH_PROTECTION

    #define ON_HASH(...) __VA_ARGS__

#else

    #define ON_HASH(...) 

#endif

///@brief tokensArr dump that substitutes __FILE__, __func__, __LINE__
#define TOKENS_ARR_DUMP(STK) TokensArrDump((STK), __FILE__, __func__, __LINE__)

ON_HASH
(
    ///@brief HashType for hasing function in tokensArr
    typedef HashType HashFuncType(const void* hashingArr, const size_t length, const uint64_t seed);
)

/// @brief Contains all info about data to use it 
struct TokensArr
{
    ON_CANARY
    (
        CanaryType structCanaryLeft; ///< left canary for the struct
    )

    ElemType* data;       ///< data with values. Have to be a dynamic array.
    size_t size;          ///< pos to push/pop values (actual size of the data at this moment).
    
    ON_HASH
    (
        
        HashType dataHash;      ///< hash of all elements in data.

        HashFuncType* HashFunc; ///< hashing function

        HashType structHash;    ///< hash of all elements in struct.
    )

    size_t capacity;     ///< REAL size of the data at this moment (calloced more than need at this moment).

    ON_CANARY
    (
        CanaryType structCanaryRight; ///< right canary for the struct
    )
};

/// @brief Errors that can occure while tokensArr is working. 
enum class TokensArrErrors
{
    NO_ERR,

    MEMORY_ALLOCATION_ERROR,
    TOKENS_ARR_EMPTY_ERR, 
    TOKENS_ARR_IS_NULLPTR,
    CAPACITY_OUT_OF_RANGE,
    SIZE_OUT_OF_RANGE,
    INVALID_CANARY, 
    INVALID_DATA_HASH,
    INVALID_STRUCT_HASH,
};

#ifdef TOKENS_ARR_HASH_PROTECTION
    /// @brief Constructor
    /// @param [out]stk tokensArr to fill
    /// @param [in]capacity size to reserve for the tokensArr
    /// @param [in]HashFunc hash function for calculating hash
    /// @return errors that occurred
    TokensArrErrors TokensArrCtor(TokensArr* const stk, const size_t capacity = 0, 
                        const HashFuncType HashFunc = TokensArrMurmurHash);
#else
    /// @brief Constructor
    /// @param [out]stk tokensArr to fill
    /// @param [in]capacity size to reserve for the tokensArr
    /// @return errors that occurred
    TokensArrErrors TokensArrCtor(TokensArr* const stk, const size_t capacity = 0);
#endif

/// @brief Destructor
/// @param [out]stk tokensArr to destruct
/// @return errors that occurred
TokensArrErrors TokensArrDtor(TokensArr* const stk);

/// @brief Pushing value to the tokensArr
/// @param [out]stk tokensArr to push in
/// @param [in]val  value to push
/// @return errors that occurred
TokensArrErrors TokensArrPush(TokensArr* stk, const ElemType val);

/// @brief Verifies if tokensArr is used right
/// @param [in]stk tokensArr to verify
/// @return TokensArrErrors in tokensArr
TokensArrErrors TokensArrVerify(TokensArr* stk);

/// @brief Prints tokensArr to log-file 
/// @param [in]stk tokensArr to print out
/// @param [in]fileName __FILE__
/// @param [in]funcName __func__
/// @param [in]lineNumber __LINE__
void TokensArrDump(const TokensArr* stk, const char* const fileName, 
                                     const char* const funcName,
                                     const int lineNumber);

/// @brief Checks if tokensArr is empty
/// @param [in]stk tokensArr to check 
/// @return true if tokensArr is empty otherwise false
static inline bool TokensArrIsEmpty(const TokensArr* stk)
{
    assert(stk);
    assert(stk->data);
    
    return stk->size == 0;
}

/// @brief Prints tokensArr error to log file
/// @param [in]error error to print
void TokensArrPrintError(TokensArrErrors error);

#endif // TOKENS_ARR_H