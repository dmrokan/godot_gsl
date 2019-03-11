#ifndef GODOT_GSL_PARSER_H
#define GODOT_GSL_PARSER_H

#include "modules/gdscript/gdscript_parser.h"
#include "godot_gsl_function.h"

typedef GDScriptParser::Node GSLNode;
typedef GDScriptParser::ClassNode GSLCNode;
typedef GDScriptParser::FunctionNode GSLFNode;
typedef GDScriptParser::BlockNode GSLBNode;
typedef GDScriptParser::LocalVarNode GSLLVNode;
typedef GDScriptParser::OperatorNode GSLONode;

class GodotGSLParser {
public:
    GodotGSLParser() { parser = memnew(GDScriptParser); }
    ~GodotGSLParser();
    void parse(const String &code);
    void traverse();

private:
    void traverse_block(GSLBNode *block);
    GDScriptParser *parser;
    Map<String, GodotGSLFunction*> functions;
};

#endif
