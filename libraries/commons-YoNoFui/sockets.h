#ifndef SOCKET_H_
#define SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

typedef enum{
	ACCEPTED=0,
	CONSOLA,
	NUCLEO,
	CPU,
	UMC,
	SWAP
} enum_processes;

typedef struct{
	enum_processes process;
	void* message;
} t_MessagePackage;

int openServerConnection(int newSocketServerPort, int *socketServer);
int acceptClientConnection(int *socketServer, int *socketClient);
int openClientConnection(char *IPServer, int PortServer, int *socketClient);
int sendClientAcceptation(int *socketClient, fd_set *readSocketSet);
int receiveMessage(int *socketClient, t_MessagePackage *messageRcv);

#endif /*SOCKET_H_*/
