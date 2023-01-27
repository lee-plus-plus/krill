// mini calculator
#include <cassert>
#include <iostream>
#include <map>
#include <stack>
#include <string>
#include <vector>
using std::pair, std::vector, std::map, std::stack;
using namespace std;

struct GrammarNode {
    double value;
}; // DIY

GrammarNode syntaxParser(vector<int> tokens, vector<char> lexValues) {
    // declaration of struct
    struct Prod {
        int         symbol;
        vector<int> right;
    };
    enum TYPE { ACTION, REDUCE, GOTO, ACCEPT };
    struct Action {
        TYPE type;
        int  tgt;
    };
    typedef map<pair<int, int>, Action> ActionTable;

    // prods
    static const vector<Prod> prods = {
        /* FILL-0 */
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

    // Action Table
    static const ActionTable actionTable = {
        /* FILL-1 */
        {{0, 1}, {GOTO, 1}},     {{0, 2}, {GOTO, 2}},
        {{0, 3}, {ACTION, 3}},   {{0, 8}, {ACTION, 4}},
        {{0, 9}, {ACTION, 5}},   {{1, -1}, {ACCEPT, 0}},
        {{2, -1}, {REDUCE, 1}},  {{2, 4}, {REDUCE, 1}},
        {{2, 5}, {ACTION, 6}},   {{2, 6}, {ACTION, 7}},
        {{2, 7}, {ACTION, 8}},   {{2, 8}, {ACTION, 9}},
        {{3, 1}, {GOTO, 10}},    {{3, 2}, {GOTO, 2}},
        {{3, 3}, {ACTION, 3}},   {{3, 8}, {ACTION, 4}},
        {{3, 9}, {ACTION, 5}},   {{4, 2}, {GOTO, 11}},
        {{4, 3}, {ACTION, 3}},   {{4, 8}, {ACTION, 4}},
        {{4, 9}, {ACTION, 5}},   {{5, -1}, {REDUCE, 8}},
        {{5, 4}, {REDUCE, 8}},   {{5, 5}, {REDUCE, 8}},
        {{5, 6}, {REDUCE, 8}},   {{5, 7}, {REDUCE, 8}},
        {{5, 8}, {REDUCE, 8}},   {{6, 2}, {GOTO, 12}},
        {{6, 3}, {ACTION, 3}},   {{6, 8}, {ACTION, 4}},
        {{6, 9}, {ACTION, 5}},   {{7, 2}, {GOTO, 13}},
        {{7, 3}, {ACTION, 3}},   {{7, 8}, {ACTION, 4}},
        {{7, 9}, {ACTION, 5}},   {{8, 2}, {GOTO, 14}},
        {{8, 3}, {ACTION, 3}},   {{8, 8}, {ACTION, 4}},
        {{8, 9}, {ACTION, 5}},   {{9, 2}, {GOTO, 15}},
        {{9, 3}, {ACTION, 3}},   {{9, 8}, {ACTION, 4}},
        {{9, 9}, {ACTION, 5}},   {{10, 4}, {ACTION, 16}},
        {{11, -1}, {REDUCE, 7}}, {{11, 4}, {REDUCE, 7}},
        {{11, 5}, {REDUCE, 7}},  {{11, 6}, {REDUCE, 7}},
        {{11, 7}, {REDUCE, 7}},  {{11, 8}, {REDUCE, 7}},
        {{12, -1}, {REDUCE, 3}}, {{12, 4}, {REDUCE, 3}},
        {{12, 5}, {REDUCE, 3}},  {{12, 6}, {REDUCE, 3}},
        {{12, 7}, {REDUCE, 3}},  {{12, 8}, {REDUCE, 3}},
        {{13, -1}, {REDUCE, 4}}, {{13, 4}, {REDUCE, 4}},
        {{13, 5}, {REDUCE, 4}},  {{13, 6}, {REDUCE, 4}},
        {{13, 7}, {REDUCE, 4}},  {{13, 8}, {REDUCE, 4}},
        {{14, -1}, {REDUCE, 5}}, {{14, 4}, {REDUCE, 5}},
        {{14, 5}, {ACTION, 6}},  {{14, 6}, {ACTION, 7}},
        {{15, -1}, {REDUCE, 6}}, {{15, 4}, {REDUCE, 6}},
        {{15, 5}, {ACTION, 6}},  {{15, 6}, {ACTION, 7}},
        {{16, -1}, {REDUCE, 2}}, {{16, 4}, {REDUCE, 2}},
        {{16, 5}, {REDUCE, 2}},  {{16, 6}, {REDUCE, 2}},
        {{16, 7}, {REDUCE, 2}},  {{16, 8}, {REDUCE, 2}},
    };

    if (tokens[tokens.size() - 1] != -1) { tokens.push_back(-1); }

    stack<int>         states;
    stack<GrammarNode> stateNodes; // DIY
    states.push(0);

    for (int i = 0, accpeted = false; !accpeted;) {
        // look tokens[i], state => next_state, action
        assert(actionTable.count({states.top(), tokens[i]}) != 0);

        Action action = actionTable.at({states.top(), tokens[i]});
        switch (action.type) {
            case ACTION: {
                states.push(action.tgt);
                stateNodes.push(GrammarNode({(double)(lexValues[i] - '0')})); // DIY
                i++;
                break;
            }
            case REDUCE: {
                Prod                r = prods[action.tgt];
                vector<GrammarNode> childNodes;
                for (int j = 0; j < (int) r.right.size(); j++) {
                    states.pop();
                    childNodes.insert(childNodes.begin(), stateNodes.top());
                    stateNodes.pop();
                }

                assert(actionTable.count({states.top(), r.symbol}) != 0);
                Action action2 = actionTable.at({states.top(), r.symbol});
                assert(action2.type == GOTO);
                states.push(action2.tgt);

                GrammarNode nextNode;
                switch (action.tgt) {
                    /* FILL-2 */
                    case 0: // Q -> P
                        // for example, $0.value  = $1.value + $2.value
                        nextNode = GrammarNode(
                            {childNodes[0].value}); // DIY
                        break;
                    case 1: // P -> T
                        nextNode = GrammarNode(
                            {childNodes[0].value}); // DIY
                        break;
                    case 2: // T -> ( P )
                        nextNode = GrammarNode(
                            {childNodes[1].value}); // DIY
                        break;
                    case 3: // T -> T * T
                        nextNode = GrammarNode(
                            {childNodes[0].value * childNodes[2].value}); // DIY
                        break;
                    case 4: // T -> T / T
                        nextNode = GrammarNode(
                            {childNodes[0].value / childNodes[2].value}); // DIY
                        break;
                    case 5: // P -> T + T
                        nextNode = GrammarNode(
                            {childNodes[0].value + childNodes[2].value}); // DIY
                        break;
                    case 6: // P -> T - T
                        nextNode = GrammarNode(
                            {childNodes[0].value - childNodes[2].value}); // DIY
                        break;
                    case 7: // T -> - T
                        nextNode = GrammarNode(
                            {-childNodes[1].value}); // DIY
                        break;
                    case 8: // T -> d
                        nextNode = GrammarNode(
                            {childNodes[0].value}); // DIY
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
                accpeted = true;
                break;
            }
            default: {
                assert(false);
                break;
            }
        }
    }
    assert(stateNodes.size() == 1);
    return stateNodes.top(); // DIY
}

pair<vector<int>, vector<char>> lexicalParser(string src) {
    // {{0, "Q"}, {1, "P"}, {2, "T"}, {3, "("},
    //  {4, ")"}, {5, "*"}, {6, "/"}, {7, "+"},
    //  {8, "-"}, {9, "d"}}
    vector<int>  tokens;
    vector<char> lexValues;

    for (char c : src) {
        int token;
        if (c == '(') {
            token = 3;
        } else if (c == ')') {
            token = 4;
        } else if (c == '*') {
            token = 5;
        } else if (c == '/') {
            token = 6;
        } else if (c == '+') {
            token = 7;
        } else if (c == '-') {
            token = 8;
        } else if (c >= '0' && c <= '9') {
            token = 9;
        }
        tokens.push_back(token);
        lexValues.push_back(c);
    }
    return make_pair(tokens, lexValues);
}


int main() {
    string src;
    while (true) {
        cout << ">> ";
        getline(cin, src);
        auto[tokens, lexValues] = lexicalParser(src);
        auto node               = syntaxParser(tokens, lexValues);
        cout << node.value << endl;
    }

    return 0;
}
