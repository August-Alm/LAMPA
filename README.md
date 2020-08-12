# LAMPA

A small library to parse lambda calculus, both into a more
canonical abstract syntax tree presentation and into the de
Bruijn-coded abstract syntax trees, reading lambda "source
code" from a filepointer. Source syntax is as follows:

* Variables:    Strings of < 16 chars. Legit chars are either
              alpha-numeric or an underline.
* Lambdas:      "\x.term" where "x" is a variable and "term"
              is another lambda-expression.
* Applications: "(fun arg)" where "fun" and "arg" are terms.
              The parentheses are mandatory.
 
Additionally, we extend the lambda-calculus with a define
macro, with syntax

* Define:       "@ name = term".
 
Here `name` is a variable (as above) and `term` is a closed
term. All top-level declarations should be of this form. The
string `name` may be used as an alias for `term` in any sub-
sequent declaration.

The library also includes functions to translate between the
two abstract syntax tree-encodings, for pretty-printing and
etc; essentially everything needed for an interpreter or REPL
except an evaluation/normalization algorithm. The functions
are written focusing on efficiency but include a modicum of
error-handling and are memory-safe under most (all?) usage.
