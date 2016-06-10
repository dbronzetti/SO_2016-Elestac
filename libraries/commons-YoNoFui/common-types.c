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
	free(self->args);
	free(self->pos);
	free(self->retPos);
	free(self->retVar.offset);
	free(self->retVar.pag);
	free(self->retVar.size);
	free(self->vars);
}

int definirVariable(t_vars nombreVariable){
	int varPosition;
	t_registroStack* registroBuscado;
	registroBuscado=list_get(PCB->indiceDeStack,1);
	list_add(registroBuscado->vars,nombreVariable);
	varPosition=registroBuscado->vars->elements_count;
	list_replace_and_destroy_element(PCB->indiceDeStack,1,registroBuscado,(void*)destroyRegistroStack);
	return varPosition;
	//varPosition = buscarVariable();

	return varPosition;
}



int obtenerPosicionVariable(t_vars identificador_variable){
	int varOffset = -1 ;// ERROR value by DEFAULT -1

	//TODO return var offset in Stack
	varOffset = buscarOffsetVariable(identificador_variable);

	return varOffset;
}

bool condicionBuscarVarible(t_vars* variableBuscada,t_vars* otraVariable){
	return variableBuscada->identificador==otraVariable->identificador;
}

int buscarOffsetVariable(t_vars identificador_variable){
	t_registroStack registroBuscado;
	t_vars variableBuscada;
	registroBuscado=list_get(PCB->indiceDeStack,1);
	variableBuscada=list_find(registroBuscado.vars,(void*)condicionBuscarVariable);

	return variableBuscada.direccionValorDeVariable.offset;
}

void *dereferenciar(t_memoryLocation direccion_variable){
	char *varValue = malloc(direccion_variable.size);

	memcpy(varValue, getLogicalAddress(direccion_variable.pag) + direccion_variable.offset, direccion_variable.size);

	free(varValue);

	return (void*) varValue;
}

void asignar(t_memoryLocation direccion_variable, t_memoryLocation valor){
	char *valueChar = malloc(valor.size);

	memcpy(getLogicalAddress(direccion_variable.pag) + direccion_variable.offset, valueChar, direccion_variable.size);

	free(valueChar);
}

t_memoryLocation *obtenerValorCompartida(t_vars variable){
	t_memoryLocation *varValue = NULL;
	varValue->offset = variable.direccionValorDeVariable.offset;
	varValue->pag = variable.direccionValorDeVariable.pag;
	varValue->size = variable.direccionValorDeVariable.size;
	//TODO return var value from Nucleo
	return varValue;
}

t_memoryLocation asignarValorCompartida(t_vars variable, t_memoryLocation valor){
	char *valueChar = malloc(valor.size);

	memcpy(valueChar, getLogicalAddress(valor.pag) + valor.offset, valor.size);

	//TODO return var value copied to Nucleo

	return valor;
}

void irAlLabel(t_registroIndiceEtiqueta etiqueta){

	PCB->ProgramCounter=etiqueta.posicionDeLaEtiqueta;
	//list_add(registroAActualizar.args); "Ver como agregar los argumentos"


	//TODO change execution line to the etiqueta given

}

void llamarConRetorno(t_registroIndiceEtiqueta etiqueta, t_registroStack donde_retornar){
	t_registroStack registroAActualizar;
	registroAActualizar=list_get(PCB->indiceDeStack,1);
	registroAActualizar.retPos=donde_retornar;
	registroAActualizar.retVar=list_get(registroAActualizar.vars,1);
	registroAActualizar.pos+=1;
	list_add(PCB->indiceDeStack,registroAActualizar);

}

void retornar(t_memoryLocation *retorno){
	t_registroStack registroARegresar;
	registroARegresar=list_find(PCB->indiceDeStack,condicionRetorno);
	PCB->ProgramCounter=registroARegresar.retPos;
	PCB->StackPointer=registroARegresar.pos;

	//TODO see functionality

}
bool condicionRetorno(t_registroStack unRegistro, t_registroStack otroRegistro){
	return (unRegistro.pos==otroRegistro.retPos);
}

int imprimir(t_memoryLocation valor_mostrar){
	char *valueChar = malloc(valor_mostrar.size);

	memcpy(valueChar, getLogicalAddress(valor_mostrar.pag) + valor_mostrar.offset, valor_mostrar.size);

	//TODO send to Nucleo valueChar to be printed on Consola

	return strlen(valueChar);
}

int imprimirTexto(char *texto){

	//TODO send to Nucleo "texto" to be printed on Consola

	return strlen(texto);
}

int entradaSalida(enum_dispositivos dispositivo, int tiempo){

	//TODO send to Nucleo "dispositivo" to be used by n units of "tiempo"
	// definir enum_dispositivos

	return 0; //TODO ver que valor debe retornar
}

void wait(t_nombreSemaforo identificador_semaforo){

	//ver como implementar t_nombre_semaforo
	//TODO send to Nucleo to execute WAIT function for "identificador_semaforo"

}

void signal(t_nombreSemaforo identificador_semaforo){

	//ver como implementar t_nombre_semaforo
	//TODO send to Nucleo to execute SIGNAL function for "identificador_semaforo"

}
