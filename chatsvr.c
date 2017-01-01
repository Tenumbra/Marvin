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