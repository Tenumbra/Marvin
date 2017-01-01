#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "parse.h"

/*
 * Grammar (where "{}" is zero or more of):
 *   sum  ::=  prod  { ( '+' | '-' ) prod }
 *   prod ::=  atom  { ( '*' | '/' | '%' ) atom }
 *   atom ::=  intvalue  |  '(' sum ')'
 * (The start symbol is "sum")
 */

static char *text;  /* text we're parsing */
static enum tokentype token;  /* analysis of the next token */
static int val;  /* only if token == tok_value */
static void consume();

static struct expr *parsesum();
static struct expr *parseprod();
static struct expr *parseatom();

char *errorstatus;  /* when non-NULL, return NULL as soon as convenient */
static struct expr *cons(enum optype op, struct expr *a, struct expr *b);


struct expr *parse(char *s)
{
    struct expr *e;
    text = s;
    errorstatus = NULL;
    consume();
    if (errorstatus)
        return(NULL);
    e = parsesum();
    if (errorstatus)
        return(NULL);
    if (token == tok_eof)
        return(e);
    errorstatus = "syntax error: excess text";
    return(NULL);
}


int evalexpr(struct expr *e)
{
    if (e->subexpr)
        return(applyop(e->subexpr->op, evalexpr(e->subexpr->a),
                        evalexpr(e->subexpr->b)));
    else
        return(e->val);
}


int applyop(enum optype op, int a, int b)
{
    if (b == 0 && (op == op_div || op == op_mod)) {                                                                     