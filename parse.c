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
        fprintf(stderr, "divide by zero error in applyop()\n");
        return(0);
    }
    switch (op) {
    case op_plus:
        return(a+b);
    case op_minus:
        return(a-b);
    case op_times:
        return(a*b);
    case op_div:
        return(a/b);
    case op_mod:
        return(a%b);
    default:
        fprintf(stderr, "invalid op #%d in applyop()\n", op);
        return(0);
    }
}


static int printop(enum optype op)
{
    switch (op) {
    case op_plus:
        return('+');
    case op_minus:
        return('-');
    case op_times:
        return('*');
    case op_div:
        return('/');
    case op_mod:
        return('%');
    default:
        fprintf(stderr, "invalid op #%d in printexpr()\n", op);
        return(0);
    }
}

int printexpr(struct expr *e, char **s, int ssize)
{
    if (e->subexpr) {
        if (ssize > 0) {
            ssize--;
            *(*s)++ = '(';
        }
        ssize = printexpr(e->subexpr->a, s, ssize);
        if (ssize > 0) {
            ssize--;
            *(*s)++ = printop(e->subexpr->op);
        }
        ssize = printexpr(e->subexpr->b, s, ssize);
        if (ssize > 0) {
            ssize--;
            *(*s)++ = ')';
        }
        *(*s) = '\0';
        return(ssize);
    } else {
        char buf[40];
        int len;
        sprintf(buf, "%d", e->val);
        len = strlen(buf);
        if (len >= ssize)
            return(0);
        strcpy(*s, buf);
        *s += len;
        return(ssize - len);
    }
}


void freeexpr(struct expr *e)
{
    if (e->subexpr) {
        freeexpr(e->subexpr->a);
        freeexpr(e->subexpr->b);
        free(e->subexpr);
    }
    free(e);
}


void freesubexpr(struct expr *e)  /* free e->subexpr only */
{
    if (e->subexpr) {
        freeexpr(e->subexpr->a);
        freeexpr(e->subexpr->b);
        free(e->subexpr);
        e->subexpr = NULL;
    }
}


/* sum  ::=  prod  { ( '+' | '-' ) prod } */
static struct expr *parsesum()
{
    struct expr *e, *e2;

    e = parseprod();
    if (errorstatus || e == NULL)
        return(NULL);
    while (token == tok_plus || token == tok_minus) {
        enum optype op = token;
        consume();
        e2 = parseprod();
        if (errorstatus || e2 == NULL)
            return(NULL);
        e = cons(op, e, e2);
    }
    return(e);
}


/* prod ::=  atom  { ( '*' | '/' | '%' ) atom } */
static struct expr *parseprod()
{
    struct expr *e, *e2;

    e = parseatom();
    if (errorstatus || e == NULL)
        return(NULL);
    while (token == tok_times || token == tok_div || token == tok_mod) {
        enum optype op = token;
        consume();
        e2 = parseatom();
        if (errorstatus || e2 == NULL)
            return(NULL);
        e = cons(op, e, e2);
    }
    return(e);
}


/* atom ::=  intvalue  |  '(' sum ')' */
static struct expr *parseatom()
{
    struct expr *e;
    if (token == tok_value) {
        if ((e = malloc(sizeof(struct expr))) == NULL) {
            errorstatus = "out of memory in parse.c!";
            return(NULL);
        }
        e->subexpr = NULL;
        e->val = val;
        consume();
        return(e);
    } else if (token == tok_lparen) {
        consume();
        if (errorstatus)
            return(NULL);
        e = parsesum();
        if (errorstatus || e == NULL)
            return(NULL);
        if (token == tok_rparen) {
            consume();
            return(e);
        }
        errorstatus = "syntax error when expecting right parenthesis";
        return(NULL);
    } else {
        errorstatus = "syntax error";
        return(NULL);
    }
}


static struct expr *cons(enum optype op, struct expr *a, struct expr *b)
{
    struct expr *e;
    if ((e = malloc(sizeof(struct expr))) == NULL
            || (e->subexpr = malloc(sizeof(struct opexpr))) == NULL) {
        errorstatus = "out of memory in parse.c!";
        return(NULL);
    }
    e->subexpr->op = op;
    e->subexpr->a = a;
    e->subexpr->b = b;
    return(e);
}


static void consume()
{
    while (isspace(*text))
        text++;
    switch (*text) {
    case '\0':
        token = tok_eof;
        break;
    case '(':
        text++;
        token = tok_lparen;
        break;
    case ')':
        text++;
        token = tok_rparen;
        break;
    case '+':
        text++;
        token = tok_plus;
        break;
    case '-':
        text++;
        token = tok_minus;
        break;
    case '*':
        text++;
        token = tok_times;
        break;
    case '/':
        text++;
        token = tok_div;
        break;
    case '%':
        text++;
        token = tok_mod;
        break;
    default:
        if (isdigit(*text)) {
            val = 0;
            while (isdigit(*text))
                val = val * 10 + (*text++ - '0');
            token = tok_value;
        } else {
            static char buf[40];
            strcpy(buf, "unexpected character ");
            if ((*text & 255) < ' ' || (*text & 255) >= 127)
                sprintf(strchr(buf, '\0'), "\\%03o", (*text & 255));
            else
                sprintf(strchr(buf, '\0'), "'%c'", *text);
            errorstatus = buf;
        }
    }
}
