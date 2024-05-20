#ifndef DSL_H
#define DSL_H

#include "Tree.h"

//TODO: change token on node

#define L(token)            token->left
#define R(token)            token->right

#define   IS_NUM(token)  (token->valueType        == TreeNodeValueType::NUM)
#define L_IS_NUM(token)  (token->left->valueType  == TreeNodeValueType::NUM)
#define R_IS_NUM(token)  (token->right->valueType == TreeNodeValueType::NUM)
#define    IS_OP(token)  (token->valueType        == TreeNodeValueType::OPERATION)
#define   IS_NAME(token) (token->valueType        == TreeNodeValueType::NAME)

#define IS_STRING_LITERAL(token) (token->valueType == TreeNodeValueType::STRING_LITERAL)

#define L_NUM(token) token->left->value.num
#define R_NUM(token) token->right->value.num


#define CREATE_NUM(VALUE)         TreeNumNodeCreate(VALUE)
#define CREATE_VAR(id)            TreeNameNodeCreate(id)
#define CREATE_STRING_LITERAL(id) TreeStringLiteralNodeCreate(id);

#define GENERATE_OPERATION_CMD(NAME, ...)                                               \
    TreeNode* CREATE_##NAME ##_NODE(TreeNode* left, TreeNode* right = nullptr);  \

#include "Operations.h"

#undef GENERATE_OPERATION_CMD

#endif