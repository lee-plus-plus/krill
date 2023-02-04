#include "krill/syntax.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "krill/grammar.h"
#include "krill/lexical.h"
#include "krill/utils.h"
#include <cassert>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using krill::error::parse_error;
using krill::log::logger;
using namespace krill::type;
using namespace krill::grammar;
using namespace krill::utils;
using namespace std;

namespace krill::runtime {

// ------------------------------ AstPrinter ------------------------------

AstPrinter &AstPrinter::showMidNodes(bool flag) {
    showMidNodes_ = flag;
    return *this;
}

AstPrinter &AstPrinter::showLocation(bool flag) {
    showLocation_ = flag;
    return *this;
}

AstPrinter &AstPrinter::showColor(bool flag) {
    showColor_ = flag;
    return *this;
}

AstPrinter &AstPrinter::setWidth(int width) {
    assert(width >= 0);
    width_ = width;
    return *this;
}

// ----------------------------- BaseParser_core  -----------------------------

void BaseParser_core::clear_core() {
    inputs_     = queue<Token>{};
    states_     = stack<int>();
    symbols_    = stack<string>();
    row_curr    = 1;
    col_curr    = 1;
    offset_     = 0;
    isAccepted_ = false;
    states_.push(0);
}

void BaseParser_core::parse() {
    const auto &prods_   = grammar_.prods;
    const auto &symNames = grammar_.symbolNames;

    while (!isAccepted_ && offset_ < inputs_.size()) {
        Token input = inputs_.front();

        logger.debug("parser: states_: [{}]",
                     fmt::join(to_vector(states_), ","));
        logger.debug("parser: symbols_: [{}]",
                     fmt::join(to_vector(symbols_), ","));
        logger.debug("  parser: look: <token {}> ‘{}’", symNames.at(input.id),
                     unescape(input.lval));

        assert(states_.size() > 0);
        if (actionTable_.count({states_.top(), input.id}) == 0) {
            onError_impl();
            logger.debug("  parser: ERROR");
            throw parse_error(
                input.row_st, input.col_st,
                fmt::format("meet unexpected {}",
                            input.id == END_SYMBOL
                                ? "end of input"
                                : fmt::format(" <token {}> ‘{}’",
                                              symNames.at(input.id),
                                              unescape(input.lval))));
        }

        assert(actionTable_.count({states_.top(), input.id}) != 0);
        Action action = actionTable_.at({states_.top(), input.id});

        switch (action.type) {
        case Action::Type::kAction: {
            logger.debug("  parser: ACTION push [{}]", action.tgt);
            states_.push(action.tgt);
            symbols_.push(symNames.at(input.id));

            // virtual
            onAction_impl(input);
            // auto nextNode = make_shared<AstNode>(toNode(input));
            // onAction(nextNode.get());
            // nodes_.push(nextNode);

            inputs_.pop();
            break;
        }
        case Action::Type::kReduce: {
            const Prod &               r = prods_.at(action.tgt);
            deque<shared_ptr<AstNode>> child;

            auto poped_states = get_top(states_, r.right.size());
            for (int j = 0; (int) j < r.right.size(); j++) {
                states_.pop();
                symbols_.pop();
            }

            assert(actionTable_.count({states_.top(), r.symbol}) != 0);
            Action action2 = actionTable_.at({states_.top(), r.symbol});
            assert(action2.type == Action::Type::kGoto);
            states_.push(action2.tgt);
            symbols_.push(symNames.at(r.symbol));
            assert(0 <= action.tgt && action.tgt < prods_.size());

            logger.debug(
                "  parser: REDUCE r{} pop [{}] GOTO {}", action.tgt,
                fmt::join(poped_states, ","), action2.tgt);

            // virtual
            onReduce_impl(action.tgt);
            // for (int j = 0; (int) j < r.right.size(); j++) {
            //     child.push_front(nodes_.top());
            //     nodes_.pop();
            // }
            // auto nextNode =
            //     make_shared<AstNode>(toNode(child, r.symbol, action.tgt));
            // onReduce(nextNode.get());
            // nodes_.push(nextNode);
            break;
        }
        case Action::Type::kAccept: {
            logger.debug("  parser: ACCEPT");
            isAccepted_ = true;

            // virtual
            onAccept_impl();

            break;
        }
        default: {
            assert(false);
            break;
        }
        }
    }
}

BaseParser_core::BaseParser_core(const Grammar &    grammar,
                                 const ActionTable &actionTable)
    : grammar_(grammar), actionTable_(actionTable), inputs_(), states_({0}),
      symbols_(), row_curr(1), col_curr(1), offset_(0), isAccepted_(false) {}


} // namespace krill::runtime