#include "krill/codegen.h"
#include "krill/grammar.h"
#include "krill/utils.h"
#include <fmt/core.h>
#include <sstream>
using namespace krill::grammar;
using namespace std;

namespace krill::codegen {

const char codeSyntaxParserInCStyle[] = R"(
#include <assert.h>
#include <stdio.h>

typedef struct {{ int value; }} GrammarNode; // DIY

GrammarNode syntaxParser(const int num_tokens, int *tokens, int *values) {{
  typedef struct {{ int symbol; int len_right; int *right; }} Prod;
  enum TYPE {{ACTION=0, REDUCE=1, GOTO=2, ACCEPT=3}};
  
  #define num_states /* FILL-0 */ {}
  #define num_symbols /* FILL-1 */ {}
  #define num_action_items /* FILL-2 */ {}
  #define num_prods /* FILL-3 */ {}

  Prod prods[num_prods] = {{
    /* FILL-4 */ {} 
  }};
  int dfa_table[num_states][num_symbols] = {{
    /* FILL-5 */ {} 
  }};
  enum TYPE actions_type[num_action_items] = {{
    /* FILL-6 */ {} 
  }};
  int actions_tgt[num_action_items] = {{
    /* FILL-7 */ {} 
  }};
  assert(tokens[num_tokens - 1] == -1);

  int states[num_tokens]; // stack
  int len_states = 0;
  GrammarNode state_nodes[num_tokens]; // DIY
  int len_state_nodes = 0;
  states[len_states++] = 0;

  for (int i = 0, flag = 1; flag; ) {{
    int now = (tokens[i] != -1) ? tokens[i] : num_symbols - 1;

    // look tokens[i], state => next_state, action
    assert(dfa_table[states[len_states - 1]][now] != -1);
    enum TYPE action_type = actions_type[ dfa_table[states[len_states - 1]][now] ];
    int action_tgt = actions_tgt[ dfa_table[states[len_states - 1]][now] ];

    switch (action_type) {{
    case ACTION: {{
      states[len_states++] = action_tgt;
      state_nodes[len_state_nodes++].value = values[i]; // DIY
      i++;
      break;
    }}
    case REDUCE: {{
      Prod r = prods[action_tgt];
      GrammarNode child_nodes[r.len_right];
      for (int j = 0; j < (int) r.len_right; j++) {{
        len_states--;
        child_nodes[r.len_right - j - 1] = state_nodes[--len_state_nodes];
      }}

      assert(dfa_table[states[len_states - 1]][r.symbol] != -1);
      enum TYPE action2_type = actions_type[ dfa_table[states[len_states - 1]][r.symbol] ];
      int action2_tgt = actions_tgt[ dfa_table[states[len_states - 1]][r.symbol] ];
      assert(action2_type == GOTO);
      states[len_states++] = action2_tgt;
        GrammarNode nextNode;
        switch (action_tgt) {{
          /* FILL-8 */ {} 
          default: {{
            assert(0);
            break;
          }}
        }}
        state_nodes[len_state_nodes++] = nextNode;
        break;
      }}
      case ACCEPT: {{
        flag = 0;
        break;
      }}
      default: {{
        assert(0);
        break;
      }}
    }}       
  }}
  assert(len_state_nodes == 1);
  return state_nodes[0];
}}
)";

const char codeSyntaxParserInCppStyle[] = R"(
#include <cassert>
#include <map>
#include <stack>
#include <vector>
using std::pair, std::vector, std::map, std::stack;

struct GrammarNode {{ int value; }}; // DIY

GrammarNode syntaxParser(vector<int> tokens) {{
  // declaration of struct
  struct Prod {{ int symbol; vector<int> right; }};
  enum TYPE {{ACTION, REDUCE, GOTO, ACCEPT}};
  struct Action {{ TYPE type; int tgt; }};
  typedef map<pair<int, int>, Action> ActionTable;

  // prods
  vector<Prod> prods = {{
      /* FILL-0 */ {}
  }}; 

  // Action Table
  ActionTable actionTable = {{
      /* FILL-1 */ {}
  }};

  if (tokens[tokens.size() - 1] != -1) {{
    tokens.push_back(-1);
  }}

  stack<int> states;
  stack<GrammarNode> stateNodes; // DIY
  states.push(0);

  for (int i = 0, accpeted = false; !accpeted; ) {{
    // look tokens[i], state => next_state, action
    assert(actionTable.count({{states.top(), tokens[i]}}) != 0);

    Action action = actionTable[{{states.top(), tokens[i]}}];
    switch (action.type) {{
      case ACTION: {{
        states.push(action.tgt);
        stateNodes.push(GrammarNode({{ tokens[i] }})); // DIY
        i++;
        break;
      }}
      case REDUCE: {{
        Prod r = prods[action.tgt];
        vector<GrammarNode> childNodes;
        for (int j = 0; j < (int) r.right.size(); j++) {{
          states.pop();
          childNodes.insert(childNodes.begin(), stateNodes.top());
          stateNodes.pop();
        }}

        assert(actionTable.count({{states.top(), r.symbol}}) != 0);
        Action action2 = actionTable[{{states.top(), r.symbol}}];
        assert(action2.type == GOTO);
        states.push(action2.tgt);

        GrammarNode nextNode;
        switch (action.tgt) {{
          /* FILL-2 */ {}
          default: {{
            assert(false);
            break;
          }}
        }}
        stateNodes.push(nextNode);
        break;
      }}
      case ACCEPT: {{
        accpeted = true;
        break;
      }}
      default: {{
        assert(false);
        break;
      }}
    }}       
  }}
  assert(stateNodes.size() == 1);
  return stateNodes.top();
}} 
)";

// generate code of Syntax Parser (C format, standalone)
void genSyntaxParserInCStyle(const Grammar &grammar,
                             map<int, string> symbolNames,
                             const ActionTable &actionTable, ostream &oss) {

    // num_states, num_symbols, num_action_items
    stringstream def_num_states;
    stringstream def_num_symbols;
    stringstream def_num_action_items;
    stringstream def_num_prods;

    int num_states = 0;
    for (auto[key, action] : actionTable) {
        num_states = max(num_states, key.first + 1);
    }
    int end_symbol       = symbolNames.size(); // -1, map to num_symbols-1
    int num_symbols      = symbolNames.size() + 1;
    int num_action_items = actionTable.size();
    int num_prods        = grammar.prods.size();

    def_num_states << num_states;
    def_num_symbols << num_symbols;
    def_num_action_items << num_action_items;
    def_num_prods << num_prods;

    // prods
    stringstream def_prods;
    def_prods << endl;
    for (int i = 0; i < num_prods; i++) {
        const auto &prod = grammar.prods[i];
        def_prods << "    /* " << i << ": ";
        def_prods << symbolNames[prod.symbol] << " -> ";
        for (int r : prod.right) { def_prods << symbolNames[r] << " "; }
        def_prods << "*/ {" << prod.symbol << ", ";
        def_prods << prod.right.size() << ", ";
        def_prods << "(int [" << prod.right.size() << "]) {";
        for (int j = 0; j < prod.right.size(); j++) {
            def_prods << prod.right[j]
                      << ((j + 1 < prod.right.size()) ? ", " : "");
        }
        def_prods << "}}, " << endl;
    }

    // action_table => dfa_table, actions_type, actions_tgt
    // (compressed storage)
    const string typeName[] = {"ACTION", "REDUCE", "GOTO", "ACCEPT"};
    int **dfa_table         = new int *[num_states];
    for (int i = 0; i < num_states; i++) {
        dfa_table[i] = new int[num_symbols];
        for (int j = 0; j < num_symbols; j++) { dfa_table[i][j] = -1; }
    }

    Action *actions = new Action[num_action_items];
    {
        int i = 0;
        for (auto[key, action] : actionTable) {
            // if (!(key.first < num_states && key.second < num_symbols)) {
            //     cout << key.first << ", " << key.second << endl;
            // }
            if (key.second != -1) {
                dfa_table[key.first][key.second] = i;
            } else {
                dfa_table[key.first][end_symbol] = i;
            }
            actions[i] = action;
            i++;
        }
    }

    stringstream def_dfa_table;
    def_dfa_table << endl;
    for (int i = 0; i < num_states; i++) {
        def_dfa_table << "    {";
        for (int j = 0; j < num_symbols; j++) {
            def_dfa_table << dfa_table[i][j] << ", ";
        }
        def_dfa_table << "}, " << endl;
    }

    stringstream def_actions_type;
    def_actions_type << endl;
    for (int i = 0; i < num_action_items;) {
        def_actions_type << "    ";
        for (int col = 0; col < 16 && i < num_action_items; col++, i++) {
            def_actions_type << (int) (actions[i].type) << ", ";
        }
        def_actions_type << endl;
    }

    stringstream def_actions_tgt;
    def_actions_tgt << endl;
    for (int i = 0; i < num_action_items;) {
        def_actions_tgt << "    ";
        for (int col = 0; col < 16 && i < num_action_items; col++, i++) {
            def_actions_tgt << actions[i].tgt << ", ";
        }
        def_actions_tgt << endl;
    }

    // reductions
    stringstream def_reductions;
    def_reductions << endl;
    for (int i = 0; i < grammar.prods.size(); i++) {
        const auto &prod = grammar.prods[i];
        def_reductions << "          "
                       << "case " << i << ": // ";
        def_reductions << symbolNames[prod.symbol] << " -> ";
        for (int r : prod.right) { def_reductions << symbolNames[r] << " "; }
        def_reductions << endl;
        if (i == 0) {
            def_reductions << "            "
                           << "// for example, $0.value  = $1.value + $2.value"
                           << endl;
        }
        def_reductions << "            "
                       << "nextNode.value = child_nodes[0].value; // DIY"
                       << endl;
        def_reductions << "            "
                       << "break;" << endl;
    }

    oss << fmt::format(codeSyntaxParserInCStyle, def_num_states.str(),
                       def_num_symbols.str(), def_num_action_items.str(),
                       def_num_prods.str(), def_prods.str(),
                       def_dfa_table.str(), def_actions_type.str(),
                       def_actions_tgt.str(), def_reductions.str());
}

// generate code of Syntax Parser (C++ format, standalone)
void genSyntaxParserInCppStyle(const Grammar &grammar,
                               map<int, string> symbolNames,
                               const ActionTable &actionTable, ostream &oss) {

    // prods
    stringstream defProds;
    defProds << endl;
    for (int i = 0; i < grammar.prods.size(); i++) {
        const auto &prod = grammar.prods[i];
        defProds << "    /* " << i << ": ";
        defProds << symbolNames[prod.symbol] << " -> ";
        for (int r : prod.right) { defProds << symbolNames[r] << " "; }
        defProds << "*/ {" << prod.symbol << ", {";
        for (int j = 0; j < prod.right.size(); j++) {
            defProds << prod.right[j]
                     << ((j + 1 < prod.right.size()) ? ", " : "");
        }
        defProds << "}}"
                 << ", " << endl;
    }

    // action table
    stringstream defActionTable;
    defActionTable << endl;
    const string typeName[] = {"ACTION", "REDUCE", "GOTO", "ACCEPT"};
    for (auto[key, action] : actionTable) {
        defActionTable << "    {{" << key.first << ", " << key.second << "}, {";
        defActionTable << typeName[action.type] << ", " << action.tgt << "}},"
                       << endl;
    }

    // reductions
    stringstream defReductions;
    defReductions << endl;
    for (int i = 0; i < grammar.prods.size(); i++) {
        const auto &prod = grammar.prods[i];
        defReductions << "          case " << i << ": // ";
        defReductions << symbolNames[prod.symbol] << " -> ";
        for (int r : prod.right) { defReductions << symbolNames[r] << " "; }
        defReductions << endl;
        if (i == 0) {
            defReductions << "            "
                          << "// for example, $0.value  = $1.value + $2.value"
                          << endl;
        }
        defReductions << "            "
                      << "nextNode = GrammarNode({ childNodes[0].value + "
                         "childNodes[1].value }); // DIY"
                      << endl;
        defReductions << "            "
                      << "break;" << endl;
    }

    oss << fmt::format(codeSyntaxParserInCppStyle, defProds.str(),
                       defActionTable.str(), defReductions.str());
}


} // namespace krill::codegen