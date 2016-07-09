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
int ultimoPosicionPC;
int socketUMC;
void setPageSize (int pageSize){
	tamanioDePagina = pageSize;
}
int socket;



int getLogicalAddress (int page){
	return (page * tamanioDePagina);
}
void destroyRegistroStack(t_registroStack* self){
	free(self->args); //TODO NO SE LIBERA ASI, es una LISTA! HACER list destroy con destroy element correspondiente
	free(self->vars);//TODO NO SE LIBERA ASI, es una LISTA! HACER list destroy con destroy element correspondiente
	free(self->retVar);
}

t_puntero definirVariable(t_nombre_variable identificador){
	t_puntero posicionDeLaVariable;
	t_vars* ultimaPosicionOcupada;
	t_vars* variableAAgregar = malloc(sizeof(t_vars));
	variableAAgregar->direccionValorDeVariable = malloc(sizeof(t_memoryLocation));
	variableAAgregar->identificador=identificador;

	ultimaPosicionOcupada = buscarEnElStackPosicionPagina(PCB);

	if(ultimoPosicionPC == PCB->ProgramCounter){
		t_registroStack* ultimoRegistro = malloc(sizeof(t_registroStack));

		if(ultimaPosicionOcupada->direccionValorDeVariable->offset == tamanioDePagina){
			variableAAgregar->direccionValorDeVariable->pag = ultimaPosicionOcupada->direccionValorDeVariable->pag + 1; //increasing 1 page to last one
			variableAAgregar->direccionValorDeVariable->offset = ultimaPosicionOcupada->direccionValorDeVariable->offset + sizeof(int); // sizeof(int) because of all variables values in AnsisOP are integer
			variableAAgregar->direccionValorDeVariable->size = ultimaPosicionOcupada->direccionValorDeVariable->size;
		}else{//we suppossed that the variable value is NEVER going to be greater than the page size
			variableAAgregar->direccionValorDeVariable->pag = ultimaPosicionOcupada->direccionValorDeVariable->pag;
			variableAAgregar->direccionValorDeVariable->offset = ultimaPosicionOcupada->direccionValorDeVariable->offset + sizeof(int); // sizeof(int) because of all variables values in AnsisOP are integer
			variableAAgregar->direccionValorDeVariable->size=ultimaPosicionOcupada->direccionValorDeVariable->size;
		}

		ultimoRegistro=list_get(PCB->indiceDeStack,PCB->indiceDeStack->elements_count);
		list_add(ultimoRegistro->vars, (void*)variableAAgregar);

		posicionDeLaVariable= &variableAAgregar->direccionValorDeVariable;

		return posicionDeLaVariable;
	}else{
		t_registroStack* registroAAgregar=malloc(sizeof(t_registroStack));

		if(ultimaPosicionOcupada->direccionValorDeVariable->offset==tamanioDePagina){
			variableAAgregar->direccionValorDeVariable->pag=ultimaPosicionOcupada->direccionValorDeVariable->pag+1;
			variableAAgregar->direccionValorDeVariable->offset=ultimaPosicionOcupada->direccionValorDeVariable->offset+4;
			variableAAgregar->direccionValorDeVariable->size=ultimaPosicionOcupada->direccionValorDeVariable->size;
		}else{
			variableAAgregar->direccionValorDeVariable->pag=ultimaPosicionOcupada->direccionValorDeVariable->pag;
			variableAAgregar->direccionValorDeVariable->offset=ultimaPosicionOcupada->direccionValorDeVariable->offset+4;
			variableAAgregar->direccionValorDeVariable->size=ultimaPosicionOcupada->direccionValorDeVariable->size;
		}

		list_add(registroAAgregar->vars,(void*)variableAAgregar);
		registroAAgregar->pos = PCB->indiceDeStack->elements_count - 1;

		list_add(PCB->indiceDeStack,registroAAgregar);

		posicionDeLaVariable= (void*) &variableAAgregar->direccionValorDeVariable;

		return posicionDeLaVariable;
	}
}

t_vars* buscarEnElStackPosicionPagina(t_PCB* pcb){
	t_registroStack* ultimoRegistro;
	t_vars* ultimaPosicionLlena;
	ultimoRegistro=list_get(pcb->indiceDeStack,pcb->indiceDeStack->elements_count);
	ultimaPosicionLlena=list_get(ultimoRegistro->vars,ultimoRegistro->vars->elements_count);
	//TODO 1) TENER EN CUENTA la pagina donde arranca el stack
	//TODO 2) TENER EN CUENTA argumentos para ultima posicion
	return ultimaPosicionLlena;
}




t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	bool condicionVariable(t_vars* unaVariable){
		return (unaVariable->identificador==identificador_variable);
	}
	int i;
	t_registroStack* registroBuscado=malloc(sizeof(t_registroStack));
	t_vars* posicionBuscada=malloc(sizeof(t_memoryLocation));
	t_puntero posicionEncontrada;
	for(i=0;i<PCB->indiceDeStack->elements_count;i++){
		registroBuscado=list_get(PCB->indiceDeStack,i);
		posicionBuscada=list_find(registroBuscado->vars,(void*)condicionVariable);
	}
	posicionEncontrada =(void*)posicionBuscada->direccionValorDeVariable;
	return posicionEncontrada;



}



t_valor_variable dereferenciar(t_puntero direccion_variable){
	t_valor_variable varValue=malloc(sizeof(t_valor_variable));
	t_memoryLocation* posicionSenialada=malloc(sizeof(t_memoryLocation));
	/*memcpy(&posicionSenialada->offset,&direccion_variable,4);
	memcpy(&posicionSenialada->pag,(&direccion_variable+4),4);
	memcpy(&posicionSenialada->size,(&direccion_variable+8),4);*/
	sendMessage(&socketUMC,direccion_variable,sizeof(t_memoryLocation));
	char* valorRecibido;
	receiveMessage(&socket,valorRecibido,sizeof(int));
	memcpy(&varValue,valorRecibido,4);
	return varValue;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){
	//Envio la posicion que tengo que asignar a la umc
	char* direccionVariable=malloc(sizeof(t_memoryLocation));
	memcpy(direccionVariable,&direccion_variable,sizeof(t_memoryLocation));
	sendMessage(&socketUMC,direccionVariable,sizeof(t_memoryLocation));
	char* valorVariable=malloc(sizeof(t_valor_variable));
	memcpy(valorVariable,&valor,sizeof(t_valor_variable));
	sendMessage(&socketUMC,valorVariable,sizeof(t_valor_variable));
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
	t_registroIndiceEtiqueta* registroBuscado=malloc(sizeof(t_memoryLocation));
	t_registroStack* registroAnterior=malloc(sizeof(t_memoryLocation));

	t_registroStack* nuevoRegistroStack=malloc(sizeof(t_registroStack));
	t_memoryLocation* ultimaPosicionDeMemoria=malloc(sizeof(t_memoryLocation));
	t_memoryLocation* nuevaPosicionDeMemoria=malloc(sizeof(t_memoryLocation));
	t_vars* infoVariable=malloc(sizeof(t_vars));
	int condicionEtiquetas(t_nombre_etiqueta unaEtiqueta,t_registroIndiceEtiqueta registroIndiceEtiqueta){
		return (registroIndiceEtiqueta.funcion==unaEtiqueta);
	}
	ultimaPosicionDeMemoria=buscarUltimaPosicionOcupada(PCB);
	nuevaPosicionDeMemoria->offset=ultimaPosicionDeMemoria->offset+4;
	if(ultimaPosicionDeMemoria->offset==tamanioDePagina){
		nuevaPosicionDeMemoria->pag=ultimaPosicionDeMemoria->pag;}
	else{
		nuevaPosicionDeMemoria->pag=ultimaPosicionDeMemoria->pag+1;

	}
	nuevaPosicionDeMemoria->size=ultimaPosicionDeMemoria->size;
	nuevoRegistroStack->retPos=PCB->ProgramCounter;
	list_add(nuevoRegistroStack->args,nuevaPosicionDeMemoria);
	nuevoRegistroStack->pos=PCB->indiceDeStack->elements_count+1;
	registroAnterior=list_get(PCB->indiceDeStack,PCB->indiceDeStack->elements_count);
	infoVariable=(t_vars*)list_get(registroAnterior->vars,registroAnterior->vars->elements_count);
	nuevoRegistroStack->retVar=infoVariable->direccionValorDeVariable;
	registroBuscado=(t_registroIndiceEtiqueta*)list_find(PCB->indiceDeEtiquetas,(void*)condicionEtiquetas);
	PCB->ProgramCounter=registroBuscado->posicionDeLaEtiqueta;

}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	t_valor_variable retorno;
	irAlLabel(etiqueta);

	retornar(retorno);

}

void retornar(t_valor_variable retorno){

	t_registroStack* registroARegresar;
	t_memoryLocation* retVar;
	t_registroStack* registroActual;
	bool condicionRetorno(t_registroStack* unRegistro){
		return (unRegistro->retPos==registroARegresar->pos);
	}
	registroARegresar=(t_registroStack*)list_find(PCB->indiceDeStack,(void*)condicionRetorno);
	PCB->ProgramCounter=registroARegresar->retPos;
	PCB->StackPointer=registroARegresar->pos;
	registroActual=list_get(PCB->indiceDeStack,PCB->StackPointer);
	char* valorRetorno;
	char* retornar;

	memcpy(valorRetorno,registroActual->retVar->offset,4);
	memcpy(valorRetorno,registroActual->retVar->pag,4);
	memcpy(valorRetorno,registroActual->retVar->size,4);
	sendMessage(socketUMC,valorRetorno,sizeof(t_memoryLocation));
	memcpy(&valorRetorno,retorno,4);
	sendMessage(socketUMC,valorRetorno,4);




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

t_memoryLocation* buscarUltimaPosicionOcupada(t_PCB* pcbEjecutando){
	t_registroStack* ultimoRegistro=malloc(sizeof(t_registroStack));
	t_vars* ultimaPosicionDeMemoriaOcupadaVars;
	t_memoryLocation* ultimaPosicionDeMemoriaOcupadaArgs;
	ultimoRegistro=list_get(pcbEjecutando->indiceDeStack,pcbEjecutando->indiceDeStack->elements_count);
	ultimaPosicionDeMemoriaOcupadaArgs=list_get(ultimoRegistro->args,ultimoRegistro->args->elements_count);
	ultimaPosicionDeMemoriaOcupadaVars=list_get(ultimoRegistro->vars,ultimoRegistro->vars->elements_count);
	if(ultimaPosicionDeMemoriaOcupadaArgs->pag>ultimaPosicionDeMemoriaOcupadaVars->direccionValorDeVariable->pag){
		return ultimaPosicionDeMemoriaOcupadaArgs;
	}else{
		return ultimaPosicionDeMemoriaOcupadaVars->direccionValorDeVariable;
	}
	if(ultimaPosicionDeMemoriaOcupadaArgs->pag==ultimaPosicionDeMemoriaOcupadaVars->direccionValorDeVariable->pag){
		if(ultimaPosicionDeMemoriaOcupadaArgs->offset>ultimaPosicionDeMemoriaOcupadaVars->direccionValorDeVariable->offset){
			return ultimaPosicionDeMemoriaOcupadaArgs;
		}else{
			return ultimaPosicionDeMemoriaOcupadaVars->direccionValorDeVariable;
		}
	}
	return NULL;
}
