#ifndef SOCKET_H_
#define SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int openServerConnection(int newSocketServerPort, int *socketServer);
int acceptClientConnection(int *socketServer, int *socketClient);

#endif /*SOCKET_H_*/
