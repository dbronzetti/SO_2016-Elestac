/*
 * consola.h
 *
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

#define PROMPT "anSISOP> "

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include "commons/log.h"
//#include "common-types.h"
//#include "sockets.h"

typedef struct {
	int port;
	char ip_nucleo[15];
	int port_nucleo;
} t_configFile;

typedef struct {
	int socketServer;
	int socketClient;
} t_serverData;

//Logger
t_log* logConsola;

//Configuracion
t_configFile configuration;

//Encabezamiento de Funciones Principales

void leerArchivoYGuardarEnCadena();

int ReconocerComando(char *);

//Encabezamiento de Funciones Secundarias

void startServer();
void newClients(void *parameter);
void handShake(void *parameter);
void processMessageReceived(void *parameter);

#endif /* CONSOLA_H_ */
