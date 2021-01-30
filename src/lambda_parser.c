/*
    ╔═════════════════════════╗
    ║ PARSING LAMBDA CALCULUS ║
    ╚═════════════════════════╝

*/

/* ***** ***** */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "lambda_parser.h"

/* ***** ***** */

#define MALCHECK(s) if(!s) {                               \
    fprintf(stderr, "Malloc failed at line %d in `%s`.\n"  \
                  , __LINE__, __FUNCTION__);               \
    return NULL;                                           \
}                                                          \

/* ***** ***** */

//  The obvious AST encoding.

struct terms1 {
    unsigned int refcnt;
    enum {VAR1, LAM1, APP1} tag;
    union {
        char *var;
        struct lams1 {char *var; struct terms1 *bod;} *lam;
        struct apps1 {struct terms1 *fun; struct terms1 *arg;} *app;
    };
};

void decref_terms1(struct terms1 *t0)
{
    if (!t0) {return;}
    if (t0->refcnt <= 1) {
        switch (t0->tag) {
        case VAR1:
            free(t0->var);
            free(t0);
            break;
        case LAM1:
            free(t0->lam->var);
            decref_terms1(t0->lam->bod);
            free(t0->lam);
            free(t0);
            break;
        case APP1:
            decref_terms1(t0->app->fun);
            decref_terms1(t0->app->arg);
            free(t0->app);
            free(t0);
            break;
        }
    } else {
        t0->refcnt--;
    }
}

void incref_terms1(struct terms1 *t0)
{
    t0->refcnt++;
}

void fprintf_terms1(FILE *out, struct terms1 *t0)
{
    if (!t0) {
        fprintf(out, "`NULL`-term.");
    }
    switch (t0->tag) {
    case VAR1:
        fprintf(out, "%s", t0->var);
        break;
    case LAM1:
        fprintf(out, "\\%s.", t0->lam->var);
        fprintf_terms1(out, t0->lam->bod);
        break;
    case APP1:
        fprintf(out, "(");
        fprintf_terms1(out, t0->app->fun);
        fprintf(out, " ");
        fprintf_terms1(out, t0->app->arg);
        fprintf(out, ")");
        break;
    }
}


/* ***** ***** */

// de Bruijn AST type.

struct terms2 {
    unsigned int refcnt;
    enum {VAR2, LAM2, APP2} tag;
    union {
        unsigned int idx;
        struct terms2 *lam;
        struct apps2 {struct terms2 *fun; struct terms2 *arg;} *app;
    };
};

void decref_terms2(struct terms2 *t0)
{
    if (!t0) {return;}
    if (t0->refcnt <= 1) {
        switch (t0->tag) {
        case VAR2:
            free(t0);
            break;
        case LAM2:
            decref_terms2(t0->lam);
            free(t0);
            break;
        case APP2:
            decref_terms2(t0->app->fun);
            decref_terms2(t0->app->arg);
            free(t0->app);
            free(t0);
            break;
        }
    } else {
        t0->refcnt--;
    }
}

void incref_terms2(struct terms2 *t0)
{
    t0->refcnt++;
}

void fprintf_terms2(FILE *out, struct terms2 *t0)
{
    if (!t0) {
        fprintf(out, "`NULL`-term.");
    }
    switch (t0->tag) {
    case VAR2:
        fprintf(out, "%u", t0->idx);
        break;
    case LAM2:
        fprintf(out, "\\");
        fprintf_terms2(out, t0->lam);
        break;
    case APP2:
        fprintf(out, "(");
        fprintf_terms2(out, t0->app->fun);
        fprintf(out, " ");
        fprintf_terms2(out, t0->app->arg);
        fprintf(out, ")");
        break;
    }
}
/* ***** ***** */

//  Names (arrays of bound variables).

struct names {
    size_t cap; // Number of allocated names.
    size_t num; // Number of initialized names.
    char **els;
};
    
struct names *alloc_names(size_t cap)
{
    struct names *xs = malloc(sizeof(struct names));
    MALCHECK(xs);
    xs->cap = cap;
    xs->num = 0;
    char **tmp = malloc(sizeof(char*) * cap);
    MALCHECK(tmp);
    xs->els = tmp;
    return xs;
}

void free_names(struct names *xs)
{
    for (int i = 0; i < xs->num; i++){free(xs->els[i]);}
    free(xs->els); free(xs);
}

char *pop_names(struct names *xs)
{
    if (xs->num == 0) {return NULL;}
    char *res = xs->els[(xs->num) - 1];
    xs->num--;
    return res;
}

void push_names(struct names *xs, char *x)
{
    if (xs->num < xs->cap) {
       xs->els[xs->num] = x;
       xs->num++;
    } else {
        void *tmp = realloc(xs, (sizeof(xs) * 3)/2 + 8);
        if (!tmp) {
            free_names(tmp);
            fprintf(stderr, "Failed realloc at line %d in `%s`.\n"
                          , __LINE__, __FUNCTION__);
            return;
        }
        xs = tmp;
        xs->cap = ((xs->cap) * 3)/2 + 8;
        xs->els[xs->num] = x;
        xs->num++;
    }
}

int get_dbidx(char *x, struct names *xs)
{
    int num = xs->num;
    if (num == 0) {
        fprintf(stderr, "Unbound name %s.\n", x);
    }
    for (int i = num - 1; i >= 0; i--) {
        if (!strcmp(x, xs->els[i])) {
            return num -1 - i;
        }
    }
    fprintf(stderr, "Undbound name %s.\n", x);
    return -1;
}

/* ***** ***** */

//  Translating to/from de Bruijn encoding.

struct terms2 *lam2db(struct terms1 *t, struct names *xs)
{
    if (!t) {return NULL;}
    if (t->tag == VAR1) {
        int idx = get_dbidx(t->var, xs);
        if (idx == -1) {
            fprintf(stderr, "Unbound name %s.\n", t->var);
        }
        struct terms2 *var2 = malloc(sizeof(struct terms2));
        MALCHECK(var2);
        *var2 = (struct terms2) {.refcnt = 1, .tag = VAR2, .idx = idx};
        return var2;
    }
    if (t->tag == LAM1) {
        char *x = t->lam->var;
        push_names(xs, strdup(x));
        struct terms1 *bod = t->lam->bod;
        struct terms2 *bod2 = lam2db(bod, xs);
        struct terms2 *lam2 = malloc(sizeof(struct terms2));
        MALCHECK(lam2);
        *lam2 = (struct terms2) {.refcnt = 1, .tag = LAM2, .lam = bod2};
        return lam2;
    }
    // Tag `APP1`.
    struct terms1 *fun = t->app->fun;
    struct terms1 *arg = t->app->arg;
    struct terms2 *fun2 = lam2db(fun, xs);
    struct terms2 *arg2 = lam2db(arg, xs);
    struct terms2 *app2 = malloc(sizeof(struct terms2));
    MALCHECK(app2);
    struct apps2 *app2_app = malloc(sizeof(struct apps2));
    MALCHECK(app2_app);
    app2_app->fun = fun2;
    app2_app->arg = arg2;
    app2->refcnt = 1;
    app2->tag = APP2;
    app2->app = app2_app;
    return app2;
}

struct terms2 *lam2db_nonames(struct terms1 *t)
{
    struct names *xs = alloc_names(16);
    struct terms2 *result = lam2db(t, xs);
    free_names(xs);
    return result;
}

/* ***** ***** */

struct terms1 *db2lam_aux(struct terms2 *t, struct names *xs
                                          , struct names *tmp)
{
    if (!t) {return NULL;}

    if (t->tag == VAR2) {
        int i = tmp->num - 1 - t->idx;
        if (i >= 0) {
            struct terms1 *variable = malloc(sizeof(struct terms1));
            MALCHECK(variable);
            variable->refcnt = 1;
            variable->tag = VAR1;
            variable->var = strdup(tmp->els[i]);
            return variable;
        } else {
            fprintf(stderr, "The de Bruijn index %u was an unbound"
                            "variable. Malformed term.\n", t->idx);
            return NULL;
        }
    } else if (t->tag == LAM2) {
        char *x = pop_names(xs);
        MALCHECK(x)
        push_names(tmp, strdup(x));
        struct terms2 *t0 = t->lam;
        struct terms1 *body = db2lam_aux(t0, xs, tmp);
        MALCHECK(body)
        struct terms1 *lambda = malloc(sizeof(struct terms1));
        MALCHECK(lambda);
        struct lams1 *lambda_lam = malloc(sizeof(struct lams1));
        MALCHECK(lambda_lam);
        lambda_lam->var = x;
        lambda_lam->bod = body;
        lambda->refcnt = 1;
        lambda->tag = LAM1;
        lambda->lam = lambda_lam;
        return lambda;
    }
    struct terms2 *e1 = t->app->fun;
    struct terms2 *e2 = t->app->arg;
    struct terms1 *t1 = db2lam_aux(e1, xs, tmp);
    if (!t1) {return NULL;}
    struct terms1 *t2 = db2lam_aux(e2, xs, tmp);
    if (!t2) {return NULL;}
    struct terms1 *application = malloc(sizeof(struct terms1));
    MALCHECK(application);
    struct apps1 *application_app = malloc(sizeof(struct apps1));
    MALCHECK(application_app);
    application_app->fun = t1;
    application_app->arg = t2;
    application->refcnt = 1;
    application->tag = APP1;
    application->app = application_app;
    return application;
}

struct terms1 *db2lam(struct terms2 *t, struct names *xs)
{
    for (int lo = 0, hi = xs->num - 1; lo < hi; lo++, hi--) {
        char *s = xs->els[lo];
        xs->els[lo] = xs->els[hi];
        xs->els[hi] = s;
    }
    struct names *tmp = alloc_names(16);
    struct terms1 *res = db2lam_aux(t, xs, tmp);
    free_names(tmp);
    return res;
} 


/* ***** ***** */

//  Parsing utility functions.

//  Returns `0` if it the current char of `inp` is not `chr`, `1` if it is.
int parse_char(FILE *inp, char chr)
{
    int c = fgetc(inp);
    if (c == EOF) {
        fprintf(stderr, "Unexpected EOF during `parse_char`.\n");
        return 0;
    } else if (c != chr) {
        ungetc(c, inp);
        fprintf(stderr, "Bad char; expected '%c' but got '%c'.\n", chr, c);
        return 0;
    }
    return 1;
}

//  Returns `0` if it fails, `1` if it succeeds. Expects `buf` to be an
//  uninitialized (but allocated) string of length `sz`.
int parse_var(FILE *inp, char* buf, size_t sz)
{
    char c = fgetc(inp);
    if (c == EOF) {
        fprintf(stderr, "Unexpected EOF during `parse_name`.\n");
        return 0;
    }
    int i = 0;
    while ((isalnum(c) || c == '_') && i < sz - 1) {
        buf[i] = c;
        buf[i+1] = '\0';
        c = fgetc(inp);
        if (c == EOF) {
            fprintf(stderr, "Unexpected EOF during `parse_name`.\n");
            return 0;
        }
        i++;
        if (i == sz - 1) {
            char c = fgetc(inp);
            if (isalnum(c) || c == '_') {
                fprintf(stderr, "Too long variable during `parse_name`.");
                return 0;
            } else {
                ungetc(c, inp);
            }
        }
    }    
    ungetc(c, inp);
    return 1;
}

//  Advances the current char of `inp` until it is not a white-space.
void parse_whitespace(FILE *inp)
{
    int c = fgetc(inp);
    if (isspace(c)) {
        parse_whitespace(inp);
    } else {
        ungetc(c, inp);
    }
}

/* ***** ***** */

//  Parsing to canonical encoding.

struct terms1 *parse_terms1(FILE *inp)
{
    parse_whitespace(inp);
    char c = fgetc(inp);
    if (c == '\\') {
        char *variable = malloc(sizeof(char) * 16);
        MALCHECK(variable);
        if (!parse_var(inp, variable, 16)) {free(variable); return NULL;}
        if (!parse_char(inp, '.')) {free(variable); return NULL;}
        struct terms1 *body = parse_terms1(inp);
        if (!body) {return NULL;}
        struct terms1 *lambda = malloc(sizeof(struct terms1));
        if (!lambda) {return NULL;}
        struct lams1 *lambda_lam = malloc(sizeof(struct lams1));
        lambda_lam->var = variable;
        lambda_lam->bod = body;
        lambda->refcnt = 1;
        lambda->tag = LAM1;
        lambda->lam = lambda_lam;
        return lambda;
    }
    if (c == '(') {
        struct terms1 *function = parse_terms1(inp);
        if (!function) {return NULL;}
        struct terms1 *argument = parse_terms1(inp);
        if (!argument) {return NULL;}
        if (!parse_char(inp, ')')) {
            decref_terms1(function); decref_terms1(argument);
            return NULL;
        }
        struct terms1 *application = malloc(sizeof(struct terms1));
        MALCHECK(application);
        struct apps1 *application_app = malloc(sizeof(struct apps1));
        MALCHECK(application_app);
        application_app->fun = function;
        application_app->arg = argument;
        application->refcnt = 1;
        application->tag = APP1;
        application->app = application_app;
        return application;
    }
    ungetc(c, inp);
    char *name = malloc(sizeof(char) * 16);
    MALCHECK(name);
    if (!parse_var(inp, name, 16)) {free(name); return NULL;}
    struct terms1 *variable = malloc(sizeof(struct terms1));
    MALCHECK(variable);
    variable->refcnt = 1;
    variable->tag = VAR1;
    variable->var = name;
    return variable;
}

/* ***** ***** */

//  Parsing to de Bruijn encoding.

struct terms2 *parse_terms2(FILE *inp, struct names *xs)
{
    parse_whitespace(inp);
    char c = fgetc(inp);
    if (c == '\\') {
        char *variable = malloc(sizeof(char) * 16);
        MALCHECK(variable);
        if (!parse_var(inp, variable, 16)) {free(variable); return NULL;}
        push_names(xs, variable);
        if (!parse_char(inp, '.')) {return NULL;}
        struct terms2 *body = parse_terms2(inp, xs);
        if (!body) {return NULL;}
        struct terms2 *lambda = malloc(sizeof(struct terms2));
        MALCHECK(lambda);
        lambda->refcnt = 1;
        lambda->tag = LAM2;
        lambda->lam = body;
        return lambda;
    }
    if (c == '(') {
        struct terms2 *function = parse_terms2(inp, xs);
        if (!function) {return NULL;}
        struct terms2 *argument = parse_terms2(inp, xs);
        if (!argument) {return NULL;}
        if (!parse_char(inp, ')')) {
            decref_terms2(function); decref_terms2(argument);
            return NULL;
        }
        struct terms2 *application = malloc(sizeof(struct terms2));
        MALCHECK(application);
        struct apps2 *application_app = malloc(sizeof(struct apps2));
        MALCHECK(application_app);
        application_app->fun = function;
        application_app->arg = argument;
        application->refcnt = 1;
        application->tag = APP2;
        application->app = application_app;
        return application;
    }
    ungetc(c, inp);
    char *x = malloc(sizeof(char) * 16);
    MALCHECK(x);
    if (!parse_var(inp, x, 16)) {free(x); return NULL;}
    unsigned int idx = get_dbidx(x, xs);
    free(x);
    if (idx == -1) {fprintf(stderr, "Unbound name %s.\n", x);}
    struct terms2 *var2 = malloc(sizeof(struct terms2));
    MALCHECK(var2);
    *var2 = (struct terms2) {.refcnt = 1, .tag = VAR2, .idx = idx};
    return var2;
}

struct terms2 *parse_terms2_nonames(FILE *inp)
{
    struct names *xs = alloc_names(16);
    struct terms2 *result = parse_terms2(inp, xs);
    free_names(xs);
    return result;
}

/* ***** ***** */

//  Contexts.

struct binds1 {
        char *nam;
        struct terms1 *trm;
}; 

struct binds1 mk_binds1(char *name, struct terms1 *term)
{
    return (struct binds1) {.nam = name, .trm = term};
}

struct contexts1 {
    size_t cap;
    size_t num;
    struct binds1 *els;
};

struct contexts1 *alloc_contexts1(size_t cap)
{
    struct contexts1 *ctx = malloc(sizeof(struct contexts1));
    MALCHECK(ctx);
    ctx->cap = cap;
    ctx->num = 0;
    struct binds1 *tmp = malloc(sizeof(struct binds1) * cap);
    MALCHECK(tmp);
    ctx->els = tmp;
    return ctx;
}

void free_contexts1(struct contexts1 *ctx)
{
    if (!ctx) {return;}
    struct binds1 *els = ctx->els;
    for (int i = 0; i < ctx->num; i++) {
        free(els[i].nam); decref_terms1(els[i].trm);
    }
    free(els); free(ctx);
}

void push_contexts1(struct contexts1 *ctx, struct binds1 bnd)
{
    if (ctx->num < ctx->cap) {
       ctx->els[ctx->num] = bnd;
       ctx->num++;
    } else {
        void *tmp = realloc(ctx, (sizeof(ctx) * 3)/2 + 8);
        if (!tmp) {
            free_contexts1(tmp);
            fprintf(stderr, "Failed realloc at line %d in `%s`.\n"
                          , __LINE__, __FUNCTION__);
            return;
        }
        ctx = tmp;
        ctx->cap = ((ctx->cap) * 3)/2 + 8;
        ctx->els[ctx->num] = bnd;
        ctx->num++;
    }
}

struct terms1 *get_ctxterm1(char *x, struct contexts1 *ctx)
{
    struct binds1 *els = ctx->els;
    for (int i = 0; i < ctx->num; i++) {
        if (!strcmp(x, els[i].nam)) {
            incref_terms1(els[i].trm);
            return els[i].trm;
        }
    }
    return NULL;
}

//  de Bruijn contexts.

struct binds2 {
        char *nam;
        struct terms2 *trm;
}; 

struct binds2 mk_binds2(char *name, struct terms2 *term)
{
    return (struct binds2) {.nam = name, .trm = term};
}

struct contexts2 {
    size_t cap;
    size_t num;
    struct binds2 *els;
};

struct contexts2 *alloc_contexts2(size_t cap)
{
    struct contexts2 *ctx = malloc(sizeof(struct contexts1));
    MALCHECK(ctx);
    ctx->cap = cap;
    ctx->num = 0;
    struct binds2 *tmp = malloc(sizeof(struct binds2) * cap);
    MALCHECK(tmp);
    ctx->els = tmp;
    return ctx;
}

void free_contexts2(struct contexts2 *ctx)
{
    if (!ctx) {return;}
    struct binds2 *els = ctx->els;
    for (int i = 0; i < ctx->num; i++) {
        free(els[i].nam); decref_terms2(els[i].trm);
    }
    free(els); free(ctx);
}

void push_contexts2(struct contexts2 *ctx, struct binds2 bnd)
{
    if (ctx->num < ctx->cap) {
       ctx->els[ctx->num] = bnd;
       ctx->num++;
    } else {
        void *tmp = realloc(ctx, (sizeof(ctx) * 3)/2 + 8);
        if (!tmp) {
            free_contexts1(tmp);
            fprintf(stderr, "Failed realloc at line %d in `%s`.\n"
                          , __LINE__, __FUNCTION__);
            return;
        }
        ctx = tmp;
        ctx->cap = ((ctx->cap) * 3)/2 + 8;
        ctx->els[ctx->num] = bnd;
        ctx->num++;
    }
}

struct terms2 *get_ctxterm2(char *x, struct contexts2 *ctx)
{
    struct binds2 *els = ctx->els;
    for (int i = 0; i < ctx->num; i++) {
        if (!strcmp(x, els[i].nam)) {
            incref_terms2(els[i].trm);
            return els[i].trm;
        }
    }
    return NULL;
}

/* ***** ***** */

//  Parsing declarative lambda-terms.

struct terms1 *parse_declterms1(FILE *inp, struct contexts1 *ctx)
{
    parse_whitespace(inp);
    char c = fgetc(inp);
    if (c == '\\') {
        char *variable = malloc(sizeof(char) * 16);
        MALCHECK(variable);
        if (!parse_var(inp, variable, 16)) {free(variable); return NULL;}
        if (!parse_char(inp, '.')) {free(variable); return NULL;}
        struct terms1 *body = parse_declterms1(inp, ctx);
        if (!body) {return NULL;}
        struct terms1 *lambda = malloc(sizeof(struct terms1));
        if (!lambda) {return NULL;}
        struct lams1 *lambda_lam = malloc(sizeof(struct lams1));
        lambda_lam->var = variable;
        lambda_lam->bod = body;
        lambda->refcnt = 1;
        lambda->tag = LAM1;
        lambda->lam = lambda_lam;
        return lambda;
    }
    if (c == '(') {
        struct terms1 *function = parse_declterms1(inp, ctx);
        if (!function) {return NULL;}
        struct terms1 *argument = parse_declterms1(inp, ctx);
        if (!argument) {return NULL;}
        if (!parse_char(inp, ')')) {
            decref_terms1(function); decref_terms1(argument);
            return NULL;
        }
        struct terms1 *application = malloc(sizeof(struct terms1));
        MALCHECK(application);
        struct apps1 *application_app = malloc(sizeof(struct apps1));
        MALCHECK(application_app);
        application_app->fun = function;
        application_app->arg = argument;
        application->refcnt = 1;
        application->tag = APP1;
        application->app = application_app;
        return application;
    }
    if (c == '@') {
        parse_whitespace(inp);
        char *name = malloc(sizeof(char) * 16);
        MALCHECK(name);
        if (!parse_var(inp, name, 16)) {free(name); return NULL;}
        struct terms1 *ctxterm1 = get_ctxterm1(name, ctx);
        if (ctxterm1) {
            fprintf(stderr, "Variable %s already defined.\n", name);
            free(name);
        }
        parse_whitespace(inp);
        parse_char(inp, '=');
        struct terms1 *term = parse_declterms1(inp, ctx);
        struct binds1 bnd = {.nam = name, .trm = term}; 
        push_contexts1(ctx, bnd);
        return term;
    }
    ungetc(c, inp);
    char *name = malloc(sizeof(char) * 16);
    MALCHECK(name);
    if (!parse_var(inp, name, 16)) {free(name); return NULL;}
    struct terms1 *ctxterm1 = get_ctxterm1(name, ctx);
    if (!ctxterm1) {
        struct terms1 *variable = malloc(sizeof(struct terms1));
        MALCHECK(variable);
        variable->refcnt = 1;
        variable->tag = VAR1;
        variable->var = name;
        return variable;
    }
    free(name);
    return ctxterm1;
}

struct terms2 *parse_declterms2(FILE *inp, struct names *xs
                                         , struct contexts2 *ctx)
{
    parse_whitespace(inp);
    char c = fgetc(inp);
    if (c == '\\') {
        char *variable = malloc(sizeof(char) * 16);
        MALCHECK(variable);
        if (!parse_var(inp, variable, 16)) {free(variable); return NULL;}
        push_names(xs, variable);
        if (!parse_char(inp, '.')) {return NULL;}
        struct terms2 *body = parse_declterms2(inp, xs, ctx);
        if (!body) {return NULL;}
        struct terms2 *lambda = malloc(sizeof(struct terms2));
        MALCHECK(lambda);
        lambda->refcnt = 1;
        lambda->tag = LAM2;
        lambda->lam = body;
        return lambda;
    }
    if (c == '(') {
        struct terms2 *function = parse_declterms2(inp, xs, ctx);
        if (!function) {return NULL;}
        struct terms2 *argument = parse_declterms2(inp, xs, ctx);
        if (!argument) {return NULL;}
        if (!parse_char(inp, ')')) {
            decref_terms2(function); decref_terms2(argument);
            return NULL;
        }
        struct terms2 *application = malloc(sizeof(struct terms2));
        MALCHECK(application);
        struct apps2 *application_app = malloc(sizeof(struct apps2));
        MALCHECK(application_app);
        application_app->fun = function;
        application_app->arg = argument;
        application->refcnt = 1;
        application->tag = APP2;
        application->app = application_app;
        return application;
    }
    if (c == '@') {
        parse_whitespace(inp);
        char *name = malloc(sizeof(char) * 16);
        MALCHECK(name);
        if (!parse_var(inp, name, 16)) {free(name); return NULL;}
        struct terms2 *ctxterm2 = get_ctxterm2(name, ctx);
        if (ctxterm2) {
            fprintf(stderr, "Variable %s already defined.\n", name);
            free(name);
        }
        parse_whitespace(inp);
        parse_char(inp, '=');
        struct terms2 *term = parse_declterms2(inp, xs, ctx);
        struct binds2 bnd = {.nam = name, .trm = term}; 
        push_contexts2(ctx, bnd);
        return term;
    }
    ungetc(c, inp);
    char *x = malloc(sizeof(char) * 16);
    MALCHECK(x);
    if (!parse_var(inp, x, 16)) {free(x); return NULL;}
    struct terms2 *ctxterm2 = get_ctxterm2(x, ctx);
    if (!ctxterm2) {
        unsigned int idx = get_dbidx(x, xs);
        free(x);
        if (idx == -1) {fprintf(stderr, "Unbound name %s.\n", x);}
        struct terms2 *var2 = malloc(sizeof(struct terms2));
        MALCHECK(var2);
        *var2 = (struct terms2) {.refcnt = 1, .tag = VAR2, .idx = idx};
        return var2;
    }
    free(x);
    return ctxterm2;
}




