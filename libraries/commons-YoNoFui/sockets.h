#ifndef SOCKET_H_
#define SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

typedef struct{
	char process;
	char message[5]; //TODO ver tamaño mensaje
} t_MessagePackage;

int openServerConnection(int newSocketServerPort, int *socketServer);
int acceptClientConnection(int *socketServer, int *socketClient);
int openClientConnection(char *IPServer, int PortServer, int *socketClient);

#endif /*SOCKET_H_*/
