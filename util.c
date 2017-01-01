define NULL 0
#include <string.h>
#include "util.h"


char *extractline(char *p, int size)
        /* returns pointer to string after, or NULL if there isn't an entire
         * line here */
{
    char *nl = memnewline(p, size);
    if (!nl)
        return(NULL);

    /*
     * There are three cases: either this is a lone \r, a lone \n, or a CRLF.
     */
    if (*nl == '\r' && nl - p < size && nl[1] == '\n') {
        /* CRLF */
        *nl = '\0';
        return(nl + 2);
    } else {
        /* lone \n or \r */
        *nl = '\0';
        return(nl + 1);
    }
}


char *memnewline(char *p, int size)  /* finds \r _or_ \n */
        /* This is like min(memchr(p, '\r'), memchr(p, '\n')) */
        /* It is named after memchr().  There's no memcspn(). */
{
    while (size > 0) {
        if (*p == '\r' || *p == '\n')
            return(p);
        p++;
        size--;
    }
    return(NULL);
}