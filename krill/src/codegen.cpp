#include "krill/codegen.h"
#include "krill/automata.h"
#include "krill/grammar.h"
#include "krill/regex.h"
#include "krill/utils.h"
#include <fmt/format.h>
#include <sstream>
using namespace krill::type;
using namespace krill::regex;
using krill::automata::getDFAintegrated;
using krill::grammar::getLALR1table;
using namespace krill::utils;
using namespace std;

namespace krill::codegen {

/** give output as follow:
 *
 * #define AkA Action::Type::kAction
 * #define ReA Action::Type::kReduce
 * #define GoA Action::Type::kGoto
 * #define AcA Action::Type::kAccept
 *
 * const ActionTable yourActionTable = {
 *   {{1,2},{AT0,3}}, {{1,2},{AT0,3}}, {{1,2},{AT0,3}}
 * };
 **/
void genActionTableCode(const ActionTable &actionTable, ostream &oss) {
    stringstream def_actionTable;
    def_actionTable << "{\n  ";
    const string typeName[] = {"AkA", "ReA", "GoA", "AcA"};
    for (auto[key, action] : actionTable) {
        def_actionTable << fmt::format(
            "{{{{{},{}}},{{{},{}}}}}, ", key.first, key.second,
            typeName[static_cast<int>(action.type)], action.tgt);
    }
    def_actionTable << "}";

    oss << "#define AkA Action::Type::kAction\n"
           "#define ReA Action::Type::kReduce\n"
           "#define GoA Action::Type::kGoto\n"
           "#define AcA Action::Type::kAccept\n"
           "\n";
    oss << "ActionTable actionTable = " << def_actionTable.str() << "\n";
}

/** give output as follow:
 *
 * constexpr int VOID = 1;
 * constexpr int INT  = 2;
 *
 * #define NoA Associate::kNone
 * #define LeA Associate::kLeft
 * #define RiA Associate::kRight
 *
 * map<int, string> yourSymbolNames = { ... };
 * set<int> yourTerminalSet = { ... };
 * set<int> yourNonterminalSet = { ... };
 * vector<int> yourProdsPriority = { ... };
 * vector<Associate> yourProdsAssociate = { ... };
 *
 * vector<Prod> yourProds = {
 *     {program, {decl_list}},
 *     {decl_list, {decl_list, decl}},
 *     ...
 * };
 *
 * const Grammar yourGrammar(
 *   .terminalSet = yourSymbolNames,
 *   .nonterminalSet = yourTerminalSet,
 *   .prods = yourProds,
 *   .symbolNames = yourNonterminalSet,
 *   .prodsPriority = yourProdsPriority,
 *   .prodsAssociate = yourProdsAssociate,
 * );
 **/
void genGrammarCode(const Grammar &grammar, ostream &oss) {
    const auto &symbolNames = grammar.symbolNames;

    // define of symbols
    stringstream def_symbols;
    for (auto[id, name] : grammar.symbolNames) {
        if (id < 256) { continue; }
        def_symbols << fmt::format("constexpr int {} = {};\n", name, id);
    }

    // symbol names
    stringstream def_symbolnames;
    def_symbolnames << "{\n";
    for (auto[id, name] : grammar.symbolNames) {
        def_symbolnames << fmt::format("  {{{}, \"{}\"}}, \n", id, name);
    }
    def_symbolnames << "}";

    // terminals & nonterminals
    stringstream def_terminals;
    stringstream def_nonterminals;
    def_terminals << fmt::format(
        "{{\n  {}\n}}", fmt::join(to_vector(grammar.terminalSet), ", "));
    def_nonterminals << fmt::format(
        "{{\n  {}\n}}", fmt::join(to_vector(grammar.nonterminalSet), ", "));

    // prods comment
    stringstream def_prods_comment;
    def_prods_comment << "/** productions:\n";
    for (int i = 0; i < grammar.prods.size(); i++) {
        const auto &prod = grammar.prods[i];
        def_prods_comment << fmt::format(" * {:2d}: ", i);
        def_prods_comment << symbolNames.at(prod.symbol) << " -> ";
        for (int r : prod.right) {
            def_prods_comment << symbolNames.at(r) << " ";
        }
        def_prods_comment << "\n";
    }
    def_prods_comment << " **/\n";

    // prods
    stringstream def_prods;
    def_prods << "{\n";
    for (int i = 0; i < grammar.prods.size(); i++) {
        const auto &prod = grammar.prods[i];
        // def_prods << symbolNames.at(prod.symbol) << " -> ";
        // for (int r : prod.right) { def_prods << symbolNames.at(r) << " "; }
        // def_prods << "*/\n";
        def_prods << "  {" << symbolNames.at(prod.symbol) << ", {";
        for (int j = 0; j < prod.right.size(); j++) {
            def_prods << symbolNames.at(prod.right[j])
                      << ((j + 1 < prod.right.size()) ? ", " : "");
        }
        def_prods << "}}, \n";
    } // namespace krill::codegen
    def_prods << "}";

    // prodsPriority
    stringstream def_prodsPriority;
    def_prodsPriority << fmt::format("{{\n  {}\n}}",
                                     fmt::join(grammar.prodsPriority, ", "));

    // prodsAssociate
    // enum class Associate {kNone = 0, kLeft = 1, kRight = 2};
    stringstream           def_prodsAssociate;
    map<Associate, string> assoName = {{Associate::kNone, "NoA"},
                                       {Associate::kLeft, "LeA"},
                                       {Associate::kRight, "RiA"}};
    def_prodsAssociate << fmt::format(
        "{{\n  {}\n}}",
        fmt::join(apply_map(grammar.prodsAssociate, assoName), ", "));

    oss << fmt::format("{}\n", def_symbols.str());
    oss << "#define NoA Associate::kNone\n"
           "#define LeA Associate::kLeft\n"
           "#define RiA Associate::kRight\n\n";

    oss << fmt::format("const map<int, string> yourSymbolNames = {};\n\n",
                       def_symbolnames.str());
    oss << fmt::format("const set<int> yourTerminalSet = {};\n",
                       def_terminals.str());
    oss << fmt::format("const set<int> yourNonterminalSet = {};\n",
                       def_nonterminals.str());
    oss << fmt::format("const vector<int> yourProdsPriority = {};\n",
                       def_prodsPriority.str());
    oss << fmt::format("const vector<Associate> yourProdsAssociate = {};\n",
                       def_prodsAssociate.str());
    oss << "\n" << def_prods_comment.str() << "\n";
    oss << fmt::format("const vector<Prod> yourProds = {};\n\n",
                       def_prods.str());
    oss << "const Grammar yourGrammar({\n"
           "  .terminalSet=yourTerminalSet,\n"
           "  .nonterminalSet=yourNonterminalSet,\n"
           "  .prods=yourProds,\n"
           "  .symbolNames=yourSymbolNames,\n"
           "  .prodsPriority=yourProdsPriority,\n"
           "  .prodsAssociate=yourProdsAssociate,\n"
           "});\n";
}

/** give output as follow:
 *
 * const DFAgraph yourDFAgraph = { ... };
 * const map<int, int> yourFinality = { ... };
 * const DFA yourDfa({{.graph = yourDFAgraph, .finality = yourFinality}});
 * };
 **/
void genDfaCode(const DFA &dfa, ostream &oss) {
    stringstream def_dfa_graph;
    def_dfa_graph << "{\n";
    int i;

    for (auto[from, map] : dfa.graph) {
        def_dfa_graph << "  {" << from << ", {";
        i = 0;
        for (auto[symbol, to] : map) {
            def_dfa_graph << (i++ == 0 ? "" : ",");
            def_dfa_graph << "{" << symbol << "," << to << "}";
        }
        def_dfa_graph << "}},\n";
    }
    def_dfa_graph << "}";

    stringstream def_finality;
    def_finality << "{\n  ";

    i = 0;
    for (auto[state, f] : dfa.finality) {
        def_finality << (i++ == 0 ? "" : ",");
        def_finality << "{" << state << "," << f << "}";
    }
    def_finality << "\n}";

    oss << fmt::format("const DFAgraph yourDFAgraph = {};\n\n",
                       def_dfa_graph.str());
    oss << fmt::format("const map<int, int> yourFinality = {};\n\n",
                       def_finality.str());
    oss << fmt::format("const DFA yourDfa({{.graph = yourDFAgraph, .finality = "
                       "yourFinality}});\n",
                       def_finality.str());
}

/** give output as follow:
 *
 * // ActionTable
 * ...
 * // Grammar
 * ...
 * class YourParser : public Parser {
 *   public:
 *     YourParser(): Parser(yourGrammar, yourActionTable) {}
 * };
 **/
void genParserCode(const Grammar &grammar, ostream &oss) {
    auto actionTable = getLALR1table(grammar);

    oss << "// Action Table\n";
    genActionTableCode(actionTable, oss);
    oss << "\n";

    oss << "// Grammar\n";
    genGrammarCode(grammar, oss);

    oss << "class YourParser : public Parser {\n"
           "  public:\n"
           "    YourParser(): Parser(yourGrammar, yourActionTable) {}\n"
           "};\n";
}

/** give output as follow:
 *
 * // ActionTable
 * ...
 * // Grammar
 * ...
 * class YourLexer : public Lexer {
 *   public:
 *     YourLexer(): Lexer(yourDfa) {}
 * };
 *
 * int getSyntaxId(Token token) {
 *   switch (token.id) {
 *     case 0: //  0: [0-9]+
 *       return 0; // DIY
 *     case 0: //  0: [0-9]+
 *       return 0; // DIY
 *     case 0: //  0: [0-9]+
 *       return 0; // DIY
 *     ...
 *     default:
 *       assert(false);
 *       return -1;
 *   }
 * }
 **/
void genLexerCode(const vector<string> &regexs, ostream &oss) {
    vector<DFA> dfas;
    for (string regex : regexs) { dfas.push_back(getDFAfromRegex(regex)); }
    DFA dfai = getDFAintegrated(dfas);

    oss << "/**\n";
    for (int i = 0; i < regexs.size(); i++) {
        oss << fmt::format(" * {:2d}: {}\n", i, regexs[i]);
    }
    oss << " **/\n";

    genDfaCode(dfai, oss);
    oss << "\n";
    oss << "class YourLexer : public Lexer {\n"
           "  public:\n"
           "    YourLexer(): Lexer(yourDfa) {}\n"
           "};\n\n";

    stringstream def_func;
    def_func << "  switch (token.id) {\n";
    for (int i = 0; i < regexs.size(); i++) {
        def_func << fmt::format("    case {:d}: // {}\n      return 0;\n", i,
                                regexs[i]);
    }
    def_func << "    default:\n      assert(false);\n      return -1;\n  }";

    oss << fmt::format("// DIY\nint getSyntaxId(Token token) {{\n{}\n}}\n",
                       def_func.str());
}

} // namespace krill::codegen