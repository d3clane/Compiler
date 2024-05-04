#ifndef LABEL_TABLE_H
#define LABEL_TABLE_H

/// @file
/// @brief Contains functions to work with nameTable

#include <assert.h>

#include "LabelTableTypes.h"
#include "LabelTableHashFuncs.h"
#include "BackEnd/IR/IRRegisters.h"

//#define LABEL_TABLE_CANARY_PROTECTION
//#define LABEL_TABLE_HASH_PROTECTION

#ifdef LABEL_TABLE_CANARY_PROTECTION

    #define ON_CANARY(...) __VA_ARGS__
    typedef unsigned long long CanaryType;

#else
    
    #define ON_CANARY(...)

#endif

#ifdef LABEL_TABLE_HASH_PROTECTION

    #define ON_HASH(...) __VA_ARGS__

#else

    #define ON_HASH(...) 

#endif

///@brief nameTable dump that substitutes __FILE__, __func__, __LINE__
#define LABEL_TABLE_DUMP(STK) LabelTableDump((STK), __FILE__, __func__, __LINE__)

ON_HASH
(
    ///@brief HashType for hasing function in nameTable
    typedef HashType HashFuncType(const void* hashingArr, const size_t length, const uint64_t seed);
)

/// @brief Contains all info about data to use it 
struct LabelTableType
{
    ON_CANARY
    (
        CanaryType structCanaryLeft; ///< left canary for the struct
    )

    LabelTableValue* data;       ///< data with values. Have to be a dynamic array.
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
enum class LabelTableErrors
{
    NO_ERR,

    MEMORY_ALLOCATION_ERROR,
    LABEL_TABLE_EMPTY_ERR, 
    LABEL_TABLE_IS_NULLPTR,
    CAPACITY_OUT_OF_RANGE,
    SIZE_OUT_OF_RANGE,
    INVALID_CANARY, 
    INVALID_DATA_HASH,
    INVALID_STRUCT_HASH,
};

#ifdef LABEL_TABLE_HASH_PROTECTION
    /// @brief Constructor
    /// @param [out]stk nameTable to fill
    /// @param [in]capacity size to reserve for the nameTable
    /// @param [in]HashFunc hash function for calculating hash
    /// @return errors that occurred
    LabelTableErrors LabelTableCtor(LabelTableType* const stk, const size_t capacity = 0, 
                        const HashFuncType HashFunc = LabelTableMurmurHash);
#else
    /// @brief Constructor
    /// @param [out]stk nameTable to fill
    /// @param [in]capacity size to reserve for the nameTable
    /// @return errors that occurred
    LabelTableErrors LabelTableCtor(LabelTableType** const stk, const size_t capacity = 0);
#endif

/// @brief Destructor
/// @param [out]stk nameTable to destruct
/// @return errors that occurred
LabelTableErrors LabelTableDtor(LabelTableType* const stk);

/// @brief Pushing value to the nameTable
/// @param [out]stk nameTable to push in
/// @param [in]val  value to push
/// @return errors that occurred
LabelTableErrors LabelTablePush(LabelTableType* stk, const LabelTableValue val);

LabelTableErrors LabelTableFind(const LabelTableType* table, const char* name, LabelTableValue** outLabel);

LabelTableErrors LabelTableGetPos(const LabelTableType* table, LabelTableValue* namePtr, size_t* outPos);

/// @brief Verifies if nameTable is used properly
/// @param [in]stk nameTable to verify
/// @return LabelTableErrors in nameTable
LabelTableErrors LabelTableVerify(const LabelTableType* stk);

/// @brief Prints nameTable to log-file 
/// @param [in]stk nameTable to print out
/// @param [in]fileName __FILE__
/// @param [in]funcName __func__
/// @param [in]lineNumber __LINE__
void LabelTableDump(const LabelTableType* stk, const char* const fileName, 
                                     const char* const funcName,
                                     const int lineNumber);

/// @brief Checks if nameTable is empty
/// @param [in]stk nameTable to check 
/// @return true if nameTable is empty otherwise false
static inline bool LabelTableIsEmpty(const LabelTableType* stk)
{
    assert(stk);
    assert(stk->data);
    
    return stk->size == 0;
}

static inline const char* LabelTableGetLabel(const LabelTableType* table, size_t pos)
{
    return table->data[pos].label;
}

/// @brief Prints nameTable error to log file
/// @param [in]error error to print
void LabelTablePrintError(LabelTableErrors error);

void LabelTableValueCtor(LabelTableValue* value, const char* string, IRNode* connectedNode);

#endif // LABEL_TABLE_H