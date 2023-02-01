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

class MinicGrammar : public Grammar {
  public:
    MinicGrammar();
};

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

    int col_lex_ = 1;
    int row_lex_ = 1;
    int col_syn_ = 1;
    int row_syn_ = 1;

    stringstream         source_;
    vector<stringstream> sourceLines_;

    shared_ptr<APTnode> root_; // magic, don't touch
    vector<APTnode>     nodes_;

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

extern Grammar minicGrammar;
// extern SyntaxParser  minicSyntaxParser;
// extern LexicalParser minicLexicalParser;
// extern MinicParser   minicParser;

} // namespace krill::minic

namespace krill::minic::syntax {

// Grammar
// constexpr int ζ = -1;
constexpr int IDENT          = 258;
constexpr int VOID_          = 259;
constexpr int INT_           = 260;
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
constexpr int FOR            = 277;
constexpr int program        = 278;
constexpr int decl_list      = 279;
constexpr int decl           = 280;
constexpr int var_decl       = 281;
constexpr int fun_decl       = 282;
constexpr int type_spec      = 283;
constexpr int int_literal    = 284;
constexpr int init_list      = 285;
constexpr int FUNCTION_IDENT = 286;
constexpr int params         = 287;
constexpr int compound_stmt  = 288;
constexpr int param_list     = 289;
constexpr int param          = 290;
constexpr int stmt_list      = 291;
constexpr int stmt           = 292;
constexpr int expr_stmt      = 293;
constexpr int block_stmt     = 294;
constexpr int if_stmt        = 295;
constexpr int while_stmt     = 296;
constexpr int for_stmt       = 297;
constexpr int return_stmt    = 298;
constexpr int continue_stmt  = 299;
constexpr int break_stmt     = 300;
constexpr int expr           = 301;
constexpr int args_          = 302;
constexpr int WHILE_IDENT    = 303;
constexpr int local_decls    = 304;
constexpr int local_decl     = 305;
constexpr int arg_list       = 306;

} // namespace krill::minic::syntax

#endif
