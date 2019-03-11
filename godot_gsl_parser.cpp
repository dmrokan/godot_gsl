#include "godot_gsl_parser.h"

GodotGSLParser::~GodotGSLParser()
{
    memdelete(parser);
}

void GodotGSLParser::parse(const String &code)
{
    parser->parse(code);
}

void GodotGSLParser::traverse()
{
    const GSLNode *head = parser->get_parse_tree();

    printf("Node type %d\n", head->type);
    if (head->type == GDScriptParser::Node::TYPE_CLASS)
    {
        Vector<GSLFNode *> functions = ((GSLCNode *) head)->functions;

        for (int k = 0; k < functions.size(); k++)
        {
            GSLFNode *fnode = functions.get(k);
            printf("Node type %d\n", fnode->type);

            GodotGSLFunction *fnc = memnew(GodotGSLFunction((String) fnode->name));

            GSLBNode *bnode = fnode->body;
            traverse_block(bnode);
        }
    }
}

void GodotGSLParser::traverse_block(GSLBNode *block)
{
    static const int TYPE_LOCAL_VAR = GDScriptParser::Node::TYPE_LOCAL_VAR;
    static const int TYPE_OPERATOR = GDScriptParser::Node::TYPE_OPERATOR;

    List<GSLNode *> statements = block->statements;

    for (int k = 0; k < statements.size(); k++)
    {
        GSLNode *node = statements[k];

        /* See gdscript_parser.h line 121 for node types */
        printf("Node type %d\n", node->type);

        if (node->type == TYPE_LOCAL_VAR)
        {
            GSLLVNode *lvnode = (GSLLVNode *) node;
            printf("  Local var name %s\n", ((String) lvnode->name).utf8().get_data());
            printf("  Assign type %d\n", lvnode->assign->type);
        }
        else if (node->type == TYPE_OPERATOR)
        {
            GSLONode *onode = (GSLONode *) node;
            printf("  Operator node type %d\n", onode->type);

            for (int l = 0; l < onode->arguments.size(); l++)
            {
                GSLNode *arg = onode->arguments[l];
                printf("    Argument type %d\n", arg->type);
            }
        }
    }
}
