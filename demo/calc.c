#include <stdio.h>
#include <assert.h>

int /* DIY */ lexicalAnalyze(const int num_tokens, int *tokens, int *values) {
  typedef struct { int symbol; int len_right; int *right; } Production;
  enum TYPE {ACTION=0, REDUCE=1, GOTO=2, ACCEPT=3};
  struct GrammarNode { int value; }; // DIY

  #define num_states 17
  #define num_symbols 11
  #define num_action_items 76

  Production prods[9] = {
    /* 0: Q -> P */ {0, 1, (int [1]) {1}}, 
    /* 1: P -> T */ {1, 1, (int [1]) {2}}, 
    /* 2: T -> ( P ) */ {2, 3, (int [3]) {3, 1, 4}}, 
    /* 3: T -> T * T */ {2, 3, (int [3]) {2, 5, 2}}, 
    /* 4: T -> T / T */ {2, 3, (int [3]) {2, 6, 2}}, 
    /* 5: P -> T + T */ {1, 3, (int [3]) {2, 7, 2}}, 
    /* 6: P -> T - T */ {1, 3, (int [3]) {2, 8, 2}}, 
    /* 7: T -> - T */ {2, 2, (int [2]) {8, 2}}, 
    /* 8: T -> d */ {2, 1, (int [1]) {9}}, 
  };
  int dfa_table[num_states][num_symbols] = {
    {-1, 0, 1, 2, -1, -1, -1, -1, 3, 4, -1, }, 
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 5, }, 
    {-1, -1, -1, -1, 7, 8, 9, 10, 11, -1, 6, }, 
    {-1, 12, 13, 14, -1, -1, -1, -1, 15, 16, -1, }, 
    {-1, -1, 17, 18, -1, -1, -1, -1, 19, 20, -1, }, 
    {-1, -1, -1, -1, 22, 23, 24, 25, 26, -1, 21, }, 
    {-1, -1, 27, 28, -1, -1, -1, -1, 29, 30, -1, }, 
    {-1, -1, 31, 32, -1, -1, -1, -1, 33, 34, -1, }, 
    {-1, -1, 35, 36, -1, -1, -1, -1, 37, 38, -1, }, 
    {-1, -1, 39, 40, -1, -1, -1, -1, 41, 42, -1, }, 
    {-1, -1, -1, -1, 43, -1, -1, -1, -1, -1, -1, }, 
    {-1, -1, -1, -1, 45, 46, 47, 48, 49, -1, 44, }, 
    {-1, -1, -1, -1, 51, 52, 53, 54, 55, -1, 50, }, 
    {-1, -1, -1, -1, 57, 58, 59, 60, 61, -1, 56, }, 
    {-1, -1, -1, -1, 63, 64, 65, -1, -1, -1, 62, }, 
    {-1, -1, -1, -1, 67, 68, 69, -1, -1, -1, 66, }, 
    {-1, -1, -1, -1, 71, 72, 73, 74, 75, -1, 70, }, 
  };
  enum TYPE actions_type[num_action_items] = {
    2, 2, 0, 0, 0, 3, 1, 1, 0, 0, 0, 0, 2, 2, 0, 0, 
    0, 2, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 0, 0, 0, 2, 
    0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 1, 1, 1, 1, 
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
    0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 
  };
  int actions_tgt[num_action_items] = {
    1, 2, 3, 4, 5, 0, 1, 1, 6, 7, 8, 9, 10, 2, 3, 4, 
    5, 11, 3, 4, 5, 8, 8, 8, 8, 8, 8, 12, 3, 4, 5, 13, 
    3, 4, 5, 14, 3, 4, 5, 15, 3, 4, 5, 16, 7, 7, 7, 7, 
    7, 7, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 
    6, 7, 6, 6, 6, 7, 2, 2, 2, 2, 2, 2, 
  };
  assert(tokens[num_tokens - 1] == -1);

  int states[num_tokens];
  int len_states = 0;
  struct GrammarNode state_nodes[num_tokens]; // DIY
  int len_state_nodes = 0;
  states[len_states++] = 0;

  for (int i = 0, flag = 1; flag; ) {
    int now = (tokens[i] != -1) ? tokens[i] : num_symbols - 1;

    // look tokens[i], state => next_state, action
    assert(dfa_table[states[len_states - 1]][now] != -1);
    enum TYPE action_type = actions_type[ dfa_table[states[len_states - 1]][now] ];
    int action_tgt = actions_tgt[ dfa_table[states[len_states - 1]][now] ];
    switch (action_type) {
    case ACTION: {
      states[len_states++] = action_tgt;
      state_nodes[len_state_nodes++].value = values[i]; // DIY
      i++;
      break;
    }
    case REDUCE: {
      Production r = prods[action_tgt];
      struct GrammarNode child_nodes[r.len_right];
      for (int j = 0; j < (int) r.len_right; j++) {
        len_states--;
        child_nodes[r.len_right - j - 1] = state_nodes[--len_state_nodes];
      }

      assert(dfa_table[states[len_states - 1]][r.symbol] != -1);
      enum TYPE action2_type = actions_type[ dfa_table[states[len_states - 1]][r.symbol] ];
      int action2_tgt = actions_tgt[ dfa_table[states[len_states - 1]][r.symbol] ];
      assert(action2_type == GOTO);
      states[len_states++] = action2_tgt;
        struct GrammarNode nextNode;
        switch (action_tgt) {
          case 0: // Q -> P 
            nextNode.value = child_nodes[0].value; // DIY
            break;
          case 1: // P -> T 
            nextNode.value = child_nodes[0].value; // DIY
            break;
          case 2: // T -> ( P ) 
            nextNode.value = child_nodes[1].value; // DIY
            break;
          case 3: // T -> T * T 
            nextNode.value = child_nodes[0].value * child_nodes[2].value; // DIY
            break;
          case 4: // T -> T / T 
            nextNode.value = child_nodes[0].value / child_nodes[2].value; // DIY
            break;
          case 5: // P -> T + T 
            nextNode.value = child_nodes[0].value + child_nodes[2].value; // DIY
            break;
          case 6: // P -> T - T 
            nextNode.value = child_nodes[0].value - child_nodes[2].value; // DIY
            break;
          case 7: // T -> - T 
            nextNode.value = -child_nodes[1].value; // DIY
            break;
          case 8: // T -> d 
            nextNode.value = child_nodes[0].value; // DIY
            break;
          default: {
            assert(0);
            break;
          }
        }
        state_nodes[len_state_nodes++] = nextNode;
        break;
      }
      case ACCEPT: {
        flag = 0;
        break;
      }
      default: {
        assert(0);
        break;
      }
    }       
  }
  assert(len_state_nodes == 1);
  return state_nodes[0].value; // DIY
}

int main() {
  #define num_tokens 9
  int tokens[num_tokens] = { 3, 9, 7, 9, 4, 5, 8, 9, -1 };
  int values[num_tokens] = { 0, 1, 0, 3, 0, 0, 0, 5,  0 };
  int result = lexicalAnalyze(num_tokens, tokens, values);
  printf("%d\n", result);
}