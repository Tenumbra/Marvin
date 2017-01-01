#include <stdio.h>
#include "parse.h"
#include "chatsvr.h"
#include "util.h"
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

int port = 1234;
int skip = 1;

struct client {
    int fd;
    char buf[MAXMESSAGE+16];  /* partial line(s) */
    int bytes_in_buf;  /* how many data bytes in buf (after nextpos) */
    char *nextpos;  /* if non-NULL, move this down to buf[0] before reading */
    char name[MAXHANDLE + 1];  /* name[0]==0 means no name yet */
    struct client *next;
} *top = NULL;

static struct client addclient(int fd);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s hostname [port number] ...\n", argv[0]);
        return(1);
    }

    //INITIALIZERS
    int sockfd;
    fd_set master;
    char buf[500];
    struct hostent *hp;
    struct sockaddr_in peer;
    char *name = malloc(MAXHANDLE);
    char *todo = malloc(MAXMESSAGE);
    extern void reply(char *buf, char *name, int sockfd);
    extern char *myreadline(struct client *p);
    struct client *p = malloc(sizeof(struct client));
    extern int tracer(char *str, int start, int len);
    extern int choice(int nfds, fd_set *readfds, fd_set *writefds,
            fd_set *exceptfds, struct timeval *timeout);

    //HOST
    if ((hp = gethostbyname(argv[1])) == NULL) {
        fprintf(stderr, "%s: no such host\n", argv[1]);
        return(1);
    }

    if (hp->h_addr_list[0] == NULL || hp->h_addrtype != AF_INET) {
        fprintf(stderr, "%s: not an internet protocol host name\n", argv[1]);