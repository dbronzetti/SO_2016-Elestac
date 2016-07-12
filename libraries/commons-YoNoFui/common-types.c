#include "common-types.h"

/******************* PRIMITIVAS *******************/

/*
 * Listado
	definirVariable
	obtenerPosicionVariable
	dereferenciar
	asignar
	obtenerValorCompartida
	asignarValorCompartida
	irAlLabel
	llamarConRetorno
	retornar
	imprimir
	imprimirTexto
	entradaSalida
	wait
	signal
 *
 */

int tamanioDePagina = -1; //TODO ver como se lo paso desde la UMC
t_PCB* PCB;

void setPageSize (int pageSize){
	tamanioDePagina = pageSize;
}

int getLogicalAddress (int page){
	return (page * tamanioDePagina);
}

void destroyRegistroStack(t_registroStack* self){
	free(self->args); //TODO NO SE LIBERA ASI, es una LISTA! HACER list destroy con destroy element correspondiente
	free(self->vars);//TODO NO SE LIBERA ASI, es una LISTA! HACER list destroy con destroy element correspondiente
	free(self->retVar);
}
