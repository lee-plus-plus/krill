// mini calculator
#include "krill/deps.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
using namespace krill;

// -------------------- parser --------------------

// Action Table
#define AkA Action::Type::kAction
#define ReA Action::Type::kReduce
#define GoA Action::Type::kGoto
#define AcA Action::Type::kAccept

ActionTable yourActionTable = {
    {{0, 40}, {AkA, 1}},    {{0, 45}, {AkA, 2}},    {{0, 260}, {GoA, 3}},
    {{0, 261}, {GoA, 4}},   {{0, 262}, {AkA, 5}},   {{1, 40}, {AkA, 1}},
    {{1, 45}, {AkA, 2}},    {{1, 260}, {GoA, 6}},   {{1, 261}, {GoA, 4}},
    {{1, 262}, {AkA, 5}},   {{2, 40}, {AkA, 1}},    {{2, 45}, {AkA, 2}},
    {{2, 261}, {GoA, 7}},   {{2, 262}, {AkA, 5}},   {{3, -1}, {AcA, 0}},
    {{3, 43}, {AkA, 8}},    {{3, 45}, {AkA, 9}},    {{4, -1}, {ReA, 1}},
    {{4, 41}, {ReA, 1}},    {{4, 42}, {AkA, 10}},   {{4, 43}, {ReA, 1}},
    {{4, 45}, {ReA, 1}},    {{4, 47}, {AkA, 11}},   {{5, -1}, {ReA, 8}},
    {{5, 41}, {ReA, 8}},    {{5, 42}, {ReA, 8}},    {{5, 43}, {ReA, 8}},
    {{5, 45}, {ReA, 8}},    {{5, 47}, {ReA, 8}},    {{6, 41}, {AkA, 12}},
    {{6, 43}, {AkA, 8}},    {{6, 45}, {AkA, 9}},    {{7, -1}, {ReA, 7}},
    {{7, 41}, {ReA, 7}},    {{7, 42}, {AkA, 10}},   {{7, 43}, {ReA, 7}},
    {{7, 45}, {ReA, 7}},    {{7, 47}, {AkA, 11}},   {{8, 40}, {AkA, 1}},
    {{8, 45}, {AkA, 2}},    {{8, 261}, {GoA, 13}},  {{8, 262}, {AkA, 5}},
    {{9, 40}, {AkA, 1}},    {{9, 45}, {AkA, 2}},    {{9, 261}, {GoA, 14}},
    {{9, 262}, {AkA, 5}},   {{10, 40}, {AkA, 1}},   {{10, 45}, {AkA, 2}},
    {{10, 261}, {GoA, 15}}, {{10, 262}, {AkA, 5}},  {{11, 40}, {AkA, 1}},
    {{11, 45}, {AkA, 2}},   {{11, 261}, {GoA, 16}}, {{11, 262}, {AkA, 5}},
    {{12, -1}, {ReA, 2}},   {{12, 41}, {ReA, 2}},   {{12, 42}, {ReA, 2}},
    {{12, 43}, {ReA, 2}},   {{12, 45}, {ReA, 2}},   {{12, 47}, {ReA, 2}},
    {{13, -1}, {ReA, 5}},   {{13, 41}, {ReA, 5}},   {{13, 42}, {AkA, 10}},
    {{13, 43}, {ReA, 5}},   {{13, 45}, {ReA, 5}},   {{13, 47}, {AkA, 11}},
    {{14, -1}, {ReA, 6}},   {{14, 41}, {ReA, 6}},   {{14, 42}, {AkA, 10}},
    {{14, 43}, {ReA, 6}},   {{14, 45}, {ReA, 6}},   {{14, 47}, {AkA, 11}},
    {{15, -1}, {ReA, 3}},   {{15, 41}, {ReA, 3}},   {{15, 42}, {ReA, 3}},
    {{15, 43}, {ReA, 3}},   {{15, 45}, {ReA, 3}},   {{15, 47}, {ReA, 3}},
    {{16, -1}, {ReA, 4}},   {{16, 41}, {ReA, 4}},   {{16, 42}, {AkA, 10}},
    {{16, 43}, {ReA, 4}},   {{16, 45}, {ReA, 4}},   {{16, 47}, {ReA, 4}},
};

// Grammar
constexpr int Q = 259;
constexpr int P = 260;
constexpr int T = 261;
constexpr int d = 262;

#define NoA Associate::kNone
#define LeA Associate::kLeft
#define RiA Associate::kRight

const map<int, string> yourSymbolNames = {
    {-1, "ζ"},   {0, "ε"},    {40, "'('"}, {41, "')'"},
    {42, "'*'"}, {43, "'+'"}, {45, "'-'"}, {47, "'/'"},
    {259, "Q"},  {260, "P"},  {261, "T"},  {262, "d"},
};

const set<int>          yourTerminalSet    = {40, 41, 42, 43, 45, 47, 262};
const set<int>          yourNonterminalSet = {259, 260, 261};
const vector<int>       yourProdsPriority  = {1, 2, 3, 4, 5, 6, 7, 8, 9};
const vector<Associate> yourProdsAssociate = {NoA, NoA, NoA, NoA, NoA,
                                              NoA, NoA, NoA, NoA};

/** productions:
 *  0: Q -> P
 *  1: P -> T
 *  2: T -> '(' P ')'
 *  3: T -> T '*' T
 *  4: T -> T '/' T
 *  5: P -> P '+' T
 *  6: P -> P '-' T
 *  7: T -> '-' T
 *  8: T -> d
 **/

const vector<Prod> yourProds = {
    {Q, {P}},         {P, {T}},         {T, {'(', P, ')'}},
    {T, {T, '*', T}}, {T, {T, '/', T}}, {P, {P, '+', T}},
    {P, {P, '-', T}}, {T, {'-', T}},    {T, {d}},
};

const Grammar yourGrammar(yourTerminalSet, yourNonterminalSet, yourProds,
                          yourSymbolNames, yourProdsPriority,
                          yourProdsAssociate);

// -------------------- lexer --------------------

/** regex:
 *  0: [\-\+]0|[1-9][0-9]*
 *  1: \+
 *  2: \-
 *  3: \*
 *  4: \/
 *  5: \(
 *  6: \)
 *  7: [ ]+
 **/
const DFAgraph yourDFAgraph = {
    {0,
     {{32, 1},
      {40, 2},
      {41, 3},
      {42, 4},
      {43, 5},
      {45, 6},
      {47, 7},
      {49, 8},
      {50, 8},
      {51, 8},
      {52, 8},
      {53, 8},
      {54, 8},
      {55, 8},
      {56, 8},
      {57, 8}}},
    {1, {{32, 1}}},
    {5, {{48, 9}}},
    {6, {{48, 9}}},
    {8,
     {{48, 8},
      {49, 8},
      {50, 8},
      {51, 8},
      {52, 8},
      {53, 8},
      {54, 8},
      {55, 8},
      {56, 8},
      {57, 8}}},
};

const map<int, int> yourFinality = {{0, 0}, {1, 8}, {2, 6}, {3, 7}, {4, 4},
                                    {5, 2}, {6, 3}, {7, 5}, {8, 1}, {9, 1}};

const DFA yourDfa({.graph = yourDFAgraph, .finality = yourFinality});

class YourLexer : public Lexer {
  public:
    YourLexer() : Lexer(yourDfa) {}
};

// -------------------- main --------------------


class YourParser : public Parser {
  public:
    YourParser() : Parser(yourGrammar, yourActionTable) {}

    void onAction(AstNode *node);
    void onReduce(AstNode *node);
    void onAccept();
    void onError();
};

void YourParser::onAction(AstNode *node) {
    if (node->id == d) { node->attr.RefN<double>("value") = stoi(node->lval); }
}

void YourParser::onReduce(AstNode *node) {
    AttrDict *         attr = &node->attr;
    vector<AttrDict *> child;
    for (auto &node : node->child) { child.push_back(&node.get()->attr); }

    switch (node->pidx) {
    case 0: // Q -> P
    case 1: // P -> T
    case 8: // T -> d
        attr->RefN<double>("value") = child[0]->Ref<double>("value");
        break;
    case 2: // T -> ( P )
        attr->RefN<double>("value") = child[1]->Ref<double>("value");
        break;
    case 3: // T -> T * T
        attr->RefN<double>("value") =
            child[0]->Ref<double>("value") * child[2]->Ref<double>("value");
        break;
    case 4: // T -> T / T
        attr->RefN<double>("value") =
            child[0]->Ref<double>("value") / child[2]->Ref<double>("value");
        break;
    case 5: // P -> T + T
        attr->RefN<double>("value") =
            child[0]->Ref<double>("value") + child[2]->Ref<double>("value");
        break;
    case 6: // P -> T - T
        attr->RefN<double>("value") =
            child[0]->Ref<double>("value") - child[2]->Ref<double>("value");
        break;
    case 7: // T -> - T
        attr->RefN<double>("value") = -child[1]->Ref<double>("value");
        break;
    default:
        assert(false);
    }
}

void YourParser::onAccept() {
    auto node = nodes_.top();
    spdlog::info("result: {}", node.get()->attr.Ref<double>("value"));
}

void YourParser::onError() {}

// DIY
int getSyntaxId(Token token) {
    switch (token.id) {
    case -1: // end of input
        return -1;
    case 0: // [\-\+]0|[1-9][0-9]*
        return d;
    case 1: // \+
        return '+';
    case 2: // \-
        return '-';
    case 3: // \*
        return '*';
    case 4: // \/
        return '/';
    case 5: // \(
        return '(';
    case 6: // \)
        return ')';
    case 7: // [ ]+
        return -2;
    default:
        assert(false);
        return -2;
    }
}

int main() {
    YourLexer  lexer;
    YourParser parser;

    while (true) {
        lexer.clear();
        parser.clear();

        cerr << "> ";
        string line;

        getline(cin, line);
        if (cin.eof()) { break; }
        if (line.size() == 0) { continue; }

        stringstream ss;
        ss << line;

        while (!parser.isAccepted()) {
            Token token = lexer.parseStep(ss);
            spdlog::info("<token {:d}> \"{}\"", token.id, token.lval);

            int syntaxId = getSyntaxId(token);
            if (syntaxId == -2) { continue; }
            token.id = syntaxId;

            parser.parseStep(token);
        }

        auto root = parser.getAstRoot();
        cout << AstPrinter{}.showAttr().print(root.get());
    }
}
