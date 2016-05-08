/*
 * nucleo.h
 *
 */

#ifndef NUCLEO_H_
#define NUCLEO_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sockets.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/temporal.h>
#include <commons/process.h>
#include <commons/txt.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/queue.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <netdb.h>
#include <unistd.h>

// Estructuras
typedef struct{
	int PUERTO_PROG;
	int PUERTO_CPU;
	int QUANTUM;
	int QUANTUM_SLEEP;
	char SEM_IDS[20];
	int SEM_INIT[20];
	char IO_IDS[20];
	int IO_SLEEP[20];
	char SHARED_VARS[20];
	char *path;
	t_dictionary *properties;
}configFile;

typedef struct{
	int longitud;
	int desplazamiento;
}t_cod;

typedef struct{
	/*serialización de un diccionario que asocia el identificador de cada
	función y etiqueta del programa con la primer instrucción ejecutable de la misma*/
}t_etiqueta;

typedef struct{
	t_list* args;
	t_list*	vars;
	int retPos;
	int retVar;
}t_stack;

//Estructura PCB
typedef struct PCB{
	int pid;
	int estado;//0: New, 1: Ready, 2: Exec, 3: Block, 4:5: Exit
	int finalizar;
	int pc;
	int sp;
	char path[250];
	int paginasDeCodigo;
	t_cod indiceDeCodigo;
	t_etiqueta indiceDeEtiquetas;
	t_stack indiceDeStack;

}t_pcb;

//Estructura Procesos Bloqueados
typedef struct procesosbloq{
	int pid;
	int tiempo;
}t_bloqueado;

//Estructura Procesos (en cola)
typedef struct procesos{
	int pid;
	int pc;
}t_proceso;

typedef struct io{
	int socket;
	int pid;
	char ids[];
}t_entradasalida;

/*Para Serializacion de CPU (en sockets.h?)
typedef struct datosCPU{
	int id;
	int numSocket;
	int estado;
}t_cpu;*/

//Logger
t_log* logNucleo;

//Configuracion
configFile* configNucleo;

//Variables de Listas
t_list* listaCPU;
t_list* listaProcesos;

//Variables de Colas
t_queue* colaListos;
t_queue* colaBloqueados;
t_queue* colaFinalizar;

//Variables Globales
int socketServer;
int socketClient;
int idProcesos=0;
int numCPU=0;


#endif /* NUCLEO_H_ */
