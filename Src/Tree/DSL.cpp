#include "DSL.h"

#include "Tree.h"


#define GENERATE_OPERATION_CMD(NAME, ...)                                       \
    TreeNode* CREATE_##NAME ##_NODE(TreeNode* left, TreeNode* right)     \
    {                                                                           \
        return TreeNodeCreate(TreeCreateOpVal(TreeOperationId::NAME),     \
                              TreeNodeValueType::OPERATION,                   \
                              left, right);                                     \
    }

#include "Operations.h"

#undef GENERATE_OPERATION_CMD
