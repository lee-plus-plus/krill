// build from krill's lexical analyzer
#include <iostream>
#include <vector>
#include <map>
#include <stack>
#include <cassert>
using std::pair, std::vector, std::map, std::stack;
using std::cout, std::endl;

int lexicalAnalyze(vector<int> src, vector<int> values) {
  struct Production { int symbol; vector<int> right; };
  enum TYPE {ACTION, REDUCE, GOTO, ACCEPT};
  struct Action { TYPE type; int tgt; };
  typedef map<pair<int, int>, Action> ActionTable;
  struct GrammarNode { int value; }; // DIY

  vector<Production> prods = {
    /* 0: Q -> P */ {0, {1}}, 
    /* 1: P -> T */ {1, {2}}, 
    /* 2: T -> ( P ) */ {2, {3, 1, 4}}, 
    /* 3: T -> T * T */ {2, {2, 5, 2}}, 
    /* 4: T -> T / T */ {2, {2, 6, 2}}, 
    /* 5: P -> T + T */ {1, {2, 7, 2}}, 
    /* 6: P -> T - T */ {1, {2, 8, 2}}, 
    /* 7: T -> - T */ {2, {8, 2}}, 
    /* 8: T -> d */ {2, {9}}, 
  };
  ActionTable analyzeTable = {
    {{0, 1}, {GOTO, 1}},
    {{0, 2}, {GOTO, 2}},
    {{0, 3}, {ACTION, 3}},
    {{0, 8}, {ACTION, 4}},
    {{0, 9}, {ACTION, 5}},
    {{1, -1}, {ACCEPT, 0}},
    {{2, -1}, {REDUCE, 1}},
    {{2, 4}, {REDUCE, 1}},
    {{2, 5}, {ACTION, 6}},
    {{2, 6}, {ACTION, 7}},
    {{2, 7}, {ACTION, 8}},
    {{2, 8}, {ACTION, 9}},
    {{3, 1}, {GOTO, 10}},
    {{3, 2}, {GOTO, 2}},
    {{3, 3}, {ACTION, 3}},
    {{3, 8}, {ACTION, 4}},
    {{3, 9}, {ACTION, 5}},
    {{4, 2}, {GOTO, 11}},
    {{4, 3}, {ACTION, 3}},
    {{4, 8}, {ACTION, 4}},
    {{4, 9}, {ACTION, 5}},
    {{5, -1}, {REDUCE, 8}},
    {{5, 4}, {REDUCE, 8}},
    {{5, 5}, {REDUCE, 8}},
    {{5, 6}, {REDUCE, 8}},
    {{5, 7}, {REDUCE, 8}},
    {{5, 8}, {REDUCE, 8}},
    {{6, 2}, {GOTO, 12}},
    {{6, 3}, {ACTION, 3}},
    {{6, 8}, {ACTION, 4}},
    {{6, 9}, {ACTION, 5}},
    {{7, 2}, {GOTO, 13}},
    {{7, 3}, {ACTION, 3}},
    {{7, 8}, {ACTION, 4}},
    {{7, 9}, {ACTION, 5}},
    {{8, 2}, {GOTO, 14}},
    {{8, 3}, {ACTION, 3}},
    {{8, 8}, {ACTION, 4}},
    {{8, 9}, {ACTION, 5}},
    {{9, 2}, {GOTO, 15}},
    {{9, 3}, {ACTION, 3}},
    {{9, 8}, {ACTION, 4}},
    {{9, 9}, {ACTION, 5}},
    {{10, 4}, {ACTION, 16}},
    {{11, -1}, {REDUCE, 7}},
    {{11, 4}, {REDUCE, 7}},
    {{11, 5}, {REDUCE, 7}},
    {{11, 6}, {REDUCE, 7}},
    {{11, 7}, {REDUCE, 7}},
    {{11, 8}, {REDUCE, 7}},
    {{12, -1}, {REDUCE, 3}},
    {{12, 4}, {REDUCE, 3}},
    {{12, 5}, {REDUCE, 3}},
    {{12, 6}, {REDUCE, 3}},
    {{12, 7}, {REDUCE, 3}},
    {{12, 8}, {REDUCE, 3}},
    {{13, -1}, {REDUCE, 4}},
    {{13, 4}, {REDUCE, 4}},
    {{13, 5}, {REDUCE, 4}},
    {{13, 6}, {REDUCE, 4}},
    {{13, 7}, {REDUCE, 4}},
    {{13, 8}, {REDUCE, 4}},
    {{14, -1}, {REDUCE, 5}},
    {{14, 4}, {REDUCE, 5}},
    {{14, 5}, {ACTION, 6}},
    {{14, 6}, {ACTION, 7}},
    {{15, -1}, {REDUCE, 6}},
    {{15, 4}, {REDUCE, 6}},
    {{15, 5}, {ACTION, 6}},
    {{15, 6}, {ACTION, 7}},
    {{16, -1}, {REDUCE, 2}},
    {{16, 4}, {REDUCE, 2}},
    {{16, 5}, {REDUCE, 2}},
    {{16, 6}, {REDUCE, 2}},
    {{16, 7}, {REDUCE, 2}},
    {{16, 8}, {REDUCE, 2}},
  };

  if (src[src.size() - 1] != -1) {
    src.push_back(-1);
  }

  stack<int> states;
  stack<GrammarNode> stateNodes; // DIY
  states.push(0);

  // stack of symbols (unnecessary)
  for (int i = 0, flag = true; flag; ) {
    // look src[i], state => next_state, action
    assert(analyzeTable.count({states.top(), src[i]}) != 0);

    Action action = analyzeTable[{states.top(), src[i]}];
    switch (action.type) {
      case ACTION: {
        states.push(action.tgt);
        stateNodes.push(GrammarNode({ values[i] })); // DIY
        i++;
        break;
      }
      case REDUCE: {
        Production r = prods[action.tgt];
        vector<GrammarNode> childNodes;
        for (int j = 0; j < (int) r.right.size(); j++) {
          states.pop();
          childNodes.insert(childNodes.begin(), stateNodes.top());
          stateNodes.pop();
        }

        assert(analyzeTable.count({states.top(), r.symbol}) != 0);
        Action action2 = analyzeTable[{states.top(), r.symbol}];
        assert(action2.type == GOTO);
        states.push(action2.tgt);

        GrammarNode nextNode;
        switch (action.tgt) {
          case 0: // Q -> P 
            nextNode = GrammarNode({ childNodes[0].value }); // DIY
            break;
          case 1: // P -> T 
            nextNode = GrammarNode({ childNodes[0].value }); // DIY
            break;
          case 2: // T -> ( P ) 
            nextNode = GrammarNode({ childNodes[1].value }); // DIY
            break;
          case 3: // T -> T * T 
            nextNode = GrammarNode({ childNodes[0].value * childNodes[2].value }); // DIY
            break;
          case 4: // T -> T / T 
            nextNode = GrammarNode({ childNodes[0].value / childNodes[2].value }); // DIY
            break;
          case 5: // P -> T + T 
            nextNode = GrammarNode({ childNodes[0].value + childNodes[2].value }); // DIY
            break;
          case 6: // P -> T - T 
            nextNode = GrammarNode({ childNodes[0].value - childNodes[2].value }); // DIY
            break;
          case 7: // T -> - T 
            nextNode = GrammarNode({ -childNodes[1].value }); // DIY
            break;
          case 8: // T -> d 
            nextNode = GrammarNode({ childNodes[0].value }); // DIY
            break;
          default: {
            assert(false);
            break;
          }
        }
        stateNodes.push(nextNode);
        break;
      }
      case ACCEPT: {
        flag = false;
        break;
      }
      default: {
        assert(false);
        break;
      }
    }       
  }
  assert(stateNodes.size() == 1);
  return stateNodes.top().value; // DIY
}

int main() {
  // ( 1 + 3 ) * - 5
  vector<int> states = { 3, 9, 7, 9, 4, 5, 8, 9 };
  vector<int> values = { 0, 1, 0, 3, 0, 0, 0, 5 };
  int result = lexicalAnalyze(states, values);
  cout << result << endl;
  return 0;
}
