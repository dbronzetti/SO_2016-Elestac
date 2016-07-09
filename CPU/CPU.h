#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include "commons/config.h"
#include "commons/log.h"

#include "sockets.h"

typedef struct {
	int port_Nucleo;
	char *ip_Nucleo;
	int port_UMC;
	char *ip_UMC;
} t_configFile;

t_configFile configuration;

AnSISOP_funciones* funciones;
AnSISOP_kernel* funciones_kernel;
int frameSize = 0;
int stackSize = 0;
int socketUMC = 0;
int socketNucleo = 0;
int QUANTUM_SLEEP = 0 ;
int QUANTUM = 0;
t_log* logCPU;

int ejecutarPrograma(t_PCB *PCB);
int connectTo(enum_processes processToConnect, int *socketClient);
void crearArchivoDeConfiguracion(char *configFile);
void sendRequestToUMC();
void waitRequestFromNucleo(int *socketClient, char * messageRcv);

#endif /* CPU_H_ */
