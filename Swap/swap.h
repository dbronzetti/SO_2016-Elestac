/*
 * swap.h
 *
 *  Created on: 27/6/2016
 *      Author: utnso
 */

#ifndef SWAP_H_
#define SWAP_H_

/*
 * swap.h
 *
 *  Created on: 25/6/2016
 *      Author: utnso
 */

/*
#define agregar_proceso 1
#define finalizar_proceso 2
#define lectura_pagina 3
#define escritura_pagina 4
*/

#include <stdlib.h>
#include <stdbool.h>
#include "commons/string.h"
#include <string.h>
#include <sys/mman.h>
#include "commons/config.h"
#include "commons/log.h"
#include "stdbool.h"

struct bloqueDeMemoria{
	int PID;
	int cantDePaginas;
	int ocupado;
	int tamanioDelBloque;
	int paginaInicial;
}typedef bloqueSwap;

struct nuevoPrograma{
	int PID;
	int cantidadDePaginas;
	char* contenido;
}typedef nuevo_programa;

struct escribirPagina{
	int PID;
	int nroPagina;
	char* contenido;
}typedef escribir_pagina;

struct leerPagina{
	int PID;
	int nroPagina;
}typedef leer_pagina;

struct finPrograma{
	int PID;
}typedef fin_programa;

typedef struct {
	int socketServer;
	int socketClient;
} t_serverData;

int puertoEscucha;
int tamanioDePagina;
int cantidadDePaginas;
int retardoCompactacion;
int retardoAcceso;
char* nombre_swap;
char * nombreSwapFull;
t_list* listaSwap;
t_log* logSwap;
void* mapearArchivoEnMemoria(int offset,int tamanio);
void destructorBloqueSwap(bloqueSwap* self);
int agregarProceso(bloqueSwap* unBloque,char* codeScript);
int compactarArchivo();
int condicionDeCompactacion(bloqueSwap* unBloque,bloqueSwap* otroBloque);
void crearArchivoDeSwap();
void handShake (void *parameter);
void newClients (void *parameter);
void startServer();
void escribirPagina(char* paginaAEscribir,bloqueSwap* unBloque);
int verificarEspacioDisponible();
char* leerPagina(bloqueSwap* bloqueDeSwap);
void crearArchivoDeConfiguracion();
void processingMessages(int socketClient);
bloqueSwap* existeElBloqueNecesitado(bloqueSwap* otroBloque);
bool condicionLeer(bloqueSwap* unBloque,bloqueSwap* otroBloque);
bool elementosVacios(bloqueSwap* unElemento);
bloqueSwap* buscarBloqueALlenar(bloqueSwap* unBloque);
int eliminarProceso(int PID);
bloqueSwap* buscarProcesoAEliminar(int PID);
int modificarArchivo(int marcoInicial,int cantDeMarcos,int nuevoMarcoInicial);
void escribirPagina(char* paginaAEscribir,bloqueSwap* unBloque);



#endif /* SWAP_H_ */
