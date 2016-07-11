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
#include "common-types.h"

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
t_PCB* PCB;
int ultimoPosicionPC;


int ejecutarPrograma(t_PCB *PCB);
int connectTo(enum_processes processToConnect, int *socketClient);
void crearArchivoDeConfiguracion(char *configFile);
void sendRequestToUMC();
void waitRequestFromNucleo(int *socketClient, char * messageRcv);

void manejarES(char* instruccion,int PID, int pcActualizado, int banderaFinQuantum);
char* leerParametro(char* instruccion);
//Encabezamientos primitivas

t_puntero definirVariable(t_nombre_variable nombreVariable);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void retornar(t_valor_variable retorno);
void imprimir(t_valor_variable valor_mostrar);
void imprimirTexto(char *texto);
void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void wait(t_nombre_semaforo identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);

int condicionBuscarVarible(t_vars* variableBuscada,t_vars* otraVariable);
t_vars* buscarEnElStackPosicionPagina(t_PCB* pcb);
t_memoryLocation* buscarUltimaPosicionOcupada(t_PCB* pcbEjecutando);


#endif /* CPU_H_ */
