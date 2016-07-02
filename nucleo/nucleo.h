/*
 * nucleo.h
 *
 */

#ifndef NUCLEO_H_
#define NUCLEO_H_
#define EOL_DELIMITER ";"

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
char** sem_ids;
int** sem_init;
char** io_ids;
int* io_sleep;
char** shared_vars;
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
	t_nombre_dispositivo dispositivo;
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
int activePID = 0;
int socketUMC = 0;
pthread_mutex_t activeProcessMutex;

//Encabezamientos de Funciones Principales

void runScript(char* codeScript);
void planificarProceso();
void finalizaProceso(int socket, int PID, int estado);
void hacerEntradaSalida(int socket, int PID, int ProgramCounter, t_nombre_dispositivo dispositivo, int tiempo);
void entrada_salida(t_nombre_dispositivo dispositivo, int tiempo);
void atenderBloqueados();
void atenderCorteQuantum(int socket, int PID);

//Encabezamiento de Funciones Secundarias

int buscarCPULibre();
int buscarPCB(int id);
void cambiarEstadoProceso(int PID, int estado);
void liberarCPU(int socket);
int buscarCPU(int socket);
void actualizarPC(int PID, int ProgramCounter);
void crearArchivoDeConfiguracion(char* configFile);

//Conexiones

void startServer();
void newClients(void *parameter);
void processMessageReceived(void *parameter);
void handShake(void *parameter);

#endif /* NUCLEO_H_ */
