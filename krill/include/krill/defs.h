#ifndef DEFS_H
#define DEFS_H

namespace krill {
    namespace lexical {
        class DFA;
        class NFA;
    }
    namespace grammar {
        class Grammar;
        class ActionTable;
    }
    namespace utils {}
    namespace runtime {}

    using DFA = lexical::DFA;
    using NFA = lexical::NFA;
    using Grammar = grammar::Grammar;
    using ActionTable = grammar::ActionTable;

} // namespace compiler
#endif