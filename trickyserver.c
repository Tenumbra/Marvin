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
    fflush(stdout);

    write(clientfd, greeting, sizeof greeting - 1);
    printf("sent banner\n");
    fflush(stdout);

    for (i = 0; i < 5; i++) {
        sleep(1);
        write(clientfd, message1 + i * 5, 5);
        printf("sent %.5s (but no newline, so you shouldn't react yet)\n", message1 + i * 5);
        fflush(stdout);
    }

    printf("[slowing down now]\n");
    fflush(stdout);
    sleep(2);
    write(clientfd, message2, sizeof message2 - 1);
    printf("sent newline (so you should see all of the above) plus more text (which shouldn't do anything yet)\n");
    fflush(stdout);

    sleep(3);
    write(clientfd, message3, sizeof message3 - 1);
    printf("sent newline (so you should see \"something\")\n");
    fflush(stdout);

    sleep(2);
    return(0);
}


void setup()  /* bind and listen, abort on error */
{
    struct sockaddr_in r;

    (void)signal(SIGPIPE, SIG_IGN);

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    memset(&r, '\0', sizeof r);
    r.sin_family = AF_INET;
    r.sin_addr.s_addr = INADDR_ANY;
    r.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *)&r, sizeof r)) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 5)) {
        perror("listen");
        exit(1);
    }
}
