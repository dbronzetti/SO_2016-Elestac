#define agregar_proceso 1
#define finalizar_proceso 2
#define lectura_pagina 3
#define escritura_pagina 4

#include <stdlib.h>
#include <stdbool.h>
#include "commons/string.h"
#include <string.h>
#include <sys/mman.h>
#include "commons/config.h"
#include "common-types.h"

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

/*<----------------------------------Prototipos----------------------------------------->*/
 void* mapearArchivoEnMemoria(int offset,int tamanio);
 void destructorBloqueSwap(bloqueSwap* self);
 int agregarProceso(bloqueSwap* unBloque,t_list* unaLista);
 int compactarArchivo(t_list* unaLista);
 int condicionDeCompactacion(bloqueSwap* unBloque,bloqueSwap* otroBloque);
 void crearArchivoDeSwap();
 void handShake (void *parameter);
 void newClients (void *parameter);
 void startServer();
 void escribirPagina(char* paginaAEscribir,bloqueSwap* unBloque,t_list* listaSwap);
 int verificarEspacioDisponible(t_list* listaSwap);
 char* leerPagina(bloqueSwap* bloqueDeSwap,t_list* listaSwap);
 void crearArchivoDeConfiguracion();
 void processingMessages(int socketClient);
 int existeElBloqueNecesitado(t_list* listaSwap);
 bool condicionLeer(bloqueSwap* unBloque,bloqueSwap* otroBloque);
 int eliminarProceso(t_list* unaLista,bloqueSwap* unProceso);
 int elementosVacios(bloqueSwap* unElemento);






/*>----------------------------------Prototipos-----------------------------------------<*/



///////////////////////configurable trucho//////////////////////////////////
int* puertoEscucha;
int tamanioDePagina;
int cantidadDePaginas;
int retardoCompactacion;
int retardoAcceso;
char* nombre_swap;
t_list* listaSwap;

int setup(){
crearArchivoDeConfiguracion();
crearArchivoDeSwap();
FILE* archivoSwap;
char* paginaAEnviar;
bloqueSwap* bloqueInicial;
bloqueInicial->PID=0;
bloqueInicial->ocupado=0;
bloqueInicial->tamanioDelBloque=cantidadDePaginas*tamanioDePagina;
list_add(listaSwap,(void*)bloqueInicial);
startServer();
return 1;
}

void processingMessages(int socketClient){
	char* mensajeRecibido;
	char* operacionRecibida;
	char* paginaRecibida;

	int operacionARealizar;
	receiveMessage(&socketClient,operacionRecibida,sizeof(int));
	//,deserializarOperacion(operacionARealizar,operacionRecibida);


	switch(operacionARealizar){
	case agregar_proceso:{
		bloqueSwap* pedidoRecibidoYDeserializado;
		nuevo_programa programaRecibido;

		receiveMessage(&socketClient,mensajeRecibido,sizeof(bloqueSwap));
		//deserializarBloqueSwap(programaRecibido,mensajeRecibido);
		pedidoRecibidoYDeserializado->PID=programaRecibido.PID;
		pedidoRecibidoYDeserializado->cantDePaginas=programaRecibido.cantidadDePaginas;
		if(verificarEspacioDisponible(listaSwap)>pedidoRecibidoYDeserializado->cantDePaginas){
			if(existeElBloqueNecesitado(listaSwap)){
				agregarProceso(pedidoRecibidoYDeserializado,listaSwap);
			}else{
				compactarArchivo(listaSwap);
				agregarProceso(pedidoRecibidoYDeserializado,listaSwap);
			}
		}else{
			printf("No hay espacio disponible para agregar el bloque");
		};
		break;
	}
	case finalizar_proceso:{
		fin_programa procesoAFinalizar;
		bloqueSwap* pedidoRecibidoYDeserializado;
		receiveMessage(&socketClient,mensajeRecibido,sizeof(bloqueSwap));
		deserializarBloqueSwap(procesoAFinalizar,mensajeRecibido);
		pedidoRecibidoYDeserializado->PID=procesoAFinalizar.PID;
		eliminarProceso(listaSwap,pedidoRecibidoYDeserializado);
		break;
	}

	case lectura_pagina:{
		bloqueSwap* pedidoRecibidoYDeserializado;
		leer_pagina lecturaNueva;
		char* paginaLeida;
		receiveMessage(&socketClient,mensajeRecibido,sizeof(bloqueSwap));
		deserializarBloqueSwap(lecturaNueva,mensajeRecibido);
		pedidoRecibidoYDeserializado->PID=lecturaNueva.PID;
		pedidoRecibidoYDeserializado->paginaInicial=lecturaNueva.nroPagina;
		paginaLeida=leerPagina(pedidoRecibidoYDeserializado,listaSwap);
		sendMessage(socketClient,paginaLeida,tamanioDePagina);
		break;
	}
	case escritura_pagina:{
		escribir_pagina escrituraNueva;
		bloqueSwap* pedidoRecibidoYDeserializado;
		receiveMessage(&socketClient,mensajeRecibido,sizeof(bloqueSwap));
		deserializarBloqueSwap(escrituraNueva,mensajeRecibido);
		pedidoRecibidoYDeserializado->PID=escrituraNueva.PID;
		pedidoRecibidoYDeserializado->paginaInicial=escrituraNueva.nroPagina;
		escribirPagina(escrituraNueva.contenido,pedidoRecibidoYDeserializado,listaSwap);
		break;
	}

	}
}

void destructorBloqueSwap(bloqueSwap* self){
	free(self->PID);
	free(self->cantDePaginas);
	free(self->ocupado);
	free(self->paginaInicial);
	free(self->tamanioDelBloque);
	free(self);
}

int agregarProceso(bloqueSwap* unBloque,t_list* unaLista){
	/*<----------------------------------------------abroElArchivo----------------------------------------------------->*/
	void* archivoMapeado;
	archivoMapeado=mapearArchivoEnMemoria(tamanioDePagina*unBloque->paginaInicial,cantidadDePaginas*tamanioDePagina);
	bloqueSwap* elementoEncontrado;
	elementoEncontrado=(bloqueSwap*)malloc(sizeof(bloqueSwap));
	bloqueSwap* nuevoBloqueVacio;
	nuevoBloqueVacio=(bloqueSwap*)malloc(sizeof(bloqueSwap));


	bool posibleBloqueParaLlenar(bloqueSwap* unBloque,bloqueSwap* posibleBloque,void* destructorBloqueSwap){
			return (unBloque->ocupado==0 && unBloque->cantDePaginas<=posibleBloque->cantDePaginas);
		}

	bool posibleBloqueAEliminar(bloqueSwap* unBloque,bloqueSwap posibleBloqueAEliminar){
		return unBloque->paginaInicial==posibleBloqueAEliminar.paginaInicial;
	}

	elementoEncontrado=list_find(unaLista,(void*)posibleBloqueParaLlenar);
	elementoEncontrado->tamanioDelBloque=elementoEncontrado->tamanioDelBloque-unBloque->tamanioDelBloque;
	nuevoBloqueVacio->PID=0;
	nuevoBloqueVacio->ocupado=0;
	nuevoBloqueVacio->tamanioDelBloque=elementoEncontrado->tamanioDelBloque-unBloque->tamanioDelBloque;
	memset(archivoMapeado,"1",tamanioDePagina*unBloque->cantDePaginas);
	list_remove_and_destroy_by_condition(unaLista,(void*)posibleBloqueAEliminar,(void*)destructorBloqueSwap);
	list_add(unaLista,(void*)nuevoBloqueVacio);
	list_add(unaLista,(void*)unBloque);
	munmap(archivoMapeado,(tamanioDePagina*cantidadDePaginas));
	return 0;
}



int compactarArchivo(t_list* unaLista){
	void* archivoMapeado;
	archivoMapeado=mapearArchivoEnMemoria(0,cantidadDePaginas*tamanioDePagina);
	int i,acum;
	bloqueSwap* bloqueLleno=malloc(sizeof(bloqueSwap));
	bloqueSwap* bloqueLlenoSiguiente=malloc(sizeof(bloqueSwap));
	bloqueSwap* bloqueVacioCompacto=malloc(sizeof(bloqueSwap));
	int bloqueVacioAEliminar(bloqueSwap unBloque){
		return (unBloque.ocupado==0);
		};
	for(i=0;unaLista->elements_count<=i;i++){
		{	list_remove_and_destroy_by_condition(unaLista,(void*)bloqueVacioAEliminar,(void*)destructorBloqueSwap);
		}
		bloqueLleno=(bloqueSwap*)list_get(unaLista,0);
		bloqueLleno->paginaInicial=0;
		acum=bloqueLleno->cantDePaginas;
	for(i=0;unaLista->elements_count>i;i++){
		bloqueLleno=(bloqueSwap*)list_get(unaLista,i);
		bloqueLlenoSiguiente=(bloqueSwap*)list_get(unaLista,i+1);
		bloqueLlenoSiguiente->paginaInicial=bloqueLleno->cantDePaginas+1;
		memcpy(archivoMapeado + (bloqueLleno->cantDePaginas*tamanioDePagina),archivoMapeado+(bloqueLlenoSiguiente->paginaInicial*tamanioDePagina),bloqueLlenoSiguiente->cantDePaginas*tamanioDePagina);
		acum+=bloqueLlenoSiguiente->cantDePaginas;
		}
		if(cantidadDePaginas-acum!=0){
		bloqueVacioCompacto->cantDePaginas=cantidadDePaginas-acum;
		bloqueVacioCompacto->ocupado=0;
		bloqueVacioCompacto->PID=0;
		bloqueVacioCompacto->paginaInicial=acum+1;
		list_add(unaLista,bloqueVacioCompacto);
		}
	}
	return 0;
}

int eliminarProceso(t_list* unaLista,bloqueSwap* unProceso){
	bloqueSwap* procesoAEliminar=malloc(sizeof(bloqueSwap));
	bool buscarPorPid(bloqueSwap unProceso,bloqueSwap otroProceso){
		return (unProceso.PID==otroProceso.PID);
	}
	list_remove_and_destroy_by_condition(unaLista,(void*)buscarPorPid,(void*)destructorBloqueSwap);
	return 1;
}

void crearArchivoDeSwap(){
	int tamanioDePagina;
			tamanioDePagina=256;
			int cantidadDePaginas;
			cantidadDePaginas=10;

			char* cadena=string_new();
			string_append(&cadena,"dd if=/dev/zero of=/home/utnso/Escritorio/");


			string_append(&cadena,nombre_swap);
			char* segundaParteCadena=string_new();
			string_append(&segundaParteCadena," bs=");
			string_append(&cadena,segundaParteCadena);
			string_append_with_format(&cadena,"%i",tamanioDePagina);
			char* terceraParteCadena=string_new();
			string_append(&terceraParteCadena," count=");
			string_append(&cadena,terceraParteCadena);
			string_append_with_format(&cadena,"%i",cantidadDePaginas);
			system(cadena);
	}
bool condicionLeer(bloqueSwap* unBloque,bloqueSwap* otroBloque){
			return (unBloque->PID==otroBloque->PID);
			}
char* leerPagina(bloqueSwap* bloqueDeSwap,t_list* listaSwap){
	bloqueSwap* bloqueEncontrado;
	void* archivoMapeado;
	char* paginaAEnviar=malloc(sizeof(tamanioDePagina));
	bloqueEncontrado=list_find(listaSwap,(void*)condicionLeer);
	archivoMapeado=mapearArchivoEnMemoria(tamanioDePagina*bloqueEncontrado->paginaInicial,tamanioDePagina*cantidadDePaginas);
	memcpy(paginaAEnviar,archivoMapeado+(tamanioDePagina*bloqueDeSwap->paginaInicial),tamanioDePagina);
	munmap(archivoMapeado,cantidadDePaginas*tamanioDePagina);
	return paginaAEnviar;
	}

void escribirPagina(char* paginaAEscribir,bloqueSwap* unBloque,t_list* listaSwap){
	int descriptorSwap;
	void* archivoMapeado;
	bloqueSwap* bloqueEncontrado;
	bloqueEncontrado=list_find(listaSwap,(void*)condicionLeer);
	archivoMapeado=mapearArchivoEnMemoria(tamanioDePagina*bloqueEncontrado->paginaInicial,tamanioDePagina*cantidadDePaginas);
	memset(archivoMapeado+((bloqueEncontrado->paginaInicial+unBloque->paginaInicial)*tamanioDePagina),paginaAEscribir,tamanioDePagina);
	munmap(archivoMapeado,cantidadDePaginas*tamanioDePagina);
}


void* mapearArchivoEnMemoria(int offset,int tamanio){
	FILE* archivoSwap;
	int descriptorSwap;
	void* archivoMapeado;
	archivoSwap=fopen(nombre_swap,"a+");
	descriptorSwap=fileno(archivoSwap);
	lseek(descriptorSwap,0,SEEK_SET);
	archivoMapeado=mmap(0,tamanio,PROT_WRITE,MAP_SHARED,descriptorSwap,offset);
	return archivoMapeado;

}

int elementosVacios(bloqueSwap* unElemento){
	return unElemento->ocupado==0;
}
int verificarEspacioDisponible(t_list* listaSwap){
	t_list* listaFiltrada;
	int i;
	int acum;
	bloqueSwap* bloqueDevuelto;
	listaFiltrada=list_filter(listaSwap,(void*)elementosVacios);
	for(i=0;i<listaFiltrada->elements_count;i++){
		bloqueDevuelto=list_get(listaFiltrada,i);
		acum=acum+(bloqueDevuelto->cantDePaginas);

	}
	return acum;
}

int condicionDeCompactacion(bloqueSwap* unBloque,bloqueSwap* otroBloque){
	return(unBloque->cantDePaginas>=otroBloque->cantDePaginas);
}

int existeElBloqueNecesitado(t_list* listaSwap){
	return list_any_satisfy(listaSwap,(void*)condicionDeCompactacion);
}

void crearArchivoDeConfiguracion(){
	t_config* configuracionDeSwap;
	configuracionDeSwap=config_create("/home/utnso/git/tp-2016-1c-YoNoFui/Swap/Config.swap");
	puertoEscucha=config_get_int_value(configuracionDeSwap,"PUERTO_ESCUCHA");
	nombre_swap=config_get_string_value(configuracionDeSwap,"NOMBRE_SWAP");
	retardoCompactacion=config_get_int_value(configuracionDeSwap,"RETARDO_COMPACTACION");
	retardoAcceso=config_get_int_value(configuracionDeSwap,"RETARDO_ACCESO");
	tamanioDePagina=config_get_int_value(configuracionDeSwap,"TAMANIO_PAGINA");
	cantidadDePaginas=config_get_int_value(configuracionDeSwap,"CANTIDAD_PAGINAS");
}

void startServer(){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	t_serverData serverData;

	exitCode = openServerConnection(&puertoEscucha, &serverData.socketServer);
	printf("socketServer: %d\n",serverData.socketServer);

	//If exitCode == 0 the server connection is opened and listening
	if (exitCode == 0) {
		puts ("The server is opened.");

		exitCode = listen(serverData.socketServer, SOMAXCONN);

		while (1){
			newClients((void*) &serverData);
		}
	}

}

void newClients (void *parameter){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	int pid;

	t_serverData *serverData = (t_serverData*) parameter;

	exitCode = acceptClientConnection(&serverData->socketServer, &serverData->socketClient);

	if (exitCode == EXIT_FAILURE){
		printf("There was detected an attempt of wrong connection\n");//TODO => Agregar logs con librerias
	}else{

		handShake(parameter);

	}// END handshakes

}


void handShake (void *parameter){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure

	t_serverData *serverData = (t_serverData*) parameter;

	//Receive message size
	int messageSize = 0;
	char *messageRcv = malloc(sizeof(messageSize));
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(messageSize));

	//Receive message using the size read before
	memcpy(&messageSize, messageRcv, sizeof(int));
	//printf("messageSize received: %d\n",messageSize);
	messageRcv = realloc(messageRcv,messageSize);
	receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

	printf("bytes received in message: %d\n",receivedBytes);

	//starting handshake with client connected
	t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
	deserializeHandShake(message, messageRcv);

	//Now it's checked that the client is not down
	if ( receivedBytes == 0 ){
		perror("The client went down while handshaking!"); //TODO => Agregar logs con librerias
		printf("Please check the client: %d is down!\n", serverData->socketClient);
	}else{
		switch ((int) message->process){
			case UMC:{
				printf("%s\n",message->message);

				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){

					//TODO start receiving request
					processingMessages(serverData->socketClient);

				}

				break;
			}
			default:{
				perror("Process not allowed to connect");//TODO => Agregar logs con librerias
				printf("Invalid process '%d' tried to connect to UMC\n",(int) message->process);
				close(serverData->socketClient);
				break;
			}
		}
	}

	free(messageRcv);
	free(message->message);
	free(message);
}
