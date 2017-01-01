#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "chatsvr.h"

int port = 1234;

static int listenfd;


int main(int argc, char **argv)
{
    int c, clientfd, i;
    struct sockaddr_in r;
    socklen_t len;
    static char greeting[] = CHATSVR_ID_STRING "\r\n";
    static char message1[] = "abcdefghijklmnopqrstuvwxy";
    static char message2[] = "\r\nsomething";
    static char message3[] = "\r\n";
    extern void setup();

    while ((c = getopt(argc, argv, "p:")) != EOF) {
        if (c == 'p') {
            if ((port = atoi(optarg)) == 0) {
                fprintf(stderr, "%s: port number must be a positive integer\n", argv[0]);
                return(1);
            }
        } else {
            fprintf(stderr, "usage: %s [-p port]\n", argv[0]);
            return(1);
        }
    }

    setup();  /* aborts on error */

    if ((clientfd = accept(listenfd, (struct sockaddr *)&r, &len)) < 0) {
        perror("accept");
        return(1);
    }
    printf("new connection from %s, fd %d\n", inet_ntoa(r.sin_addr), clientfd);