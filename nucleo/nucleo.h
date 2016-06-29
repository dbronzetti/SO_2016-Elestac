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
	char* sem_ids;
	int* sem_init;
	char* io_ids;
	int* io_sleep;
	char* shared_vars;
	int stack_size;
	int frames_size;
} t_configFile;

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

typedef struct {
	int socket;
	int PID;
	char ids[];
} t_entradasalida;

//Configuracion
t_configFile configNucleo;

//Logger
t_log* logNucleo;

//Variables de Listas
t_list* listaCPU;
t_list* listaProcesos;

//Variables de Colas
t_queue* colaListos;
t_queue* colaBloqueados;
t_queue* colaFinalizar;

//Variables Globales
int idProcesos = 1;
int numCPU = 0;

//Encabezamientos de Funciones Principales

void runScript(char*);
void planificarProceso(void);
void finalizaProceso(int, int, int);
void hacerEntradaSalida(int, int, int, int);
void atenderBloqueados();
void atenderCorteQuantum(int, int);

//Encabezamiento de Funciones Secundarias

int buscarCPULibre(void);
int buscarPCB(int);
void cambiarEstadoProceso(int, int);
void liberarCPU(int);
int buscarCPU(int);
void actualizarPC(int, int);
void crearArchivoDeConfiguracion(char*);

//Conexiones

void startServer();
void newClients(void *parameter);
void processMessageReceived(void *parameter);
void handShake(void *parameter);

#endif /* NUCLEO_H_ */
