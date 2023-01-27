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

void genActionTable(const ActionTable &actionTable, ostream &oss) {
    stringstream def_actionTable;
    def_actionTable << "{\n";
    const string typeName[] = {"AT0", "AT1", "AT2", "AT3"};
    for (auto[key, action] : actionTable) {
        def_actionTable << fmt::format(
            "{{{{{}, {}}}, {{{}, {}}}}}, ", key.first, key.second,
            typeName[static_cast<int>(action.type)], action.tgt);
    }
    def_actionTable << "}";

    oss << "struct Action {\n"
           "  enum Type { kAction = 0, kReduce = 1, kGoto = 2, kAccept = 3 };\n"
           "  TYPE type; int tgt;\n"
           "};\n";
    oss << "using ActionTable = map<pair<int, int>, Action>;\n\n";
    oss << "#define AT0 Action::Type::kAction\n"
           "#define AT1 Action::Type::kReduce\n"
           "#define AT2 Action::Type::kGoto\n"
           "#define AT3 Action::Type::kAccept\n"
           "\n";
    oss << fmt::format("ActionTable actionTable = {};\n",
                       def_actionTable.str());
}

void genGrammar(const Grammar &grammar, ostream &oss) {
    const auto & symbolNames = grammar.symbolNames;

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
    for (int i = 0; i < grammar.prods.size(); i++) {
        const auto &prod = grammar.prods[i];
        def_prods_comment << fmt::format("// {:2d}: ", i);
        def_prods_comment << symbolNames.at(prod.symbol) << " -> ";
        for (int r : prod.right) {
            def_prods_comment << symbolNames.at(r) << " ";
        }
        def_prods_comment << "\n";
    }

    // prods
    stringstream def_prods;
    def_prods << "{\n";
    for (int i = 0; i < grammar.prods.size(); i++) {
        def_prods << fmt::format("  /* {:2d} */", i);
        const auto &prod = grammar.prods[i];
        // def_prods << symbolNames.at(prod.symbol) << " -> ";
        // for (int r : prod.right) { def_prods << symbolNames.at(r) << " "; }
        // def_prods << "*/\n";
        def_prods << " { " << symbolNames.at(prod.symbol) << ", {";
        for (int j = 0; j < prod.right.size(); j++) {
            def_prods << symbolNames.at(prod.right[j])
                      << ((j + 1 < prod.right.size()) ? ", " : "");
        }
        def_prods << "}}, \n";
    }
    def_prods << "}";

    // prodsPriority
    stringstream def_prodsPriority;
    def_prodsPriority << fmt::format("{{\n  {}\n}}", fmt::join(grammar.prodsPriority, ", "));

    // prodsAssociate
    // enum class Associate {kNone = 0, kLeft = 1, kRight = 2};
    stringstream def_prodsAssociate;
    map<Associate, string> assoName = {{Associate::kNone, "AS0"},
                                       {Associate::kLeft, "AS1"},
                                       {Associate::kRight, "AS2"}};
    def_prodsAssociate << fmt::format(
        "{{\n  {}\n}}", fmt::join(apply_map(grammar.prodsAssociate, assoName), ", "));

    oss << fmt::format("{}\n", def_symbols.str());
    oss << "struct Prod { int symbol; vector<int> right; };\n";
    oss << "enum class Associate {kNone = 0, kLeft = 1, kRight = 2};\n"
           "struct Grammar {\n  set<int> terminalSet;\n  set<int> nonterminalSet;\n"
           "  vector<Prod> prods;\n  map<int, string> symbolNames;\n  vector<int> "
           "prodsPriority;\n  vector<Associate> prodsAssociate;\n};\n\n"
           "#define AS0 Associate::kNone\n"
           "#define AS1 Associate::kLeft\n"
           "#define AS2 Associate::kRight\n\n"; 

    oss << fmt::format("map<int, string> symbolNames = {};\n\n",
                       def_symbolnames.str());
    oss << fmt::format("set<int> terminalSet = {};\n", def_terminals.str());
    oss << fmt::format("set<int> nonterminalSet = {};\n",
                       def_nonterminals.str());
    oss << fmt::format("vector<int> prodsPriority = {};\n", def_prodsPriority.str());
    oss << fmt::format("vector<Associate> prodsAssociate = {};\n", def_prodsAssociate.str());
    oss << "\n" << def_prods_comment.str() << "\n";
    oss << fmt::format("vector<Prod> prods = {};\n\n", def_prods.str());
    oss << "Grammar grammar({\n"
           "  .terminalSet=terminalSet,\n"
           "  .nonterminalSet=nonterminalSet,\n"
           "  .prods=prods,\n"
           "  .symbolNames=symbolNames,\n"
           "  .prodsPriority=prodsPriority,\n"
           "  .prodsAssociate=prodsAssociate,\n"
           "});\n";
}

void genDFA(const DFA &dfa, ostream &oss) {
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

    oss << "constexpr int EMPTY_SYMBOL = 0;\n"
        << "using DFAgraph = map<int, map<int, int>>;\n"
        << "struct DFA { DFAgraph graph; map<int, int> finality; };\n"
        << fmt::format("const DFAgraph graph = {};\n\n", def_dfa_graph.str());
    oss << fmt::format("const map<int, int> finality = {};\n\n",
                       def_finality.str());
    oss << fmt::format(
        "const DFA dfa({{.graph = graph, .finality = finality}});\n",
        def_finality.str());
}

void genSyntaxParser(const Grammar &grammar, ostream &oss) {
    auto actionTable = getLALR1table(grammar);

    oss << "// Action Table\n";
    genActionTable(actionTable, oss);
    oss << "\n";

    oss << "// Grammar\n";
    genGrammar(grammar, oss);

    oss << "SyntaxParser syntaxParser(grammar, actionTable);\n";
}

void genLexicalParser(const vector<string> &regexs, ostream &oss) {
    vector<DFA> dfas;
    for (string regex : regexs) { dfas.push_back(getDFAfromRegex(regex)); }
    DFA dfai = getDFAintegrated(dfas);

    for (int i = 0; i < regexs.size(); i++) {
        oss << fmt::format("// {:2d}: {}\n", i, regexs[i]);
    }
    oss << "\n";

    genDFA(dfai, oss);
    oss << "\n";
    oss << "LexicalParser lexicalParser(dfa);\n\n";

    stringstream def_func;
    def_func << "  switch (token.id) {\n";
    for (int i = 0; i < regexs.size(); i++) {
        def_func << fmt::format(
            "    case {:d}: // {:2d}: {}\n      return 0; // DIY\n", i, i, regexs[i]);
    }
    def_func << "    default:\n      assert(false);\n"; def_func << "}\n";

    oss << fmt::format("int getSyntaxId(Token token) {{\n{}\n}}\n",
                       def_func.str());
}

} // namespace krill::codegen