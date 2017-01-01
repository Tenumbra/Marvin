/* #include <stdio.h> before including me */

enum tokentype {
    tok_eof,
    tok_value,
    tok_lparen, tok_rparen,
    tok_plus, tok_minus, tok_times, tok_div, tok_mod
};

/* "enum optype" is a subset of "enum tokentype", with matching values */
#define optype tokentype
#define op_plus tok_plus
#define op_minus tok_minus
#define op_times tok_times
#define op_div tok_div
#define op_mod tok_mod

struct expr {
    /*
     * If subexpr is null, then val is the integer constant value of this
     * expression.  If subexpr is non-null, then val is ignored.
     */
    struct opexpr *subexpr;
    int val;
};

struct opexpr {
    enum optype op;
    struct expr *a, *b;
};

extern struct expr *parse(char *s);
extern int evalexpr(struct expr *e);
extern int applyop(enum optype op, int a, int b);
/* printexpr() produces a zero-terminated string and returns space
 * remaining in buf.  It returns zero if the string didn't fit. */
extern int printexpr(struct expr *e, char **buf, int bufsize);
extern void freeexpr(struct expr *e);
extern void freesubexpr(struct expr *e);  /* free e->subexpr only */

/*
 * After a NULL return from parse(), this string is a user-targetted error
 * message:
 */
extern char *errorstatus;