/*
 * nucleo.h
 *
 */

#ifndef NUCLEO_H_
#define NUCLEO_H_
#define EOL_DELIMITER ";"

#include "common-types.h"
#include "sockets.h"
#include "commons/log.h"
#include "commons/temporal.h"
#include "commons/process.h"
#include "commons/txt.h"
#include "commons/collections/list.h"
#include "commons/collections/dictionary.h"
#include "commons/collections/queue.h"
#include "commons/config.h"
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>

// Estructuras
typedef struct {
	int puerto_prog;
	int puerto_cpu;
	int quantum;
	int quantum_sleep;
	char sem_ids[20];
	int sem_init[20];
	char io_ids[20];
	int io_sleep[20];
	char shared_vars[20];
	int stack_size;
} t_configFile;

typedef enum {
	PUERTO_PROG = 0,
	PUERTO_CPU,
	QUANTUM,
	QUANTUM_SLEEP,
	SEM_IDS,
	SEM_INIT,
	IO_IDS,
	IO_SLEEP,
	SHARED_VARS,
	STACK_SIZE
} enum_configParameters;

typedef struct {
	int socketServer;
	int socketClient;
} t_serverData;

//Estructura Procesos Bloqueados
typedef struct {
	int PID;
	int tiempo;
} t_bloqueado;

//Estructura Procesos en cola
typedef struct {
	int PID;
	int ProgramCounter;
} t_proceso;

//Semaforos
pthread_mutex_t listadoCPU;
pthread_mutex_t cListos;
pthread_mutex_t cBloqueados;
pthread_mutex_t cFinalizar;
pthread_mutex_t socketMutex;

//Semaforo Contador
sem_t semBloqueados;

typedef struct datosEntradaSalida {
	int tiempo;
	int ProgramCounter;
} t_es;

typedef struct {
	int socket;
	int PID;
	char ids[];
} t_entradasalida;

//Logger
t_log* logNucleo;

//Configuracion
t_configFile configuration; //version UMC
//t_configFile* configNucleo; //version swap

//Variables de Listas
t_list* listaCPU;
t_list* listaProcesos;

//Variables de Colas
t_queue* colaListos;
t_queue* colaBloqueados;
t_queue* colaFinalizar;

//Variables Globales
int idProcesos = 0;
int numCPU = 0;

//Encabezamientos de Funciones Principales

void correrPath(char*);
void planificarProceso(void);
void finalizaProceso(int, int, int);
void hacerEntradaSalida(int, int, int, int);
void atenderBloqueados();
void atenderCorteQuantum(int, int);

//Encabezamiento de Funciones Secundarias

void startServer();
void newClients(void *parameter);
void processMessageReceived(void *parameter);
void handShake(void *parameter);
void deserializeIO(t_es*, char**);
int buscarCPULibre(void);
int buscarPCB(int);
void cambiarEstadoProceso(int, int);
void liberarCPU(int);
int buscarCPU(int);
void actualizarPC(int, int);
void getConfiguration(char *configFile);
int getEnum(char *string);

#endif /* NUCLEO_H_ */
