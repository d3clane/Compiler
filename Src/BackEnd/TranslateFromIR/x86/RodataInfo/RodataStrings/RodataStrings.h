#ifndef RODATA_STRINGS_H
#define RODATA_STRINGS_H

/// @file
/// @brief Contains functions to work with nameTable

#include <assert.h>

#include "RodataStringsTypes.h"
#include "RodataStringsHashFuncs.h"

//#define RODATA_STRINGS_CANARY_PROTECTION
//#define RODATA_STRINGS_HASH_PROTECTION

#ifdef RODATA_STRINGS_CANARY_PROTECTION

    #define ON_CANARY(...) __VA_ARGS__
    typedef unsigned long long CanaryType;

#else
    
    #define ON_CANARY(...)

#endif

#ifdef RODATA_STRINGS_HASH_PROTECTION

    #define ON_HASH(...) __VA_ARGS__

#else

    #define ON_HASH(...) 

#endif

///@brief nameTable dump that substitutes __FILE__, __func__, __LINE__
#define RODATA_STRINGS_DUMP(STK) RodataStringsDump((STK), __FILE__, __func__, __LINE__)

ON_HASH
(
    ///@brief HashType for hasing function in nameTable
    typedef HashType HashFuncType(const void* hashingArr, const size_t length, const uint64_t seed);
)

/// @brief Contains all info about data to use it 
struct RodataStringsType
{
    ON_CANARY
    (
        CanaryType structCanaryLeft; ///< left canary for the struct
    )

    RodataStringsValue* data;       ///< data with values. Have to be a dynamic array.
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

/// @brief Errors that can occur while nameTable is working. 
enum class RodataStringsErrors
{
    NO_ERR,

    MEMORY_ALLOCATION_ERROR,
    RODATA_STRINGS_EMPTY_ERR, 
    RODATA_STRINGS_IS_NULLPTR,
    CAPACITY_OUT_OF_RANGE,
    SIZE_OUT_OF_RANGE,
    INVALID_CANARY, 
    INVALID_DATA_HASH,
    INVALID_STRUCT_HASH,
};

#ifdef RODATA_STRINGS_HASH_PROTECTION
    /// @brief Constructor
    /// @param [out]stk nameTable to fill
    /// @param [in]capacity size to reserve for the nameTable
    /// @param [in]HashFunc hash function for calculating hash
    /// @return errors that occurred
    RodataStringsErrors RodataStringsCtor(RodataStringsType* const stk, const size_t capacity = 0, 
                        const HashFuncType HashFunc = RodataStringsMurmurHash);
#else
    /// @brief Constructor
    /// @param [out]stk nameTable to fill
    /// @param [in]capacity size to reserve for the nameTable
    /// @return errors that occurred
    RodataStringsErrors RodataStringsCtor(RodataStringsType** const stk, const size_t capacity = 0);
#endif

/// @brief Destructor
/// @param [out]stk nameTable to destruct
/// @return errors that occurred
RodataStringsErrors RodataStringsDtor(RodataStringsType* const stk);

/// @brief Pushing value to the nameTable
/// @param [out]stk nameTable to push in
/// @param [in]val  value to push
/// @return errors that occurred
RodataStringsErrors RodataStringsPush(RodataStringsType* stk, const RodataStringsValue val);

RodataStringsErrors RodataStringsFind(const RodataStringsType* table, const char* name, RodataStringsValue** outString);

RodataStringsErrors RodataStringsGetPos(const RodataStringsType* table, RodataStringsValue* namePtr, size_t* outPos);

/// @brief Verifies if nameTable is used properly
/// @param [in]stk nameTable to verify
/// @return RodataStringsErrors in nameTable
RodataStringsErrors RodataStringsVerify(const RodataStringsType* stk);

/// @brief Prints nameTable to log-file 
/// @param [in]stk nameTable to print out
/// @param [in]fileName __FILE__
/// @param [in]funcName __func__
/// @param [in]lineNumber __LINE__
void RodataStringsDump(const RodataStringsType* stk, const char* const fileName, 
                                     const char* const funcName,
                                     const int lineNumber);

/// @brief Checks if nameTable is empty
/// @param [in]stk nameTable to check 
/// @return true if nameTable is empty otherwise false
static inline bool RodataStringsIsEmpty(const RodataStringsType* stk)
{
    assert(stk);
    assert(stk->data);
    
    return stk->size == 0;
}

static inline const char* RodataStringsGetString(const RodataStringsType* table, size_t pos)
{
    return table->data[pos].string;
}

/// @brief Prints nameTable error to log file
/// @param [in]error error to print
void RodataStringsPrintError(RodataStringsErrors error);

void RodataStringsValueCtor(RodataStringsValue* value, const char* string);

#endif // RODATA_STRINGS_H