#ifndef TREE_H
#define TREE_H

#include <stdio.h>
#include "NameTable/NameTable.h"

#define GENERATE_OPERATION_CMD(NAME, ...) NAME, 

enum class TreeOperationId
{
    #include "Operations.h"
};

#undef GENERATE_OPERATION_CMD

union TreeNodeValue
{
    int             num;
    size_t          nameId;
    TreeOperationId operation;
}; 

enum class TreeNodeValueType
{
    NUM,
    NAME,
    OPERATION,
    STRING_LITERAL,
};

struct TreeNode
{
    TreeNodeValue        value;
    TreeNodeValueType    valueType;
    
    TreeNode*  left;
    TreeNode* right;
};

struct Tree
{
    TreeNode* root;

    NameTableType* allNamesTable;
};

enum class TreeErrors
{
    NO_ERR,

    MEM_ERR,

    READING_ERR,

    CAPACITY_ERR,
    VARIABLE_NAME_ERR, 
    VARIABLE_VAL_ERR,
    VARIABLES_DATA_ERR,

    NODE_EDGES_ERR,

    NO_REPLACEMENT,
};

//-------------Expression main funcs----------

TreeErrors TreeCtor(Tree* tree);
void TreeDtor(Tree* tree);

TreeNode* TreeNodeCreate(TreeNodeValue value, TreeNodeValueType valueType,
                             TreeNode* left  = nullptr, TreeNode* right = nullptr);

void TreeNodeDtor(TreeNode* node);
void TreeNodeDeepDtor(TreeNode* node);

TreeErrors TreeVerify     (const Tree*      tree);
TreeErrors TreeVerify     (const TreeNode* node);

TreeNodeValue TreeCreateNumVal  (int value);
TreeNodeValue TreeCreateOpVal   (TreeOperationId operationId);
TreeNodeValue TreeCreateNameVal (size_t nameId);

TreeNode* TreeNumNodeCreate             (int value);
TreeNode* TreeNameNodeCreate            (size_t nameId);
TreeNode* TreeStringLiteralNodeCreate   (size_t literalId);

#define TREE_TEXT_DUMP(tree) TreeTextDump((tree), __FILE__, __func__, __LINE__)

void TreeTextDump(const Tree* tree, 
                  const char* fileName, const char* funcName, const int line);

void TreeGraphicDump(const Tree* tree, bool openImg);

#define TREE_DUMP(tree) TreeDump((tree), __FILE__, __func__, __LINE__)

void TreeDump(const Tree* tree, const char* fileName, const char* funcName, const int line);

void TreeNodeSetEdges(TreeNode* node, TreeNode* left, TreeNode* right);

//Tree       TreeCopy(const Tree* tree);
//TreeNode* TreeNodeCopy(const TreeNode* node);

TreeErrors TreePrintPrefixFormat(const Tree* tree, FILE* outStream);

TreeErrors TreeReadPrefixFormat(Tree* tree, FILE* inStream = stdin);

//-------------Operations funcs-----------

int  TreeOperationGetId(const char* string);
const char* TreeOperationGetLongName (const  TreeOperationId operation);

#endif