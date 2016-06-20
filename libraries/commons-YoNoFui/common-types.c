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

t_puntero definirVariable(t_nombre_variable nombreVariable){
	t_puntero varPosition;
	t_registroStack* registroBuscado=malloc(sizeof(t_registroStack));

	registroBuscado=list_get(PCB->indiceDeStack,PCB->indiceDeStack->elements_count);
	list_add_in_index(registroBuscado->vars,(registroBuscado->vars->elements_count+1),(void*)nombreVariable);
	varPosition=registroBuscado->vars->elements_count+1;
	return varPosition;
	//varPosition = buscarVariable();
}




t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	bool condicionIgualdad(t_nombre_variable OtroNombreVariable){
			return(identificador_variable==OtroNombreVariable);
		}
	bool condicionVariable(t_registroStack unRegistro){
			return(list_any_satisfy(unRegistro.vars,(void*)condicionIgualdad));
		}
	t_registroStack* registroBuscado=malloc(sizeof(t_registroStack));
	registroBuscado=(t_registroStack*)list_find(PCB->indiceDeStack,(void*)condicionVariable);
	return(registroBuscado->pos);


}



/*int buscarOffsetVariable(t_vars identificador_variable){
	t_registroStack registroBuscado;
	t_vars variableBuscada;
	registroBuscado=list_get(PCB->indiceDeStack,1);
	variableBuscada=list_find(registroBuscado.vars,(void*)condicionBuscarVariable);

	return variableBuscada.direccionValorDeVariable.offset;
}*/

t_valor_variable dereferenciar(t_puntero direccion_variable){
	t_valor_variable varValue;
	//char *varValue = malloc(direccion_variable.size);

	//memcpy(varValue, getLogicalAddress(direccion_variable.pag) + direccion_variable.offset, direccion_variable.size);

	//free(varValue);

	return varValue;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){

	/*
	memcpy(getLogicalAddress(direccion_variable.pag) + direccion_variable.offset, valueChar, direccion_variable.size);

	free(valueChar);
	*/
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	t_valor_variable varValue;
	/*t_memoryLocation *varValue = NULL;
	varValue->offset = variable.direccionValorDeVariable.offset;
	varValue->pag = variable.direccionValorDeVariable.pag;
	varValue->size = variable.direccionValorDeVariable.size;
	//TODO return var value from Nucleo*/
	return varValue;
}

t_valor_variable asignarValorCompartida(t_vars variable, t_valor_variable valor){
	t_valor_variable value;
	/*
	char *valueChar = malloc(valor.size);

	memcpy(valueChar, getLogicalAddress(valor.pag) + valor.offset, valor.size);

	//TODO return var value copied to Nucleo
	*/
	return value;

}

void irAlLabel(t_registroIndiceEtiqueta etiqueta){

	PCB->ProgramCounter=etiqueta.posicionDeLaEtiqueta;
	//list_add(registroAActualizar.args); "Ver como agregar los argumentos"


	//TODO change execution line to the etiqueta given

}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	t_valor_variable retorno;
	irAEtiqueta(etiqueta);
	retorno=dereferenciar(donde_retornar);
	retornar(retorno);

}

void retornar(t_valor_variable retorno){

	t_registroStack* registroARegresar;
	bool condicionRetorno(t_registroStack unRegistro){
		return (unRegistro.retPos==registroARegresar->pos);
	}
	registroARegresar=(t_registroStack*)list_find(PCB->indiceDeStack,(void*)condicionRetorno);
	PCB->ProgramCounter=registroARegresar->retPos;
	PCB->StackPointer=registroARegresar->pos;

	//TODO see functionality

}


void imprimir(t_valor_variable valor_mostrar){
	/*char *valueChar = malloc(valor_mostrar.size);

	memcpy(valueChar, getLogicalAddress(valor_mostrar.pag) + valor_mostrar.offset, valor_mostrar.size);

	//TODO send to Nucleo valueChar to be printed on Consola
	*/
}

void imprimirTexto(char *texto){

	//TODO send to Nucleo "texto" to be printed on Consola

}

int entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){

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
