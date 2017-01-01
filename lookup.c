include <stdio.h>
#include <netdb.h>


int main(int argc, char **argv)
{
    struct hostent *hp;

    if (argc != 2) {
        fprintf(stderr, "usage: %s hostname\n", argv[0]);
        return(1);
    }

    if ((hp = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr, "%s: no such host\n", argv[1]);
        return(1);
    }
    if (hp->h_addr_list[0] == NULL || hp->h_addrtype != AF_INET) {
        fprintf(stderr, "%s: not an internet protocol host name\n", argv[1]);
        return(1);
    }

    printf("lookup of %s successful\n", argv[1]);
    /*
     * then if you have (for example) "struct sockaddr_in r", you can set the
     * r.sin_addr member with:
                memcpy(&r.sin_addr, hp->h_addr_list[0], hp->h_length);
     */

    return(0);
}