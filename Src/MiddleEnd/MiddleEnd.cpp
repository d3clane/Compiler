#include <assert.h>
#include <math.h>

#include "Tree/Tree.h"
#include "Common/DoubleFuncs.h"
#include "Tree/DSL.h"
#include "Common/Log.h"

//---------------Calculation-------------------

static int TreeCalculate(const TreeNode* node);

static int CalculateUsingOperation(const TreeOperationId operation, 
                                   const int val1, const int val2 = 0);

//--------------------------------Simplify-------------------------------------------

static TreeNode* TreeSimplifyConstants          (TreeNode* node, int* simplifiesCount);
static TreeNode* TreeSimplifyNeutralNodes       (TreeNode* node, int* simplifiesCount);
static inline TreeNode* TreeSimplifyAdd         (TreeNode* node, int* simplifiesCount);
static inline TreeNode* TreeSimplifySub         (TreeNode* node, int* simplifiesCount);
static inline TreeNode* TreeSimplifyMul         (TreeNode* node, int* simplifiesCount);
static inline TreeNode* TreeSimplifyDiv         (TreeNode* node, int* simplifiesCount);
static inline TreeNode* TreeSimplifyPow         (TreeNode* node, int* simplifiesCount);

static inline TreeNode* TreeSimplifyReturnLeftNode (TreeNode* node);
static inline TreeNode* TreeSimplifyReturnRightNode(TreeNode* node);
static inline TreeNode* TreeSimplifyReturnNumNode(TreeNode* node, int val);

//---------------------------------------------------------------------------------------

static bool TreeNodeCanBeCalculated(const TreeNode* node);

//---------------------------------------------------------------------------------------


int TreeCalculate(const Tree* tree)
{
    assert(tree);

    return TreeCalculate(tree->root);
}

static int TreeCalculate(const TreeNode* node)
{
    if (node == nullptr)
        return 0;
    
    if (IS_NUM(node))
        return node->value.num;

    assert(!IS_NAME(node));

    int firstVal  = TreeCalculate(L(node));
    int secondVal = TreeCalculate(R(node));
    
    return CalculateUsingOperation(node->value.operation, firstVal, secondVal);
}

static int CalculateUsingOperation(const TreeOperationId operation, 
                                   const int val1, const int val2)
{
    #define GENERATE_OPERATION_CMD(NAME, CALC_CODE, ...)                                    \
        case TreeOperationId::NAME:                                                         \
        {                                                                                   \
            CALC_CODE;                                                                      \
            break;                                                                          \
        }                                                                               

    switch(operation)
    {
        #include "Tree/Operations.h"

        default:
            break;
    }

    #undef GENERATE_OPERATION_CMD

    LOG_BEGIN();
    Log   ("Op name - %s\n, id - %d", TreeOperationGetLongName(operation), operation);
    printf("Op name - %s\n", TreeOperationGetLongName(operation));
    LOG_END();

    assert(false);

    return 0;
}

//---------------------------------------------------------------------------------------

void TreeSimplify(Tree* tree)
{
    assert(tree);

    int simplifiesCount = 0;

    do
    {
        simplifiesCount = 0;
        tree->root = TreeSimplifyConstants   (tree->root, &simplifiesCount);
        tree->root = TreeSimplifyNeutralNodes(tree->root, &simplifiesCount);
    
    } while (simplifiesCount != 0);
}

static TreeNode* TreeSimplifyConstants (TreeNode* node, int* simplifiesCount)
{
    assert(simplifiesCount);

    if (node == nullptr)
        return node;

    if (TreeNodeCanBeCalculated(node) && !IS_NUM(node))
    {
        TreeNode* numNode = CREATE_NUM(TreeCalculate(node));
        *simplifiesCount += 1;
        TreeNodeDtor(node);
        return numNode;
    }

    node->left  = TreeSimplifyConstants(L(node), simplifiesCount);
    node->right = TreeSimplifyConstants(R(node), simplifiesCount);

    return node;
}

static TreeNode* TreeSimplifyNeutralNodes(TreeNode* node, int* simplifiesCount)
{
    if (node == nullptr || !IS_OP(node))
        return node;
    
    TreeNode* left  = TreeSimplifyNeutralNodes(L(node), simplifiesCount);
    TreeNode* right = TreeSimplifyNeutralNodes(R(node), simplifiesCount);
    
    if (L(node) != left)
    {
        TreeNodeDtor(L(node));
        node->left = left;        
    }

    if (R(node) != right)
    {
        TreeNodeDtor(R(node));
        node->right = right;
    }

    if (left == nullptr || right == nullptr)
        return node;
    
    assert(IS_OP(node));

    if (IS_NAME(right) && IS_NAME(left) && left->value.nameId == right->value.nameId && 
        node->value.operation == TreeOperationId::SUB)
        return TreeSimplifyReturnNumNode(node, 0);

    if (!IS_NUM(left) && !IS_NUM(right))
        return node;

    assert(L(node) == left);
    assert(R(node) == right);
    switch (node->value.operation)
    {
        case TreeOperationId::ADD:
            return TreeSimplifyAdd(node, simplifiesCount);
        case TreeOperationId::SUB:
            return TreeSimplifySub(node, simplifiesCount);
        case TreeOperationId::MUL:
            return TreeSimplifyMul(node, simplifiesCount);
        case TreeOperationId::DIV:
            return TreeSimplifyDiv(node, simplifiesCount);
        
        case TreeOperationId::POW:
            return TreeSimplifyPow(node, simplifiesCount);

        default:
            break;
    }

    return node;
}

//---------------------------------------------------------------------------------------

#define CHECK()                 \
do                              \
{                               \
    assert(simplifiesCount);    \
    assert(node);              \
    assert(L(node));           \
    assert(R(node));           \
} while (0)


static inline TreeNode* TreeSimplifyAdd(TreeNode* node, int* simplifiesCount)
{
    CHECK();

    if (R_IS_NUM(node) && DoubleEqual(R_NUM(node), 0))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnLeftNode(node);
    }

    if (L_IS_NUM(node) && DoubleEqual(L_NUM(node), 0))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnRightNode(node);
    }

    return node;
}

static inline TreeNode* TreeSimplifySub(TreeNode* node, int* simplifiesCount)
{
    CHECK();

    if (R_IS_NUM(node) && DoubleEqual(R_NUM(node), 0))
    {    
        (*simplifiesCount)++;
        return TreeSimplifyReturnLeftNode(node);
    }

    return node;
}

static inline TreeNode* TreeSimplifyMul(TreeNode* node, int* simplifiesCount)
{
    CHECK();

    if (R_IS_NUM(node) && DoubleEqual(R_NUM(node), 0))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnNumNode(node, 0);
    }

    if (L_IS_NUM(node) && DoubleEqual(L_NUM(node), 0))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnNumNode(node, 0);
    }

    if (R_IS_NUM(node) && DoubleEqual(R_NUM(node), 1))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnLeftNode(node);
    }

    if (L_IS_NUM(node) && DoubleEqual(L_NUM(node), 1))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnRightNode(node);
    }

    return node;
}

static inline TreeNode* TreeSimplifyDiv(TreeNode* node, int* simplifiesCount)
{
    CHECK();

    if (L_IS_NUM(node) && DoubleEqual(L_NUM(node), 0))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnNumNode(node, 0);
    }

    if (R_IS_NUM(node) && DoubleEqual(R_NUM(node), 1))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnLeftNode(node);
    }

    return node;
}

static inline TreeNode* TreeSimplifyPow(TreeNode* node, int* simplifiesCount)
{
    CHECK();

    if (R_IS_NUM(node) && DoubleEqual(R_NUM(node), 0))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnNumNode(node, 1);
    }

    if (L_IS_NUM(node) && DoubleEqual(L_NUM(node), 0))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnNumNode(node, 0);
    }

    if (R_IS_NUM(node) && DoubleEqual(R_NUM(node), 1))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnLeftNode(node);
    }

    if (L_IS_NUM(node) && DoubleEqual(L_NUM(node), 1))
    {
        (*simplifiesCount)++;
        return TreeSimplifyReturnNumNode(node, 1);
    }

    return node;
}


#undef CHECK
//---------------------------------------------------------------------------------------

static inline TreeNode* TreeSimplifyReturnLeftNode(TreeNode* node)
{
    assert(node);

    TreeNodeDtor(node->right);

    TreeNode* left = L(node);

    node->left  = nullptr;
    node->right = nullptr;

    return left;
}

static inline TreeNode* TreeSimplifyReturnRightNode(TreeNode* node)
{
    assert(node);

    TreeNodeDtor(node->left);

    TreeNode* right = R(node);
    
    node->left  = nullptr;
    node->right = nullptr;

    return right;
}

static inline TreeNode* TreeSimplifyReturnNumNode(TreeNode* node, int value)
{
    TreeNodeDtor(R(node));
    TreeNodeDtor(L(node));

    node->left  = nullptr;
    node->right = nullptr;

    return CREATE_NUM(value);
}

//---------------------------------------------------------------------------------------

static bool TreeNodeCanBeCalculated(const TreeNode* node)
{
    if (node == nullptr)
        return true;
    
    if (IS_NUM(node))
        return true;
    
    if (IS_NAME(node))
        return false;

    if (IS_STRING_LITERAL(node))
        return false;

    switch (node->value.operation)
    {
        case TreeOperationId::ADD:
        case TreeOperationId::SUB:
        case TreeOperationId::MUL:
        case TreeOperationId::DIV:
        case TreeOperationId::POW:
        case TreeOperationId::SQRT:
            break;
        
        default:
            return false;
            break;
    }

    bool canBeCalculated  = TreeNodeCanBeCalculated(L(node));

    if (!canBeCalculated)
        return canBeCalculated;
    
    canBeCalculated = TreeNodeCanBeCalculated(R(node));

    return canBeCalculated;
}
