#include <assert.h>
#include <stdarg.h>

#include "x86Translate.h"

static inline void FprintfAsmLine(FILE* outStream, const size_t numberOfTabs, 
                                   const char* format, ...);

#define PRINT_LABEL(LABEL) FprintfLabel(outStream, LABEL);
static inline void FprintfLabel(FILE* outStream, const char* label);

void TranslateToX86(const IR* ir, FILE* outStream)
{
    assert(ir);

    IRNode* beginNode = IRHead(ir);
    IRNode* node = beginNode;
    
#define DEF_IR_OP(OP_NAME, X86_ASM_CODE, ...)           \
    case IROperation::OP_NAME:                          \
    {                                                   \
        X86_ASM_CODE;                                   \
        break;                                          \
    }

    do
    {
        switch (node->operation)
        {
            //#include "BackEnd/IR/IROperations.h" // cases on defines
        
            default:    // Unreachable
                assert(false);
                break;
        }

        node = node->nextNode;
    } while (node != beginNode);
    
}

static inline void FprintfAsmLine(FILE* outStream, const size_t numberOfTabs, 
                                  const char* format, ...)
{
    va_list args = {};
    va_start(args, format);

    for (size_t i = 0; i < numberOfTabs; ++i)
        fprintf(outStream, "    ");
    
    vfprintf(outStream, format, args);

    va_end(args);
}

