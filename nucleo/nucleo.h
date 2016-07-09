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
#include "commons/log.h"
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>


// Estructuras
typedef struct {
int puerto_prog;
int puerto_cpu;
int puerto_umc;
char* ip_umc;
int quantum;
int quantum_sleep;
char** sem_ids;
int* sem_init;
char** io_ids;
int* io_sleep;
char** shared_vars;
int** shared_vars_values;
int stack_size;
int pageSize;
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

//Estructura datosCPU
typedef struct {
	int numSocket;
	int estadoCPU;
} t_datosCPU;

//Semaforos
pthread_mutex_t listadoCPU;
pthread_mutex_t listadoProcesos;
pthread_mutex_t cListos;
pthread_mutex_t cBloqueados;
pthread_mutex_t cFinalizar;
pthread_mutex_t socketMutex;
pthread_mutex_t activeProcessMutex;
pthread_mutex_t mutex_config;

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
int frameSize = 0;

//Encabezamientos de Funciones Principales

void runScript(char* codeScript);
void planificarProceso();
int procesarMensajeCPU(t_PCB* datosPCB, t_proceso* datosProceso,t_MessageNucleo_CPU* contextoProceso,int libreCPU);
void finalizaProceso(int socket, int PID, int estado);
void deserializarES(t_es* datos, char* buffer);
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

//Encabezamiento de Funciones de Stack

void armarIndiceDeEtiquetas(t_PCB unBloqueControl,t_metadata_program* miMetaData);
void armarIndiceDeCodigo(t_PCB unBloqueControl,t_metadata_program* miMetaData);
int definirVar(char* nombreVariable,t_registroStack miPrograma,int posicion);

//Privilegiadas perro

t_valor_variable* obtenerValor(t_nombre_compartida variable);
void grabarValor(t_nombre_compartida variable, t_valor_variable* valor);
void EntradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void wait(t_nombre_semaforo identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);




//Conexiones y Funciones para los mensajes

void startServerProg();
void startServerCPU();
void newClients(void *parameter);
void processMessageReceived(void *parameter);
void handShake(void *parameter);
int connectTo(enum_processes processToConnect, int *socketClient);
int procesarRespuesta(int libre);
void iniciarPrograma(int PID, char* codeScript);
void finalizarPrograma(int PID);

#endif /* NUCLEO_H_ */
