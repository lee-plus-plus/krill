RegEx : Parallel
Parallel : Parallel '|' Seq
Parallel : Seq
Seq : Seq Item
Seq : Item
Item : Closure
Item : Atom
Closure : Atom '+'
Closure : Atom '*'
Closure : Atom '?'
 Atom : '(' Parallel ')'
 Atom : Char
 Atom : Range
 Range : '[' RangeSeq ']'
 Range : '[' '^' RangeSeq ']'
 RangeSeq : RangeSeq RangeItem
 RangeSeq : RangeItem
 RangeItem : Char '-' Char
 RangeItem : Char
 Atom : '.'