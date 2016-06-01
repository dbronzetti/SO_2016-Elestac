
#include <stdio.h>
#include <stdlib.h>
#include "commons/collections/list.h"
#include <stdbool.h>
#include "commons/string.h"
#include <string.h>
#include <sys/mman.h>

struct bloqueDeMemoria{
	int PID;
	int cantDePaginas;
	int ocupado;
	int tamanioDelBloque;
	int paginaInicial;

}typedef bloqueSwap;

///////////////////////configurable trucho//////////////////////////////////
int puertoEscucha;
int tamanioDePagina;
int cantidadDePaginas;
int retardoCompactacion;
char nombre_swap[50];
t_list* listaSwap;

int main(){
listaSwap=list_create();
bloqueSwap* bloqueInicial;
bloqueInicial->PID=0;
bloqueInicial->ocupado=0;
bloqueInicial->tamanioDelBloque=cantidadDePaginas*tamanioDePagina;


list_add(listaSwap,(void*)bloqueInicial);

return 1;

}

int agregarProceso(bloqueSwap* unBloque,t_list* unaLista){
	/*<----------------------------------------------abroElArchivo----------------------------------------------------->*/
	FILE *archivoSwap;
	int descriptorSwap;
	void* archivoMapeado;
	archivoSwap=fopen(nombre_swap,"a+");
	descriptorSwap=fileno(archivoSwap);
	lseek(descriptorSwap,0,SEEK_SET);
	archivoMapeado=mmap(0,(tamanioDePagina*cantidadDePaginas),PROT_WRITE,MAP_SHARED,(void*)descriptorSwap,(tamanioDePagina*unBloque->paginaInicial));
	bloqueSwap* elementoEncontrado;
	elementoEncontrado=(bloqueSwap*)malloc(sizeof(bloqueSwap));
	bloqueSwap* nuevoBloqueVacio;
	nuevoBloqueVacio=(bloqueSwap*)malloc(sizeof(bloqueSwap));
	void destructorBloqueSwap(bloqueSwap* self){
		free(self->PID);
		free(self->cantDePaginas);
		free(self->ocupado);
		free(self->paginaInicial);
		free(self->tamanioDelBloque);
		free(self);
	}

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

void destructorBloqueSwap(bloqueSwap* self){
	free(self->PID);
	free(self->cantDePaginas);
	free(self->ocupado);
	free(self->paginaInicial);
	free(self->tamanioDelBloque);
	free(self);
}

int compactarArchivo(t_list* unaLista){
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
		acum+=bloqueLlenoSiguiente->cantDePaginas;
	}
		if(cantidadDePaginas-acum!=0){
		bloqueVacioCompacto->cantDePaginas=cantidadDePaginas-acum;
		bloqueVacioCompacto->ocupado=0;
		bloqueVacioCompacto->PID=0;
		bloqueVacioCompacto->paginaInicial=acum+1;

		list_add(unaLista,bloqueVacioCompacto);
		return 0;
	}
}}

int eliminarProceso(t_list* unaLista,bloqueSwap unProceso){
	bloqueSwap* procesoAEliminar=malloc(sizeof(bloqueSwap));
	bool buscarPorPid(bloqueSwap unProceso,bloqueSwap otroProceso){
		return (unProceso.PID==otroProceso.PID);
	}
	list_remove_and_destroy_by_condition(unaLista,(void*)buscarPorPid,(void*)destructorBloqueSwap);
	return 1;
}

void crearArchivoDeSwap(){
	char primerParteCadena[50]="dd if=dev/zero of=";
	char* tamanioPagina;
	tamanioPagina=string_itoa(tamanioDePagina);
	char* cadCantidadDePaginas;
	cadCantidadDePaginas=string_itoa(cantidadDePaginas);
	char segundaParteCadena[5]=" bs=";
	char terceraParteCadena[10]=" count=";
	char* cadenaTotal;
	cadenaTotal=strcat(primerParteCadena,nombre_swap);
	cadenaTotal=strcat(cadenaTotal,segundaParteCadena);
	cadenaTotal=strcat(cadenaTotal,tamanioPagina);
	cadenaTotal=strcat(cadenaTotal,terceraParteCadena);
	cadenaTotal=strcat(cadenaTotal,cadCantidadDePaginas);
	system(cadenaTotal[50]);
	}
int condicionLeer(bloqueSwap* unBloque,bloqueSwap* otroBloque){
			return (unBloque->PID==otroBloque->PID);
			}
void leerPagina(bloqueSwap* bloqueDeSwap,t_list* listaSwap){


	bloqueSwap* bloqueEncontrado;
	FILE* archivoSwap;
	int descriptorSwap;
	char* archivoMapeado;
	char* paginaAEnviar;
	archivoSwap=fopen(nombre_swap,"a+");
	descriptorSwap=fileno(archivoSwap);
	lseek(descriptorSwap,0,SEEK_SET);
	bloqueEncontrado=list_find(listaSwap,(void*)condicionLeer);
	archivoMapeado=mmap(0,tamanioDePagina*cantidadDePaginas,PROT_WRITE,MAP_SHARED,descriptorSwap,tamanioDePagina*bloqueDeSwap->paginaInicial);
	memcpy(paginaAEnviar,archivoMapeado,tamanioDePagina);
	//enviar(paginaAEnviar);
	}

void escribirPagina(char* paginaAEscribir,bloqueSwap* unBloque,t_list* listaSwap){
	int descriptorSwap;
	char* archivoMapeado;
	FILE* archivoSwap;
	archivoSwap=fopen(nombre_swap,"a+");
	descriptorSwap=fileno(archivoSwap);
	lseek(descriptorSwap,0,SEEK_SET);

	archivoMapeado=mmap(0,tamanioDePagina*cantidadDePaginas,PROT_WRITE,MAP_SHARED,descriptorSwap,tamanioDePagina*unBloque->paginaInicial);
	memset(archivoMapeado+unBloque->cantDePaginas,paginaAEscribir,tamanioDePagina);
}









