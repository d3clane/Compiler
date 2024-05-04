#include <assert.h>

#include "IRRegisters.h"

const char* IRRegisterGetName(IRRegister reg)
{
    #define DEF_IR_REG(REG_NAME)    \
        case IRRegister::REG_NAME:  \
            return #REG_NAME;

    switch (reg)
    {
        #include "IRRegistersDefs.h"

        default: // Unreachable
            assert(false);
            return nullptr;
    }

    // Unreachable

    assert(false);
    return nullptr;
}