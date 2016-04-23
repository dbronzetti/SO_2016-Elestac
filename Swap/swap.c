
#include <stdio.h>
#include <stdlib.h>
#include "commons/collections/list.h"
#include <stdbool.h>

struct bloqueDeMemoria{
	int PID;
	int cantDePaginas;
	int ocupado;
	int tamanioDelBloque;
	int paginaInicial;

}typedef bloqueSwap;

///////////////////////configurable trucho//////////////////////////////////
int puertoEscucha;
int nombreSwap;
int tamanioDePagina;
int cantidadDePaginas;
int retardoCompactacion;
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
	list_remove_and_destroy_by_condition(unaLista,(void*)posibleBloqueAEliminar,(void*)destructorBloqueSwap);
	list_add(unaLista,(void*)nuevoBloqueVacio);
	list_add(unaLista,(void*)unBloque);
	return 0;
}

bool compactarArchivo(t_list* unaLista){
	int i,acum;
	bloqueSwap bloqueLleno;
	bloqueSwap bloqueLlenoSiguiente;
	bloqueSwap bloqueVacioCompacto;

	int bloqueVacioAEliminar(bloqueSwap unBloque){
		return(unBloque.ocupado==0);
	};
	void destructorBloqueSwap(bloqueSwap* self){
		free(self->PID);
		free(self->cantDePaginas);
		free(self->ocupado);
		free(self->paginaInicial);
		free(self->tamanioDelBloque);
		free(self);
	}


	for(i=0;unaLista->elements_count<=i;i++){
		{	list_remove_and_destroy_by_condition(unaLista,bloqueVacioAEliminar,destructorBloqueSwap);
		}
		bloqueLleno=list_get(unaLista,0);
		bloqueLleno.paginaInicial=0;
		acum=bloqueLleno.cantDePaginas;
	for(i=0;unaLista->elements_count>i;i++){
		bloqueLleno=list_get(unaLista,i);
		bloqueLlenoSiguiente=list_get(unaLista,i+1);
		bloqueLlenoSiguiente.paginaInicial=bloqueLleno.cantDePaginas+1;
		acum+=bloqueLlenoSiguiente.cantDePaginas;
	}
		if(cantidadDePaginas-acum!=0){
		bloqueVacioCompacto.cantDePaginas=cantidadDePaginas-acum;
		bloqueVacioCompacto.ocupado=0;
		bloqueVacioCompacto.PID=0;
		bloqueVacioCompacto.paginaInicial=acum+1;

		list_add(unaLista,bloqueVacioCompacto);}
		return 0;
	}
}


