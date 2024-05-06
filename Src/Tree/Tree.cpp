#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "Tree.h"
#include "Common/StringFuncs.h"
#include "Common/Log.h"
#include "FastInput/InputOutput.h"
#include "Common/DoubleFuncs.h"
#include "NameTable/NameTable.h"

//---------------------------------------------------------------------------------------

static void TreeDtor     (TreeNode* node);


static TreeErrors TreePrintPrefixFormat(const TreeNode* node, FILE* outStream,
                                        const NameTableType* nameTable);

static TreeNode* TreeReadPrefixFormat(const char* const string, const char** stringEndPtr,
                                                                    NameTableType* allNamesTable);

static const char* TreeReadNodeValue(TreeNodeValue* value, TreeNodeValueType* valueType, 
                                      const char* string, NameTableType* allNamesTable);

static void TreeGraphicDump(const TreeNode* node, FILE* outDotFile);
static void DotFileCreateNodes(const TreeNode* node, FILE* outDotFile,
                                const NameTableType* nameTable);

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg);
static inline void DotFileBegin(FILE* outDotFile);
static inline void DotFileEnd  (FILE* outDotFile);

#define TREE_CHECK(tree)                        \
do                                                          \
{                                                           \
    TreeErrors err = TreeVerify(tree);    \
                                                            \
    if (err != TreeErrors::NO_ERR)                    \
        return err;                                         \
} while (0)

//---------------------------------------------------------------------------------------

//TODO: докинуть возможность ввода capacity
TreeErrors TreeCtor(Tree* tree)
{
    assert(tree);

    tree->root = nullptr;

    TREE_CHECK(tree);

    return TreeErrors::NO_ERR;
}

//---------------------------------------------------------------------------------------

void TreeDtor(Tree* tree)
{
    assert(tree);

    TreeDtor(tree->root);
    tree->root = nullptr;
}

//---------------------------------------------------------------------------------------

static void TreeDtor(TreeNode* node)
{
    if (node == nullptr)
        return;
    
    assert(node);

    TreeDtor(node->left);
    TreeDtor(node->right);

    TreeNodeDtor(node);
}

//---------------------------------------------------------------------------------------

TreeNode* TreeNodeCreate(TreeNodeValue value, TreeNodeValueType valueType,
                         TreeNode* left, TreeNode* right)
{   
    TreeNode* node  = (TreeNode*)calloc(1, sizeof(*node));
    assert(node);

    node->left      = left;
    node->right     = right;
    node->value     = value;
    node->valueType = valueType;
    
    return node;
}

//---------------------------------------------------------------------------------------

void TreeNodeDeepDtor(TreeNode* node)
{
    assert(node);

    TreeDtor(node);
}

void TreeNodeDtor(TreeNode* node)
{
    node->left         = nullptr;
    node->right        = nullptr;
    node->value.nameId = 0;

    free(node);
}

//---------------------------------------------------------------------------------------

TreeErrors TreeVerify(const Tree* tree)
{
    assert(tree);

    return TreeVerify(tree->root);
}

TreeErrors TreeVerify(const TreeNode* node)
{
    if (node == nullptr)
        return TreeErrors::NO_ERR;

    TreeErrors err = TreeVerify(node->left);
    if (err != TreeErrors::NO_ERR) return err;

    err = TreeVerify(node->right);
    if (err != TreeErrors::NO_ERR) return err;

    if (node->left == node->right && node->left != nullptr)
        return TreeErrors::NODE_EDGES_ERR;
    
    if (node->left == node || node->right == node)
        return TreeErrors::NODE_EDGES_ERR;

    return TreeErrors::NO_ERR;
}

//---------------------------------------------------------------------------------------

static inline void CreateImgInLogFile(const size_t imgIndex, bool openImg)
{
    static const size_t maxImgNameLength  = 64;
    static char imgName[maxImgNameLength] = "";
    snprintf(imgName, maxImgNameLength, "../imgs/img_%zu_time_%s.png", imgIndex, __TIME__);

    static const size_t     maxCommandLength  = 128;
    static char commandName[maxCommandLength] =  "";
    snprintf(commandName, maxCommandLength, "dot TreeHandler.dot -T png -o %s", imgName);
    system(commandName);

    snprintf(commandName, maxCommandLength, "<img src = \"%s\">\n", imgName);    
    Log(commandName);

    if (openImg)
    {
        snprintf(commandName, maxCommandLength, "open %s", imgName);
        system(commandName);
    }
}

//---------------------------------------------------------------------------------------

static inline void DotFileBegin(FILE* outDotFile)
{
    fprintf(outDotFile, "digraph G{\nrankdir=TB;\ngraph [bgcolor=\"#31353b\"];\n"
                        "edge[color=\"#00D0D0\"];\n");
}

//---------------------------------------------------------------------------------------

static inline void DotFileEnd(FILE* outDotFile)
{
    fprintf(outDotFile, "\n}\n");
}

//---------------------------------------------------------------------------------------

void TreeGraphicDump(const Tree* tree, bool openImg, const NameTableType* nameTable)
{
    assert(tree);

    static const char* dotFileName = "treeHandler.dot";
    FILE* outDotFile = fopen(dotFileName, "w");

    if (outDotFile == nullptr)
        return;

    DotFileBegin(outDotFile);

    DotFileCreateNodes(tree->root, outDotFile, nameTable);

    TreeGraphicDump(tree->root, outDotFile);

    DotFileEnd(outDotFile);

    fclose(outDotFile);

    static size_t imgIndex = 0;
    CreateImgInLogFile(imgIndex, openImg);
    imgIndex++;
}

//---------------------------------------------------------------------------------------

static void DotFileCreateNodes(const TreeNode* node, FILE* outDotFile,
                                const NameTableType* nameTable)
{
    assert(outDotFile);

    if (node == nullptr)
        return;
    
    fprintf(outDotFile, "node%p[shape=Mrecord, style=filled, ", node);

    if (node->valueType == TreeNodeValueType::OPERATION)
    {
        //printf("OP: %s\n", TreeOperationGetLongName(node->value.operation));
        fprintf(outDotFile, "fillcolor=\"#89AC76\", label = \"%s\", ", 
                            TreeOperationGetLongName(node->value.operation));
    }
    else if (node->valueType == TreeNodeValueType::NUM)
    {
        //printf("VAL: %d\n", node->value.value);
        fprintf(outDotFile, "fillcolor=\"#7293ba\", label = \"%d\", ", node->value.num);
    }
    else if (node->valueType == TreeNodeValueType::NAME)
    {
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = \"%s\", ",
                                nameTable->data[node->value.nameId].name);
    }
    else if (node->valueType == TreeNodeValueType::STRING_LITERAL)
    {
        fprintf(outDotFile, "fillcolor=\"#78DBE2\", label = %s, ",
                                nameTable->data[node->value.nameId].name);   
    }
    else 
        fprintf(outDotFile, "fillcolor=\"#FF0000\", label = \"ERROR\", ");

    fprintf(outDotFile, "color = \"#D0D000\"];\n");
    
    const NameTableType* localNameTable = nameTable;

    if (node->valueType == TreeNodeValueType::NAME && 
        nameTable->data[node->value.nameId].localNameTable)
    {
        localNameTable = (NameTableType*)nameTable->data[node->value.nameId].localNameTable;
    }

    DotFileCreateNodes(node->left,  outDotFile, localNameTable);
    DotFileCreateNodes(node->right, outDotFile, localNameTable);
}

//---------------------------------------------------------------------------------------

static void TreeGraphicDump(const TreeNode* node, FILE* outDotFile)
{
    if (node == nullptr)
    {
        fprintf(outDotFile, "\n");
        return;
    }
    
    fprintf(outDotFile, "node%p;\n", node);

    if (node->left != nullptr) fprintf(outDotFile, "node%p->", node);
    TreeGraphicDump(node->left, outDotFile);

    if (node->right != nullptr) fprintf(outDotFile, "node%p->", node);
    TreeGraphicDump(node->right, outDotFile);
}

//---------------------------------------------------------------------------------------

void TreeTextDump(const Tree* tree, const char* fileName, const char* funcName, const int line,
                                                                    const NameTableType* nameTable)
{
    assert(tree);
    assert(fileName);
    assert(funcName);

    LogBegin(fileName, funcName, line);

    Log("Tree root: %p, value: %s\n", tree->root, tree->root->value);
    Log("Tree: ");
    TreePrintPrefixFormat(tree, nullptr, nameTable);

    LOG_END();
}

//---------------------------------------------------------------------------------------

void TreeDump(const Tree* tree, 
              const char* fileName, const char* funcName, const int line,
              const NameTableType* nameTable)
{
    assert(tree);
    assert(fileName);
    assert(funcName);

    TreeTextDump(tree, fileName, funcName, line, nameTable);

    TreeGraphicDump(tree, false, nameTable);
}

//---------------------------------------------------------------------------------------

/*
Tree TreeCopy(const Tree* tree)
{
    Tree copyExpr = {};
    TreeCtor(&copyExpr);

    TreeNodeType* copyExprRoot = TreeNodeCopy(tree->root);

    copyExpr.root = copyExprRoot;

    return copyExpr;
}

TreeNodeType* TreeNodeCopy(const TreeNodeType* node)
{
    if (node == nullptr)
        return nullptr;

    TreeNodeType* left  = TreeNodeCopy(node->left);
    TreeNodeType* right = TreeNodeCopy(node->right);

    return TreeNodeCreate(node->value, node->valueType, left, right);
}
*/

//---------------------------------------------------------------------------------------

TreeNodeValue TreeCreateNumVal(int value)
{
    TreeNodeValue nodeValue =
    {
        .num = value
    };

    return nodeValue;
}

TreeNodeValue TreeCreateOpVal(TreeOperationId operation)
{
    TreeNodeValue value =
    {
        .operation = operation,
    };

    return value;
}

TreeNodeValue TreeCreateNameVal(size_t nameId)
{
    TreeNodeValue value = 
    {
        .nameId = nameId,
    };

    return value;
}

//---------------------------------------------------------------------------------------

TreeNode* TreeNumNodeCreate(int value)
{
    TreeNodeValue nodeVal = TreeCreateNumVal(value);

    return TreeNodeCreate(nodeVal, TreeNodeValueType::NUM);
}

TreeNode* TreeNameNodeCreate(size_t nameId)
{
    TreeNodeValue nodeVal  = TreeCreateNameVal(nameId);

    return TreeNodeCreate(nodeVal, TreeNodeValueType::NAME);
}

TreeNode* TreeStringLiteralNodeCreate(size_t literalId)
{
    TreeNodeValue nodeVal  = TreeCreateNameVal(literalId);

    return TreeNodeCreate(nodeVal, TreeNodeValueType::STRING_LITERAL);
}

//---------------------------------------------------------------------------------------

#define PRINT(outStream, ...)                          \
do                                                     \
{                                                      \
    if (outStream) fprintf(outStream, __VA_ARGS__);    \
    Log(__VA_ARGS__);                                  \
} while (0)

TreeErrors TreePrintPrefixFormat(const Tree* tree, FILE* outStream,
                                 const NameTableType* nameTable)
{
    assert(tree);
    assert(outStream);

    LOG_BEGIN();

    TreeErrors err = TreePrintPrefixFormat(tree->root, outStream, nameTable);

    PRINT(outStream, "\n");

    LOG_END();

    return err;
}

//---------------------------------------------------------------------------------------

static TreeErrors TreePrintPrefixFormat(const TreeNode* node, FILE* outStream,
                                        const NameTableType* nameTable)
{
    if (node == nullptr)
    {
        PRINT(outStream, "nil ");
        return TreeErrors::NO_ERR;
    }

    PRINT(outStream, "(");
    
    if (node->valueType == TreeNodeValueType::NUM)
        PRINT(outStream, "%d ", node->value.num);
    else if (node->valueType == TreeNodeValueType::NAME)
        PRINT(outStream, "%s ", nameTable->data[node->value.nameId].name);
    else if (node->valueType == TreeNodeValueType::STRING_LITERAL)
        PRINT(outStream, "%s ", nameTable->data[node->value.nameId].name);
    else
        PRINT(outStream, "%s ", TreeOperationGetLongName(node->value.operation));

    TreeErrors err = TreeErrors::NO_ERR;

    err = TreePrintPrefixFormat(node->left, outStream, nameTable);
    
    err = TreePrintPrefixFormat(node->right, outStream, nameTable);

    PRINT(outStream, ")");
    
    return err;
}

TreeErrors TreeReadPrefixFormat(Tree* tree, NameTableType** allNamesTable, FILE* inStream)
{
    assert(tree);
    assert(inStream);
    assert(allNamesTable);

    char* inputTree = ReadText(inStream);

    if (inputTree == nullptr)
        return TreeErrors::READING_ERR;

    const char* inputTreeEndPtr = inputTree;

    NameTableCtor(allNamesTable);

    tree->root = TreeReadPrefixFormat(inputTree, &inputTreeEndPtr, *allNamesTable);

    free(inputTree);

    return TreeErrors::NO_ERR;
}

//---------------------------------------------------------------------------------------

static TreeNode* TreeReadPrefixFormat(const char* const string, const char** stringEndPtr,
                                                                    NameTableType* allNamesTable)
{
    assert(string);

    const char* stringPtr = string;

    stringPtr = SkipSymbolsWhileStatement(stringPtr, isspace);

    int symbol = *stringPtr;
    stringPtr++;
    if (symbol != '(') //skipping nils
    {
        int shift = 0;
        sscanf(stringPtr, "%*s%n", &shift);
        stringPtr += shift;

        *stringEndPtr = stringPtr;
        return nullptr;
    }

    TreeNodeValue value         = {};
    TreeNodeValueType valueType = {};

    stringPtr = TreeReadNodeValue(&value, &valueType, stringPtr, allNamesTable);
    TreeNode* node = TreeNodeCreate(value, valueType);
    
    TreeNode* left  = TreeReadPrefixFormat(stringPtr, &stringPtr, allNamesTable);

    TreeNode* right = nullptr;
    right = TreeReadPrefixFormat(stringPtr, &stringPtr, allNamesTable);

    stringPtr = SkipSymbolsUntilStopChar(stringPtr, ')');
    ++stringPtr;

    TreeNodeSetEdges(node, left, right);

    *stringEndPtr = stringPtr;
    return node;
}

static const char* TreeReadNodeValue(TreeNodeValue* value, TreeNodeValueType* valueType, 
                                     const char* string, NameTableType* allNamesTable)
{
    assert(value);
    assert(string);
    assert(valueType);
    
    int readenValue = 0;
    int shift = 0;
    int scanResult = sscanf(string, "%d%n\n", &readenValue, &shift);

    if (scanResult != 0)
    {
        value->num = readenValue;
        *valueType = TreeNodeValueType::NUM;
        return string + shift;
    }

    shift = 0;

    static const size_t      maxInputStringSize  = 1024;
    static char  inputString[maxInputStringSize] =  "";

    const char* stringPtr = string;
    if (*string == '"')
    {
        size_t inputStringPos = 0;
        ++stringPtr;
        while (*stringPtr != '"')
        {
            assert(inputStringPos < maxInputStringSize);

            inputString[inputStringPos] = *stringPtr;
            inputStringPos++;
            stringPtr++;
        }
        stringPtr++;
        inputString[inputStringPos] = '\0';

        Name pushName = {};
        NameCtor(&pushName, inputString, nullptr, 0, IRRegister::NO_REG);

        NameTablePush(allNamesTable, pushName);

        *value     = TreeCreateNameVal(allNamesTable->size - 1);
        *valueType = TreeNodeValueType::STRING_LITERAL;
        return stringPtr;
    }

    sscanf(string, "%s%n", inputString, &shift);

    stringPtr = string + shift;
    assert(isspace(*stringPtr));

    int operationId = TreeOperationGetId(inputString);
    if (operationId != -1)
    {
        *value     = TreeCreateOpVal((TreeOperationId) operationId);
        *valueType = TreeNodeValueType::OPERATION;
        return stringPtr;
    }

    Name* varName = nullptr;
    NameTableFind(allNamesTable, inputString, &varName);

    if (varName == nullptr)
    {
        Name pushName = {};
        NameCtor(&pushName, inputString, nullptr, 0, IRRegister::NO_REG);

        NameTablePush(allNamesTable, pushName);

        *value = TreeCreateNameVal(allNamesTable->size - 1);
    }
    else 
    {
        size_t varNamePos = 0;
        NameTableGetPos(allNamesTable, varName, &varNamePos);

        *value = TreeCreateNameVal(varNamePos);
    }

    *valueType   = TreeNodeValueType::NAME;

    return stringPtr;
}

void TreeNodeSetEdges(TreeNode* node, TreeNode* left, TreeNode* right)
{
    assert(node);

    node->left  = left;
    node->right = right;
}


int TreeOperationGetId(const char* string)
{
    assert(string);

    #define GENERATE_OPERATION_CMD(NAME, ...)   \
        if (strcasecmp(string, #NAME) == 0)     \
            return (int)TreeOperationId::NAME;  \
        else

    #include "Operations.h"

    /* else */
    {
        return -1;
    }

    #undef GENERATE_OPERATION_CMD

    return -1;
}

const char* TreeOperationGetLongName(const  TreeOperationId operation)
{
    #define GENERATE_OPERATION_CMD(NAME, ...)           \
        case TreeOperationId::NAME:               \
            return #NAME;

    switch (operation)
    {
        #include "Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    return nullptr;
}
