/**
 *          ╔═════════════════════════╗
 *          ║ PARSING LAMBDA CALCULUS ║
 *          ╚═════════════════════════╝
 *
 * \author  August-Alm@github.com
 *
 * \notes   A small library to parse lambda calculus, both into a more
 *          canonical abstract syntax tree presentation and into the de
 *          Bruijn-coded abstract syntax trees, reading lambda "source
 *          code" from a filepointer. Source syntax is as follows:
 *
 *          Variables:    Strings of < 16 chars. Legit chars are either
 *                        alpha-numeric or '_'.
 *          Lambdas:      "\x.term" where "x" is a variable and "term"
 *                        is another lambda-expression.
 *          Applications: "(fun arg)" where "fun" and "arg" are terms.
 *                        The parentheses are mandatory.
 *
 *          Additionally, we extend the lambda-calculus with a define
 *          macro, with syntax
 *
 *          Define:       "@ name = term".
 *
 *          Here `name` is a variable (as above) and `term` is a closed
 *          term. All top-level declarations should be of this form. The
 *          string `name` may be used as an alias for `term` in any sub-
 *          sequent declaration.
 *
 *          The library also includes functions to translate between the
 *          two abstract syntax tree-encodings, for pretty-printing and
 *          etc; essentially everything needed for an interpreter or REPL
 *          except an evaluation/normalization algorithm. The functions
 *          are written focusing on efficiency but include a modicum of
 *          error-handling and are memory-safe under most (all?) usage. 
 */
 
/* ***** ***** */

#ifndef LAMBDA_PARSER_H
#define LAMBDA_PARSER_H

/* ***** ***** */


/**********************************************************************/
/*          CANONICAL AST TYPE                                        */
/**********************************************************************/

/**
 * \brief   The obvious encoding of lambda calculus: terms are either
 *          variables (symbols), lambdas (abstractions) `\variable.term`
 *          or applications `(term term)`. 
 */
struct terms1; 

/**
 * \brief   Frees all nodes (and all heap data at them) of the AST.
 */
void decref_terms1(struct terms1 *t0);

void incref_terms1(struct terms1 *t0);

/**
 * \brief   Pretty-prints the AST, returning it to lambda source code.
 */
void fprintf_terms1(FILE *out, struct terms1 *t0);

/*********************************************************************/
/*          DE BRUIJN AST TYPE                                       */
/*********************************************************************/

/** 
 * \brief   Variables are only natural numbers, lambdas (abstractions)
 *          are just `\term` and applications are `(term term)`.
 */
struct terms2;

/**
 * \brief   Frees all nodes (and all heap data at them) of the AST.
 */
void decref_terms2(struct terms2 *t0);

void inccref_terms2(struct terms2 *t0);

/**
 * \brief   Pretty-prints the AST (in textual de Bruijn form).
 */
void fprintf_terms2(FILE *out, struct terms2 *t0);



/*********************************************************************/
/*          NAMES                                                    */
/*********************************************************************/

/**
 * \brief   Stacks backed by dynamical arrays of char-pointers. Used to
 *          store lambda-bindings of variable names.
 */
struct names;

/**
 * \brief   Allocates a `names` array that can store `cap` variables
 *          without having to resize.
 */
struct names *alloc_names(size_t cap);

/**
 * \brief   Frees `xs` and all variables stored in it.
 */
void free_names(struct names *xs);

/**
 * \brief   Gets the de Bruijn index of the variable `x` with respect
 *          to the context `xs`: this is just the depth of `x` in the
 *          stack `xs` -- if `x` is unbound we return `-1`.
 */
int get_dbidx(char *x, struct names *xs);


/**********************************************************************/
/*          TRANSLATING TO/FROM DE BRUIJN ENCODING                    */
/**********************************************************************/

/**
 * \brief   Typically called with an empty stack `xs` of names, in case
 *          it stores all the lambda-bound variables of the term in `xs`
 *          after it returns. Frees neither argument.
 */
struct terms2 *lam2db(struct terms1 *t, struct names *xs);

/**
 * \brief   Convenience wrapper of `lam2db`, allocating an empty stack
 *          of bound variable names which it frees before returning.
 */
struct terms2 *lam2db_nonames(struct terms1 *t);

/**
 * \brief   Replaces de Bruijn indexes by named variables, using the
 *          given stack of names as a dictionary, and translates all
 *          lambdas and applications accordingly. Note that it reverses
 *          the stack of names.
 */
struct terms1 *db2lam(struct terms2 *t, struct names *xs);


/*********************************************************************/
/*          PARSING LAMBDA-TERMS                                     */
/*********************************************************************/

/**
 * \brief   Parses a closed term from input. Returns `NULL` in case of
 *          failure.
 */
struct terms1 *parse_terms1(FILE *inp);

/**
 * \brief   Like `parse_terms1` but de Bruijn-encodes the term while it
 *          is parsing. More efficient than first using `parse_terms1`
 *          and then `lam2db`.
 */
struct terms2 *parse_terms2(FILE *inp, struct names *xs);

/**
 * \brief   Convenience wrapper. Note that in order to, e.g., pretty
 *          print results we need to keep the `names` around (in order
 *          to be able to translate back from the de Bruijn encoding)
 *          which this function doesn't.
 */
struct terms2 *parse_terms2_nonames(FILE *inp);


/*********************************************************************/
/*          PARSING DECLARATIVE LAMBDA-TERMS                         */
/*********************************************************************/

/**
 * \brief   Struct for storing bindings between variable names and
 *          canonically encoded lambda-terms.
 */
struct contexts1;
struct contexts1 *alloc_contexts1(size_t cap);
void free_contexts1(struct contexts1 *ctx);

struct contexts2;
struct contexts2 *alloc_contexts2(size_t cap);
void free_contexts2(struct contexts2 *ctx);


/**
 * \brief   Parses one declared closed term from input. Returns `NULL`
 *          in case of failure.
 */
struct terms1 *parse_declterms1(FILE *inp, struct contexts1 *ctx);

struct terms2 *parse_declterms2(FILE *inp, struct names *xs
                                         , struct contexts2 *ctx);

/* ***** ***** */

#endif // LAMBDA_PARSER_H
