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
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include "commons/log.h"
#include "commons/config.h"
#include "sockets.h"

//Encabezamiento de Funciones

int frameSize = 0;

typedef struct {
	int port_Nucleo;
	char* ip_Nucleo;
} t_configFile;

//Logger
t_log* logConsola;

//Configuracion
t_configFile configConsola;

char* leerArchivoYGuardarEnCadena();
int reconocerComando(char* comando);
void crearArchivoDeConfiguracion();
int connectTo(enum_processes processToConnect, int *socketClient);
int reconocerOperacion();

#endif /* CONSOLA_H_ */
