#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/signal.h>
#include "chatsvr.h"
#include "util.h"

int port = 1234;

static int listenfd;

struct client {
    int fd;
    char buf[MAXMESSAGE+16];  /* partial line(s) */
    int bytes_in_buf;  /* how many data bytes in buf (after nextpos) */
    char *nextpos;  /* if non-NULL, move this down to buf[0] before reading */
    char name[MAXHANDLE + 1];  /* name[0]==0 means no name yet */
    struct client *next;
} *top = NULL;

static void addclient(int fd);
static void removeclient(struct client *p);
static void broadcast(char *s, int size);

static void read_and_process(struct client *p);
static char *myreadline(struct client *p);
static void cleanupstr(char *s);


int main(int argc, char **argv)
{
    int c;
    struct client *p, *nextp;
    extern void setup(), newconnection(), whatsup(struct client *p);

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

    /* the only way the server exits is by being killed */
    for (;;) {
        fd_set fdlist;
        int maxfd = listenfd;
        FD_ZERO(&fdlist);
        FD_SET(listenfd, &fdlist);
        for (p = top; p; p = p->next) {
            FD_SET(p->fd, &fdlist);
            if (p->fd > maxfd)
                maxfd = p->fd;
        }
        if (select(maxfd + 1, &fdlist, NULL, NULL, NULL) < 0) {
            perror("select");
        } else {
            if (FD_ISSET(listenfd, &fdlist))
                newconnection();
            for (p = top; p; p = nextp) {
                nextp = p->next;  /* in case we remove this client because of error */
                if (FD_ISSET(p->fd, &fdlist))
                    read_and_process(p);
            }
        }
    }
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


void newconnection()  /* accept connection, update linked list */
{
    int fd;
    struct sockaddr_in r;
    socklen_t len = sizeof r;

    if ((fd = accept(listenfd, (struct sockaddr *)&r, &len)) < 0) {
        perror("accept");
    } else {
        static char greeting[] = CHATSVR_ID_STRING "\r\n";
        printf("new connection from %s, fd %d\n", inet_ntoa(r.sin_addr), fd);
        fflush(stdout);
        addclient(fd);
        write(fd, greeting, sizeof greeting - 1);
    }
}


/* select() said activity; check it out */
static void read_and_process(struct client *p)
{
    char msg[MAXHANDLE + 2 + MAXMESSAGE + 2 + 1];
    char *s = myreadline(p);
    if (!s)
        return;
    if (p->name[0]) {
        sprintf(msg, "%s: %s\r\n", p->name, s);
        broadcast(msg, strlen(msg));
    } else {
        strncpy(p->name, s, MAXHANDLE);
        p->name[MAXHANDLE] = '\0';
        cleanupstr(p->name);
        if (p->name[0]) {
            printf("fd %d is using handle '%s'\n", p->fd, p->name);
            sprintf(msg, "chatsvr: Welcome to our new participant, %s\r\n", p->name);
            broadcast(msg, strlen(msg));
        } else {
            static char botchmsg[] = "chatsvr: protocol botch\r\n";
            write(p->fd, botchmsg, sizeof botchmsg - 1);
            printf("Disconnecting fd %d because of protocol botch in handle registration\n", p->fd);
            fflush(stdout);
            close(p->fd);
            removeclient(p);
        }
    }
}


char *myreadline(struct client *p)
{
    int nbytes;

    /* move the leftover data to the beginning of buf */
    if (p->bytes_in_buf && p->nextpos)
        memmove(p->buf, p->nextpos, p->bytes_in_buf);

    /* If we've already got another whole line, return it without a read() */
    if ((p->nextpos = extractline(p->buf, p->bytes_in_buf))) {
        p->bytes_in_buf -= (p->nextpos - p->buf);
        return(p->buf);
    }

    /*
     * Ok, try a read().  Note that we _never_ fill the buffer, so that there's
     * always room for a \0.
     */
    nbytes = read(p->fd, p->buf + p->bytes_in_buf, sizeof p->buf - p->bytes_in_buf - 1);
    if (nbytes <= 0) {
        if (nbytes < 0)
            perror("read()");
        if (p->name[0]) {
            char msg[MAXHANDLE + 40];
            printf("Disconnecting fd %d, name %s\n", p->fd, p->name);
            fflush(stdout);
            sprintf(msg, "chatsvr: Goodbye, %s\r\n", p->name);
            removeclient(p);
            broadcast(msg, strlen(msg));
        } else {
            printf("Disconnecting fd %d, no name\n", p->fd);
            fflush(stdout);
            removeclient(p);
        }
    } else {

        p->bytes_in_buf += nbytes;

        /* So, _now_ do we have a whole line? */
        if ((p->nextpos = extractline(p->buf, p->bytes_in_buf))) {
            p->bytes_in_buf -= (p->nextpos - p->buf);
            return(p->buf);
        }

        /*
         * Don't do another read(), to avoid the possibility of blocking.
         * However, if we've hit the maximum message size, we should call
         * it all a line.
         */
        if (p->bytes_in_buf >= MAXMESSAGE) {
            p->buf[p->bytes_in_buf] = '\0';
            p->bytes_in_buf = 0;
            p->nextpos = NULL;
            return(p->buf);
        }

    }

    /* If we get to here, we don't have a full input line yet. */
    return(NULL);
}


static void addclient(int fd)
{
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
}


static void removeclient(struct client *p)
{
    struct client **pp, *t;
    for (pp = &top; *pp && *pp != p; pp = &(*pp)->next)
        ;
    if (*pp) {
        close((*pp)->fd);
        t = (*pp)->next;
        free(*pp);
        *pp = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n", p->fd);
        fflush(stderr);
    }
}


static void broadcast(char *s, int size)
{
    struct client *p, *nextp;
    for (p = top; p; p = nextp) {
        nextp = p->next;  /* in case we remove this client because of error */
        if (p->name[0]) {
            if (write(p->fd, s, size) != size) {
                perror("write()");
                removeclient(p);
            }
        }
    }
}


static void cleanupstr(char *s)
{
    /* ajr, 16 May 1988. */
    /* colon-handling added for chatsvr, Nov 2003 */
    char *t = s;
    enum {
        IGNORESPACE,  /* initial state - spaces should be ignored. */
        COPYSPACE,    /* just saw a letter - next space should be copied. */
        COPIEDSPACE   /* just copied a space - spaces should be ignored,
                       * and if this is the last space it should be removed. */
    } state = IGNORESPACE;

    while (*t) {
        if (isspace(*t)) {
            if (state == COPYSPACE) {
                /* It's a space, but we should copy it. */
                *s++ = *t++;

                /*
                 * However, we shouldn't copy another one, and if this space is
                 * last it should be removed later.
                 */
                state = COPIEDSPACE;

            } else {
                /* An extra space; skip it. */
                t++;
            }
        } else if (iscntrl(*t)) {
            /* control character.  Skip it. */
            t++;
        } else if (*t == ':' || *t == ',') {
            /*
             * transform colons and commas to avoid parsing errors in
             * chatsvr messages
             */
            *s++ = ';';
            t++;
        } else {

            /* Not a space.  So copy it. */
            *s++ = *t++;

            /* If the next character is a space, it should be copied. */
            state = COPYSPACE;
        }
    }

    if (state == COPIEDSPACE) {
        /* Go back one, as the last character copied was a space. */
        s--;
    }

    /* Chop off the string at wherever we've copied to. */
    *s = '\0';
}
