#ifndef SOCKET_H_
#define SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
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

typedef enum{
	NEW = 0,
	READY,
	RUNNING,
	BLOCKED,
	SUSPENDED
} enum_processStatus;

typedef struct{
	enum_processes process;
	char *message;
} t_MessageGenericHandshake;

typedef struct{
	int processID;
	enum_processStatus processStatus;
	int totalPages;
	int pageNro;
} t_MessageUMC_Swap;

typedef struct{
	int processID;
	enum_processStatus processStatus;
	int totalPages;
	int pageNro;
} t_MessageUMC_CPU;

typedef struct{
	enum_processStatus processStatus;
} t_MessageUMC_Nucleo;

typedef struct{
	enum_processStatus processStatus;
} t_MessageNucleo_UMC;

typedef struct{
	int head;
	int processID;
	enum_processStatus processStatus;
	char* codeScript;
	int ProgramCounter;
	int cantInstruc;
	int operacion;
	int numSocket;
	int id;
} t_MessageNucleo_CPU;

typedef struct{
	int processID;
	char* codeScript;
	enum_processStatus processStatus;
} t_MessageNucleo_Consola;

typedef struct{
	int processID;
	enum_processStatus processStatus;
} t_MessageCPU_Nucleo;

int openServerConnection(int newSocketServerPort, int *socketServer);
int acceptClientConnection(int *socketServer, int *socketClient);
int openClientConnection(char *IPServer, int PortServer, int *socketClient);
int sendClientHandShake(int *socketClient, enum_processes process);
int sendClientAcceptation(int *socketClient);
int sendMessage (int *socketClient, char *buffer, int bufferSize);
int receiveMessage(int *socketClient, char *messageRcv, int bufferSize);
void serializeHandShake(t_MessageGenericHandshake *value, char *buffer, int valueSize);
void deserializeHandShake(t_MessageGenericHandshake *value, char *bufferReceived);

//IMPORTANTE!!! --> Nomeclatura de Serializadores y Deserealizadores
//1) serialize<FromProcess>_<ToProcess> ()
//2) deserialize<ToProcess>_<FromProcess> ()

void serializeUMC_Swap(t_MessageUMC_Swap *value, char *buffer, int valueSize);
void deserializeSwap_UMC(t_MessageUMC_Swap *value, char *bufferReceived);

void serializeUMC_Nucleo(t_MessageUMC_Nucleo *value, char *buffer, int valueSize);
void deserializeNucleo_UMC(t_MessageUMC_Nucleo *value, char *bufferReceived);

void serializeUMC_CPU(t_MessageUMC_CPU *value, char *buffer, int valueSize);
void deserializeCPU_UMC(t_MessageUMC_CPU *value, char *bufferReceived);

void serializeNucleo_CPU(t_MessageNucleo_CPU *value, char *buffer, int valueSize);
void deserializeCPU_Nucleo(t_MessageNucleo_CPU *value, char *bufferReceived);

void deserializeConsola_Nucleo(t_MessageNucleo_Consola *value, char *bufferReceived);

#endif /*SOCKET_H_*/
