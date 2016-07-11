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
int socketUMC;
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


void serializarES(t_es *dispositivoEnviar, char *dispositivoSerializado){

	memcpy(dispositivoSerializado,dispositivoEnviar->dispositivo , (strlen(dispositivoEnviar->dispositivo)));
	int offset = strlen(dispositivoEnviar->dispositivo);

	memcpy(dispositivoSerializado + offset,(void*) dispositivoEnviar->tiempo, sizeof(int));
	offset += sizeof(int);

	memcpy(dispositivoSerializado + offset,(void*) dispositivoEnviar->ProgramCounter, sizeof(int));

}


void deserializarES(t_es* datos, char* buffer) {

	memcpy(&datos->dispositivo,buffer, (strlen(datos->dispositivo)));
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






void serializarPCB(t_MessageNucleo_CPU* value, char* buffer, int valueSize){
	int offset = 0;
	enum_processes process = NUCLEO;

		//0)valueSize
		memcpy(buffer, &valueSize, sizeof(valueSize));
		offset += sizeof(valueSize);

		//1)From process
		memcpy(buffer + offset, &process, sizeof(process));
		offset += sizeof(process);

		//2)processID
		memcpy(buffer + offset, &value->processID, sizeof(value->processID));
		offset += sizeof(value->processID);

		//3)operacion
		memcpy(buffer + offset, &value->operacion, sizeof(value->operacion));
		offset += sizeof(value->operacion);

		//4)ProgramCounter
		memcpy(buffer + offset, &value->programCounter, sizeof(value->programCounter));
		offset += sizeof(value->programCounter);

	    //5)cantidadDePaginas de codigo
	    memcpy(buffer + offset, &value->cantidadDePaginas, sizeof(value->cantidadDePaginas));
		offset += sizeof(value->cantidadDePaginas);

		//6)StackPointer
		memcpy(buffer + offset, &value->stackPointer, sizeof(value->stackPointer));
		offset += sizeof(value->stackPointer);

		//7)quantum_sleep
		memcpy(buffer + offset, &value->quantum_sleep, sizeof(value->quantum_sleep));
		offset += sizeof(value->quantum_sleep);

		//8)quantum
		memcpy(buffer + offset, &value->quantum, sizeof(value->quantum));
		offset += sizeof(value->quantum);

	    //9)indiceDeEtiquetasTamanio
	    value->indiceDeEtiquetasTamanio = strlen(value->indiceDeEtiquetas) + 1;//+1 because of '\0'
		memcpy(buffer + offset, &value->indiceDeEtiquetasTamanio, sizeof(value->indiceDeEtiquetasTamanio));
		offset += sizeof(value->indiceDeEtiquetasTamanio);

		//10)indiceDeEtiquetas
	    memcpy(buffer + offset, value->indiceDeEtiquetas, value->indiceDeEtiquetasTamanio);
		offset += sizeof(value->indiceDeEtiquetasTamanio);

	    serializarListaIndiceDeCodigo(value->indiceDeCodigo,buffer + offset);
		offset += sizeof(t_registroIndiceCodigo)*value->indiceDeCodigo->elements_count;

		serializarListaStack(value->indiceDeStack, buffer + offset);

}

void serializarRegistroStack(t_registroStack* registroASerializar, char* registroSerializado){
	int offset =0 ;
	serializarStack(registroASerializar, registroSerializado + offset);
	offset =+ sizeof(int)*2 + sizeof(t_memoryLocation);

	serializeListaArgs(registroASerializar->args,registroSerializado + offset);
	offset =+ sizeof(t_memoryLocation)* registroASerializar->args->elements_count;

	serializarListasVars(registroASerializar->vars, registroSerializado + offset);
	offset =+ sizeof(t_vars)* registroASerializar->vars->elements_count;

}

void serializarListaStack(t_list* listaASerializar, char* listaSerializada){
	int offset = 0;
	memcpy(listaSerializada, &listaASerializar->elements_count, sizeof(listaASerializar->elements_count));
	int i;
	t_registroStack* registroStack = malloc(sizeof(t_registroStack));
	for(i = 0 ; i<listaASerializar->elements_count;i++){

		registroStack = list_get(listaASerializar,i);
		serializarRegistroStack(registroStack,listaSerializada + offset);
		offset =+ sizeof(t_registroStack);

	}


}

void serializarVars(t_vars* miRegistro, char* value) {
	int offset = 0;

	memcpy(value + offset,&miRegistro->identificador,sizeof(miRegistro->identificador));
	offset=sizeof(miRegistro->identificador);
	serializeMemoryLocation(miRegistro->direccionValorDeVariable, value+offset);

}

void serializeMemoryLocation(t_memoryLocation* unaPosicion, char* value) {
	int offset = 0;

	memcpy(value + offset, &unaPosicion->offset, sizeof(int));
	offset = +sizeof(unaPosicion->offset);
	memcpy(value + offset, &unaPosicion->pag, sizeof(int));
	offset = +sizeof(unaPosicion->pag);
	memcpy(value + offset, &unaPosicion->size, sizeof(int));
	offset = +sizeof(unaPosicion->size);

}

void serializarListasVars(t_list* listaASerializar, char* listaSerializada) {
	int offset = 0;
	t_vars* unaVariable = malloc(sizeof(t_vars));
	memcpy(listaSerializada + offset, &listaASerializar->elements_count,
			sizeof(listaASerializar->elements_count));
	offset = +sizeof(listaASerializar->elements_count);
	int i;
	for (i = 0; i < listaASerializar->elements_count; i++) {
		unaVariable = list_get(listaASerializar, i);
		serializarVars(unaVariable, listaSerializada + offset);
	}
}

void serializeListaArgs(t_list* listaASerializar, char* listaSerializada) {
	int offset = 0;
	t_memoryLocation* unaPosicion = malloc(sizeof(t_memoryLocation));
	memcpy(listaSerializada, &listaASerializar->elements_count,
			sizeof(listaASerializar->elements_count));
	offset = +sizeof(listaASerializar->elements_count);
	int i;
	for (i = 0; i < listaASerializar->elements_count; i++) {
		unaPosicion = list_get(listaASerializar, i);
		serializeMemoryLocation(unaPosicion, listaSerializada + offset);
	}
}

void serializarStack(t_registroStack* registroStack, char* registroSerializado) {
	int offset = 0;
	memcpy(registroSerializado + offset, &registroStack->pos, sizeof(int));
	memcpy(registroSerializado + offset, &registroStack->retPos, sizeof(int));
	serializeMemoryLocation(registroStack->retVar,
			registroSerializado + offset);

}

void deserializarRegistroStack(t_registroStack* registroRecibido, char* registroSerializado){
	int offset = 0 ;

	deserializarStack(registroRecibido, registroSerializado);
	offset =+ sizeof(registroRecibido->retVar) + sizeof(registroRecibido->retPos) + sizeof(registroRecibido->pos);

	int offsetListaArgs = deserializarListasArgs(registroRecibido->args, registroSerializado + offset);
	offset =+ offsetListaArgs - sizeof(registroRecibido->args->elements_count);

	int offsetListaVars = deserializarListasVars(registroRecibido->vars, registroSerializado + offset);
	offset =+ offsetListaVars - sizeof(registroRecibido->vars->elements_count);

}

void deserializarStack(t_registroStack* estructuraARecibir, char* registroStack) {
	int offset = 0;
	memcpy(&estructuraARecibir->pos, registroStack, sizeof(int));
	offset = +sizeof(estructuraARecibir->pos);
	memcpy(&estructuraARecibir->retPos, registroStack + offset, sizeof(int));
	offset = +sizeof(estructuraARecibir->retPos);
	deserializarMemoryLocation(estructuraARecibir->retVar,registroStack + offset);

}

void deserializarListaStack(t_list* listaARecibir, char* listaSerializada){
	int offset = 0;
	int cantidadDeElementos;
	memcpy(cantidadDeElementos, listaSerializada + offset, sizeof(listaARecibir->elements_count));
	offset=+sizeof(int);
	int i;
	t_registroStack* registroStack = malloc(sizeof(t_registroStack));
	for(i=0; i<cantidadDeElementos;i++){
		deserializarStack(registroStack,listaSerializada + offset);
		list_add(listaARecibir,registroStack);
		offset =+ sizeof(t_registroStack);

	}

}

void deserializarMemoryLocation(t_memoryLocation* unaPosicion, char* posicionRecibida) {
	int offset = 0;
	memcpy(&unaPosicion->offset, posicionRecibida + offset, sizeof(int));
	offset = +sizeof(unaPosicion->offset);
	memcpy(&unaPosicion->pag, posicionRecibida + offset, sizeof(int));
	offset = +sizeof(unaPosicion->pag);
	memcpy(&unaPosicion->size, posicionRecibida + offset, sizeof(int));
}

void deserializarVars(t_vars* unaVariable, char* variablesRecibidas) {
	int offset = 0;

	memcpy(&unaVariable->identificador,variablesRecibidas+offset,sizeof(unaVariable->identificador));
	offset=+sizeof(unaVariable->identificador);
	deserializarMemoryLocation(unaVariable->direccionValorDeVariable,variablesRecibidas+offset);

}

int deserializarListasArgs(t_list* listaArgs,char* listaSerializada){
	int offset=0;
	int cantidadDeElementos;
	memcpy(&cantidadDeElementos,listaSerializada+offset,sizeof(int));
	offset=+sizeof(int);
	int i;
	for(i=0;i<cantidadDeElementos;i++){
		t_memoryLocation* posicionDeMemoria=malloc(sizeof(t_memoryLocation));
		deserializarMemoryLocation(posicionDeMemoria,listaSerializada+offset);
		offset=+sizeof(t_memoryLocation);
		list_add(listaArgs,posicionDeMemoria);
	}
	return offset;
}

int deserializarListasVars(t_list* listaVars,char* listaSerializada){
	int offset=0;
	int cantidadDeElementos;
	memcpy(&cantidadDeElementos,listaSerializada+offset,sizeof(int));
	offset=+sizeof(int);
	int i;
	for(i=0;i<cantidadDeElementos;i++){
		t_vars* unaVariable=malloc(sizeof(t_memoryLocation));
		deserializarVars(unaVariable,listaSerializada+offset);
		offset=+sizeof(t_vars);
		list_add(listaVars,unaVariable);
	}
	return offset;
}


void serializarIndiceDeCodigo(t_registroIndiceCodigo* registroEnviar, char* registroSerializado){
	int offset = 0 ;

	memcpy(registroSerializado + offset, &registroEnviar->inicioDeInstruccion,sizeof(registroEnviar->inicioDeInstruccion));
	offset =+ sizeof(registroEnviar->inicioDeInstruccion);

	memcpy(registroSerializado + offset, &registroEnviar->longitudInstruccionEnBytes,sizeof(registroEnviar->longitudInstruccionEnBytes));
	offset =+ sizeof(registroEnviar->longitudInstruccionEnBytes);

}

void serializarListaIndiceDeCodigo(t_list* listaASerializar, char* listaSerializada){
	int offset = 0 ;
	t_registroIndiceCodigo* registroObtenido = malloc(sizeof(t_registroIndiceCodigo));
	memcpy(listaSerializada, &listaASerializar->elements_count,
			sizeof(listaASerializar->elements_count));
	offset = +sizeof(listaASerializar->elements_count);
	int i;
	for (i = 0; i < listaASerializar->elements_count; i++) {
		registroObtenido = list_get(listaASerializar,i);
		serializarIndiceDeCodigo(registroObtenido, listaSerializada + offset);
		offset =+ sizeof(t_registroIndiceCodigo);


		}

}

void deserializarIndiceDeCodigo(t_registroIndiceCodigo* registroARecibir, char* registroSerializado){
	int offset = 0;

	memcpy(&registroARecibir->inicioDeInstruccion,registroSerializado + offset,sizeof(registroARecibir->inicioDeInstruccion));
	offset=+sizeof(registroARecibir->inicioDeInstruccion);

	memcpy(&registroARecibir->longitudInstruccionEnBytes,registroSerializado + offset,sizeof(registroARecibir->longitudInstruccionEnBytes));
	offset=+sizeof(registroARecibir->longitudInstruccionEnBytes);

}

void deserializarListaIndiceDeCodigo(t_list* listaARecibir, char* listaSerializada){
	int offset = 0;
	int cantidadDeElementos = 0;
	t_registroIndiceCodigo* registroIndiceDeCodigo;
	memcpy(&cantidadDeElementos,listaSerializada+offset,sizeof(int));
	offset=+sizeof(int);
	int i;
	for(i=0;i<cantidadDeElementos;i++){
		deserializarIndiceDeCodigo(registroIndiceDeCodigo, listaSerializada + offset);
		list_add(listaARecibir, registroIndiceDeCodigo);
		offset=+sizeof(t_registroIndiceCodigo);

	}
}

void serializarIndiceDeEtiquetas(t_registroIndiceEtiqueta* registroAEnviar,char* registroSerializado){
	int offset=0;
	memcpy(registroSerializado+offset,strlen(registroAEnviar->funcion),size(strlen(registroAEnviar->funcion)));
	offset=+sizeof(strlen(registroAEnviar->funcion));
	memcpy(registroSerializado+offset,registroAEnviar->funcion,strlen(registroAEnviar->funcion));
	offset=+strlen(registroAEnviar->funcion);
	memcpy(registroSerializado,registroAEnviar->posicionDeLaEtiqueta,sizeof(registroAEnviar->posicionDeLaEtiqueta));
}

void serializarListaIndiceDeEtiquetas(t_list* listaASerializar,char* listaSerializada){
	int offset=0;
	memcpy(listaSerializada,&listaASerializar->elements_count,sizeof(listaASerializar->elements_count));
	offset=+sizeof(listaASerializar->elements_count);
	int i;
	t_registroIndiceEtiqueta* registroObtenido;
	for(i=0;i<listaASerializar->elements_count;i++){
		registroObtenido=list_get(listaASerializar,i);
		serializarIndiceDeEtiquetas(registroObtenido,listaSerializada+offset);
		offset=+(sizeof(t_registroIndiceEtiqueta)+sizeof(int));
	}
}

void DeserializarIndiceDeEtiquetas(t_registroIndiceEtiqueta* registroRecibido,char* registroSerializado){
	int offset=0;
	int tamanioDeLaCadena;
		memcpy(tamanioDeLaCadena,registroSerializado+offset,sizeof(int));
		offset=+sizeof(strlen(registroRecibido->funcion));
		memcpy(registroRecibido->funcion,registroSerializado+offset,tamanioDeLaCadena);
		offset=+strlen(registroRecibido->funcion);
		memcpy(registroRecibido->posicionDeLaEtiqueta,registroSerializado+ offset,sizeof(registroRecibido->posicionDeLaEtiqueta));

}

void DeserializarListaIndiceDeEtiquetas(t_list* listaRecibida,char* listaSerializada){
	int	offset=0;
	int cantidadDeElementos=0;
	memcpy(cantidadDeElementos,listaSerializada,sizeof(int));
	offset=+sizeof(int);
	int i;
	t_registroIndiceEtiqueta* registroObtenido=malloc(sizeof(t_registroIndiceEtiqueta));
	for(i=0;i<cantidadDeElementos;i++){
		DeserializarIndiceDeEtiquetas(registroObtenido,listaSerializada+offset);
		list_add(listaRecibida,registroObtenido);
		offset=+sizeof(t_registroIndiceEtiqueta)+sizeof(int);
	}
}








