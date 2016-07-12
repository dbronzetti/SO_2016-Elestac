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

void deserializarES(t_es* datos, char* buffer) {
	//TODO no se esta enviando el tamanio del dispositivo que es un char* ni se esta considerando el \0 en el offset
	memcpy(&datos->dispositivo,buffer, strlen(datos->dispositivo));
	int offset = strlen(datos->dispositivo);

	memcpy(&datos->tiempo, buffer+offset, sizeof(datos->tiempo));
	offset += sizeof(datos->tiempo);

	memcpy(&datos->ProgramCounter, buffer + offset, sizeof(datos->ProgramCounter));


	/* int dispositivoLen = 0;
	memcpy(&dispositivoLen, buffer + offset, sizeof(dispositivoLen));
	offset += sizeof(dispositivoLen);
	datos->dispositivo= malloc(dispositivoLen);
	memcpy(datos->dispositivo, buffer + offset, dispositivoLen);
	 */
}
