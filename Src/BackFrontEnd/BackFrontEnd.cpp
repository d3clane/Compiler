#include <assert.h>

#include "Tree/DSL.h"
#include "BackFrontEnd.h"

static void CodeBuild(TreeNode* node, NameTableType* allNamesTable, FILE* outStream, 
                                                                    size_t numberOfTabs);
static void PrintTabs(size_t numberOfTabs, FILE* outStream);

void CodeBuild(Tree* tree, FILE* outStream)
{
    assert(tree);
    assert(outStream);

    CodeBuild(tree->root, tree->allNamesTable, outStream, 0);
}

#define CODE_BUILD(NODE)           CodeBuild(NODE, allNamesTable, outStream, numberOfTabs)
#define CODE_BUILD_TAB_SHIFT(NODE) CodeBuild(NODE, allNamesTable, outStream, numberOfTabs + 1)

static void CodeBuild(TreeNode* node, NameTableType* allNamesTable, FILE* outStream, 
                                                                    size_t numberOfTabs)
{
    if (node == nullptr)
        return;
    
    if (node->valueType == TreeNodeValueType::NUM)
    {
        if (node->value.num < 0)
            fprintf(outStream, "(0 + %d) ", -node->value.num);
        else
            fprintf(outStream, "%d ", node->value.num);
            
        return;
    }

    if (node->valueType == TreeNodeValueType::NAME)
    {
        assert(!node->left && !node->right);

        fprintf(outStream, "%s ", allNamesTable->data[node->value.nameId].name);
        return;
    }

    if (node->valueType == TreeNodeValueType::STRING_LITERAL)
    {
        fprintf(outStream, "\"%s\" ", allNamesTable->data[node->value.nameId].name);
        return;
    }

    assert(node->valueType == TreeNodeValueType::OPERATION);

    switch (node->value.operation)
    {
        case TreeOperationId::NEW_FUNC:
        {
            CODE_BUILD(node->left);

            fprintf(outStream, "\n");

            CODE_BUILD(node->right);

            break;
        }

        case TreeOperationId::TYPE_INT:
        {
            fprintf(outStream, "575757 ");
            break;
        }

        case TreeOperationId::FUNC:
        {
            fprintf(outStream, "%s ", allNamesTable->data[node->left->value.nameId].name);

            assert(numberOfTabs == 0);

            CODE_BUILD_TAB_SHIFT(node->left->left);

            fprintf(outStream, "\n57\n");

            CODE_BUILD_TAB_SHIFT(node->left->right);

            fprintf(outStream, "{\n");

            break;
        }

        case TreeOperationId::LINE_END:
        {
            PrintTabs(numberOfTabs, outStream);

            CODE_BUILD(node->left);

            if (!(node->left->valueType == TreeNodeValueType::OPERATION && 
                  (node->left->value.operation == TreeOperationId::IF   ||
                   node->left->value.operation == TreeOperationId::WHILE)))
                fprintf(outStream, "57\n");

            CODE_BUILD(node->right);

            break;
        }

        case TreeOperationId::ASSIGN:
        {
            CODE_BUILD(node->left);

            fprintf(outStream, "== ");

            CODE_BUILD(node->right);

            break;
        }

        case TreeOperationId::READ:
        {
            fprintf(outStream, "{ ");
            
            break;
        }

        case TreeOperationId::PRINT:
        {
            fprintf(outStream, ". ");

            CODE_BUILD(node->left);

            break;
        }

        case TreeOperationId::FUNC_CALL:
        {
            fprintf(outStream, "%s { ", allNamesTable->data[node->left->value.nameId].name);
            
            CODE_BUILD(node->left->left);

            fprintf(outStream, "57 ");

            break;
        }

        case TreeOperationId::IF:
        {
            fprintf(outStream, "57? ");

            CODE_BUILD(node->left);

            fprintf(outStream, "57\n");
            PrintTabs(numberOfTabs, outStream);
            fprintf(outStream, "57\n");
             
            CODE_BUILD_TAB_SHIFT(node->right);

            PrintTabs(numberOfTabs, outStream);
            fprintf(outStream, "{\n");

            break;
        }

        case TreeOperationId::WHILE:
        {
            fprintf(outStream, "57! ");

            CODE_BUILD(node->left);

            fprintf(outStream, "57\n");
            PrintTabs(numberOfTabs, outStream);
            fprintf(outStream, "57\n");
             
            CODE_BUILD_TAB_SHIFT(node->right);

            PrintTabs(numberOfTabs, outStream);
            fprintf(outStream, "{\n");

            break;
        }

        case TreeOperationId::ADD:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "- ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::SUB:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "+ ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::MUL:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "/ ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::DIV:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "* ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::GREATER:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "< ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::GREATER_EQ:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "<= ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::LESS:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "> ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::LESS_EQ:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, ">= ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::EQ:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "!= ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::NOT_EQ:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "= ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::AND:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "or ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::OR:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, "and ");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::POW:
        {
            fprintf(outStream, "(");

            CODE_BUILD(node->left);

            fprintf(outStream, ") ^ (");

            CODE_BUILD(node->right);

            fprintf(outStream, ") ");

            break;
        }

        case TreeOperationId::SIN:
        {
            fprintf(outStream, "sin(");

            CODE_BUILD(node->left);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::COS:
        {
            fprintf(outStream, "cos(");

            CODE_BUILD(node->left);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::TAN:
        {
            fprintf(outStream, "tan(");

            CODE_BUILD(node->left);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::COT:
        {
            fprintf(outStream, "cot(");

            CODE_BUILD(node->left);

            fprintf(outStream, ") ");

            break;
        }
        case TreeOperationId::SQRT:
        {
            fprintf(outStream, "sqrt(");

            CODE_BUILD(node->left);

            fprintf(outStream, ") ");

            break;
        }

        default:
        {
            CODE_BUILD(node->left);
            CODE_BUILD(node->right);
            break;
        }
    }
}

static void PrintTabs(size_t numberOfTabs, FILE* outStream)
{
    for (size_t i = 0; i < numberOfTabs; ++i)
        fprintf(outStream, "    ");
}
