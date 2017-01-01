# Marvin
Server bot that can be asked basic questions

Marvin will connect to a chat server and in addition to being a normally-functioning client, it will respond to certain messages sent by other users.

The program will sign in with the user name "Marvin". When another user says "Hey Marvin," (case-insensitive comparison) followed by an arithmetic expression, he will compute this expression and reply "Hey [username], [result]".

One of the files is parse.c, which can parse and compute the value of these expressions. The file testparse.c can help with parsing any future "bots" written in c.

The user of the program can also type messages on stdin, and the program shows everything from the server on stdout. Thus if no one says "Hey Marvin,", the only difference between this and a proper chat client is that it doesn't let you enter your "handle".

Note that the supplied chat server broadcasts all messages to all clients, including the client that sent them; thus the user will see the messages you generate, prefaced by "Marvin:".
