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
int socket;



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
	memcpy((void*)varValue,(void*)direccion_variable,sizeof(t_valor_variable));
	return varValue;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){
	memcpy((void*)direccion_variable,(void*)valor,sizeof(valor));
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	t_valor_variable valorVariableDeserializado;
	char* valorVariableSerializado;

	int valorDeError;

	valorDeError=sendMessage(socket,variable,sizeof(char));
	if(valorDeError!=-1){
		printf("Los datos se enviaron correctamente");
		if(receiveMessage(socket,valorVariableSerializado,sizeof(t_valor_variable))!=-1){

			memcpy(valorVariableDeserializado,valorVariableSerializado,sizeof(t_valor_variable));
		}
	}else{
		printf("Los datos no pudieron ser enviados");

	}
	return valorVariableDeserializado;


}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	struct_compartida variableCompuesta;
	char* struct_serializado=malloc(sizeof(struct_compartida));
	t_valor_variable value;
	variableCompuesta.nombreCompartida=variable;
	variableCompuesta.valorVariable=valor;
	//serializarStructCompartida(struct_serializado,variableCompuesta);
	sendMessage(socket,struct_serializado,sizeof(struct_serializado));

	return valor;

}

void irAlLabel(t_nombre_etiqueta etiqueta){
	t_registroIndiceEtiqueta* registroBuscado;
	int condicionEtiquetas(t_nombre_etiqueta unaEtiqueta,t_registroIndiceEtiqueta registroIndiceEtiqueta){
		return (registroIndiceEtiqueta.funcion==unaEtiqueta);
	}
	registroBuscado=(t_registroIndiceEtiqueta*)list_find(PCB->indiceDeEtiquetas,(void*)condicionEtiquetas);
	PCB->ProgramCounter=registroBuscado->posicionDeLaEtiqueta;

}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	t_valor_variable retorno;
	irAlLabel(etiqueta);
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


}


void imprimir(t_valor_variable valor_mostrar){
	char *valueChar = malloc(sizeof(t_valor_variable));

	memcpy(valueChar,(void*) valor_mostrar, sizeof(valor_mostrar));
	sendMessage (&socket, valueChar, sizeof(t_valor_variable));

	//send to Nucleo valueChar to be printed on Consola

}

void imprimirTexto(char *texto){

	sendMessage (&socket, texto, sizeof(texto)); // TODO string_lenght(texto) ??

}

void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {
	t_es* dispositivoEnviar = malloc(sizeof(t_es));
	dispositivoEnviar->dispositivo = dispositivo;
	dispositivoEnviar->tiempo = tiempo;
	int sizeDS = 0;
	char* dispositivoSerializado = malloc(sizeof(sizeDS));
	serializarES(dispositivoEnviar, dispositivoSerializado);
	sendMessage(&socket, dispositivoSerializado, sizeof(t_es));
	// definir enum_dispositivos
}

void wait(t_nombre_semaforo identificador_semaforo){
	enum_semaforo semID;
	sendMessage(&socket, identificador_semaforo , strlen(identificador_semaforo));
	sendMessage(&socket, semID, sizeof(int));
	//send to Nucleo to execute WAIT function for "identificador_semaforo"

}

void signal(t_nombre_semaforo identificador_semaforo){
	enum_semaforo semID;
	sendMessage(&socket, identificador_semaforo , strlen(identificador_semaforo));
	sendMessage(&socket, semID , sizeof(int));
	//send to Nucleo to execute SIGNAL function for "identificador_semaforo"

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
