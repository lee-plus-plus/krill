#ifndef SYNTAX_H
#define SYNTAX_H
#include "attrdict.h"
#include "defs.h"
#include "grammar.h"
#include <deque>
#include <memory>
#include <ostream>
#include <stack>
#include <string>
#include <vector>
using krill::type::Grammar, krill::type::ActionTable;
using krill::type::Token, krill::type::AstNode;
using krill::utils::AttrDict;
using std::shared_ptr;
using std::string, std::ostream;
using std::vector, std::deque, std::stack;

namespace krill::type {

struct AstNode;
using AstNodePtr = shared_ptr<AstNode>;

// Annotated Parsing Tree Node
struct AstNode {
    int               id;      // syntax id
    int               pidx;    // production id, -1 if not from reducution
    string            symname; // grammar symbol name
    string            lval;    // lexical value
    AttrDict          attr;
    deque<AstNodePtr> child;

    string str() const;
};

} // namespace krill::type

namespace krill::runtime {

class AstPrinter {
private:
    bool showColor_ = false;
    bool showAttrs_ = false;
    bool skipMidNodes_ = true;

    void print_(const AstNode * const node, vector<bool> isLast, ostream &oss);
public: 
    AstPrinter() = default;

    AstPrinter &showColor(bool flag = true);
    AstPrinter &showAttrs(bool flag = true);
    AstPrinter &skipMidNodes(bool flag = true);

    string print(const AstNode * const root);
    string print(const vector<shared_ptr<AstNode>> &nodes);
};

class SyntaxParser {
  private:
    const Grammar     grammar_;
    const ActionTable actionTable_;

    vector<Token>              inputs_;  // input
    stack<int>                 states_;  // lr(1) states
    stack<int>                 symbols_; // lr(1) symbols, for debug
    stack<shared_ptr<AstNode>> nodes_;   // parsed result

    int  offset_;
    bool isAccepted_;

    void   parse();
    string getErrorMessage();

    // DIY
    void onError();
    void onReduce(AstNode &node);
    void onAction(AstNode &node);
    void onAccept();

  public:
    // SyntaxParser() = default;
    SyntaxParser(const Grammar &grammar, const ActionTable &actionTable);

    void clear();
    void parseStep(Token token);
    // void parseStep(AstNode tokenWithAttr);
    void parseAll(vector<Token> tokens);
    // void parseAll(vector<AstNode> tokensWithAttr);

    shared_ptr<AstNode> getAstRoot();
};

} // namespace krill::runtime
#endif
