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
        return(1);
    }

    //SOCKET
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    memset(&peer, '\0', sizeof peer);
    peer.sin_family = AF_INET;
    peer.sin_addr.s_addr = INADDR_ANY;

    //PORT
    if (argc > 2) {
        if (!(port = atoi(argv[2])) == 0) {
            peer.sin_port = htons(port);
        } else {
            fprintf(stderr, "%s: port number must be a positive integer\n", argv[0]);
            return(1);
        }
    } else {
        peer.sin_port = htons(1234);
    }

    //SOCKET
    peer.sin_addr = *((struct in_addr*)(hp->h_addr));
    if (connect(sockfd, (struct sockaddr *)&peer, sizeof(peer)) == -1) {
        perror("connect");
        close(sockfd);
        exit(1);
    }

    FD_ZERO(&master);
    FD_SET(STDIN_FILENO, &master);
    FD_SET(sockfd, &master);
    fd_set fds;

    //BANNER HANDLE
    *p = addclient(sockfd);
    char *buff = malloc(500);
    while (1) {
        buff = myreadline(p);
        if (buff == NULL)
            continue;
        if (!strcmp(buff, CHATSVR_ID_STRING)) {
            write(sockfd, "Marvin", MAXHANDLE);
            break;
        } else {
            fprintf(stderr, "%s: invalid chatsvr\n", buff);
            exit(1);
        }
    }

    //LOOP
    while(1) {
        fds = master;
        choice(sockfd+1, &fds, NULL, NULL, NULL);
        if(FD_ISSET(STDIN_FILENO, &fds)) {
            fgets(buf, sizeof buf, stdin);
            reply(buf, "Marvin", sockfd);
        } else if (FD_ISSET(sockfd, &fds)) {
            name = myreadline(p);
            printf("%s\n", name);
            strtok_r(name, ": ", &todo);
            tracer(todo, 0, 1);
            reply(todo, name, sockfd);
        }
    }
    return(0);
}

//Given name and command, prints required output
void reply(char *buf, char *name, int sockfd) {
    extern int tracer(char *str, int start, int len);
    extern int tinder(const char *a, const char *b);
    char *replied = buf;
    if (strlen(buf) > 0) {
       if (skip < 2) {
            skip++;
            return;
        }
        if (!tinder(buf, "Hey Marvin,")) {
            printf("Marvin: %s", replied);
            //(write(sockfd, replied, sizeof(replied));
            return;
        } else {
            tracer(buf, 0, 11);
        }
        struct expr *e = parse(buf);
        if (e) {
            sprintf(replied, "Marvin: Hey %s, %d", name, evalexpr(e));
            printf("%s\n", replied);
            //(write(sockfd, replied, sizeof(replied));
            freeexpr(e);
        } else {
            sprintf(replied, "Marvin: Hey %s, I don't like that.", name);
            printf("%s\n", replied);
            //(write(sockfd, replied, sizeof(replied));
        }
   }
}

//Runs select
int choice(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
       struct timeval *timeout) {
    int n = select(nfds, readfds, writefds, exceptfds, timeout);
    if (n < 0) {
        perror("select");
        exit(1);
    }
    return(n);
}

//Checks for a match
int tinder(const char *a, const char *b) {
    if(strncmp(a, b, strlen(b)) == 0)
        return 1;
    return 0;
}

//Cuts string to required input
int tracer(char *str, int start, int len) {
    int l = strlen(str);

    if (len < 0) len = l - start;
    if (start + len > l) len = l - start;
    memmove(str + start, str + start + len, l - len + 1);

    return len;
}

//REMAINDER IS CODE FROM CHATSVR
static struct client addclient(int fd) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        fprintf(stderr, "out of memory!\n");  /* highly unlikely to happen */
        exit(1);
    }
    p->fd = fd;
    p->bytes_in_buf = 0;
    p->nextpos = NULL;
    p->name[0] = '\0';
    p->next = top;
    top = p;
    return *p;
}

char *myreadline(struct client *p) {
    //extern char *extractline(char *p, int size);
    int nbytes;

    if (p->bytes_in_buf && p->nextpos)
        memmove(p->buf, p->nextpos, p->bytes_in_buf);

    if ((p->nextpos = extractline(p->buf, p->bytes_in_buf))) {
        p->bytes_in_buf -= (p->nextpos - p->buf);
        return(p->buf);
    }

    nbytes = read(p->fd, p->buf + p->bytes_in_buf, sizeof p->buf - p->bytes_in_buf - 1);

    if (nbytes <= 0) {
        if (nbytes < 0)
            perror("read()");
    } else {
        p->bytes_in_buf += nbytes;
        if ((p->nextpos = extractline(p->buf, p->bytes_in_buf))) {
            p->bytes_in_buf -= (p->nextpos - p->buf);
            return(p->buf);
        }

        if (p->bytes_in_buf >= MAXMESSAGE) {
            p->buf[p->bytes_in_buf] = '\0';
            p->bytes_in_buf = 0;
            p->nextpos = NULL;
            return(p->buf);
        }
    }
    return(NULL);
}
