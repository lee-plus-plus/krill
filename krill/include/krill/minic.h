#ifndef MINIC_H
#define MINIC_H
#include "defs.h"
#include "lexical.h"
#include "syntax.h"
#include <deque>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stack>
#include <string>
using krill::runtime::SyntaxParser, krill::runtime::LexicalParser;
using krill::type::Grammar;
using std::shared_ptr;
using std::stringstream;

namespace krill::minic {

class MinicSyntaxParser : public SyntaxParser {
  public:
    MinicSyntaxParser();
};

class MinicLexicalParser : public LexicalParser {
  public:
    MinicLexicalParser();
};

class MinicParser {
  private:
    MinicSyntaxParser  syntaxParser_;
    MinicLexicalParser lexicalParser_;

    int col_ = 1;
    int row_ = 1;

    stringstream         source_;
    vector<stringstream> sourceLines_;

    shared_ptr<APTnode> root_; // magic, don't touch
    vector<APTnode> nodes_;

    void    count(char c);
    APTnode tokenToNode(Token token, istream &input, bool &drop);

  public:
    MinicParser();

    string getLocatedSource(int colSt, int rowSt, int colEd, int rowEd);
    void   parseAll(istream &input);
    void   parseStep(istream &input);

    shared_ptr<APTnode> getAptNode() const;
    vector<APTnode>     getNodes() const;
};

extern Grammar       minicGrammar;
// extern SyntaxParser  minicSyntaxParser;
// extern LexicalParser minicLexicalParser;
// extern MinicParser   minicParser;

} // namespace krill::minic

namespace krill::minic::syntax {

// Grammar
// constexpr int ζ = -1;
constexpr int IDENT          = 258;
constexpr int VOID           = 259;
constexpr int INT            = 260;
constexpr int WHILE          = 261;
constexpr int IF             = 262;
constexpr int ELSE           = 263;
constexpr int RETURN         = 264;
constexpr int EQ             = 265;
constexpr int NE             = 266;
constexpr int LE             = 267;
constexpr int GE             = 268;
constexpr int AND            = 269;
constexpr int OR             = 270;
constexpr int DECNUM         = 271;
constexpr int CONTINUE       = 272;
constexpr int BREAK          = 273;
constexpr int HEXNUM         = 274;
constexpr int LSHIFT         = 275;
constexpr int RSHIFT         = 276;
constexpr int program        = 277;
constexpr int decl_list      = 278;
constexpr int decl           = 279;
constexpr int var_decl       = 280;
constexpr int fun_decl       = 281;
constexpr int type_spec      = 282;
constexpr int int_literal    = 283;
constexpr int FUNCTION_IDENT = 284;
constexpr int params         = 285;
constexpr int compound_stmt  = 286;
constexpr int param_list     = 287;
constexpr int param          = 288;
constexpr int stmt_list      = 289;
constexpr int stmt           = 290;
constexpr int expr_stmt      = 291;
constexpr int block_stmt     = 292;
constexpr int if_stmt        = 293;
constexpr int while_stmt     = 294;
constexpr int return_stmt    = 295;
constexpr int continue_stmt  = 296;
constexpr int break_stmt     = 297;
constexpr int expr           = 298;
constexpr int args_          = 299;
constexpr int WHILE_IDENT    = 300;
constexpr int local_decls    = 301;
constexpr int local_decl     = 302;
constexpr int arg_list       = 303;

} // namespace krill::minic::syntax

#endif
