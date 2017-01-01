#include <stdio.h>
#include "parse.h"

int main()
{
    char buf[500];
    while (fgets(buf, sizeof buf, stdin)) {
        struct expr *e = parse(buf);
        if (e) {
            printf("value is %d\n", evalexpr(e));
            freeexpr(e);
        } else {
            printf("%s\n", errorstatus);
        }
    }
    return(0);
}