/*
 * consola.h
 *
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

#define PROMPT "anSISOP> "

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include "commons/log.h"
#include "commons/config.h"
#include "common-types.h"
#include "sockets.h"

int frameSize = 0;

typedef struct {
	char ip_nucleo[15];
	int puerto_nucleo;
} t_configFile;

typedef struct {
	int socketServer;
	int socketClient;
} t_serverData;

//Logger
t_log* logConsola;

//Configuracion
t_configFile configConsola;

//Encabezamiento de Funciones Principales

char* leerArchivoYGuardarEnCadena();
int reconocerComando(char*);
int conectarAlNucleo(char*);

#endif /* CONSOLA_H_ */
