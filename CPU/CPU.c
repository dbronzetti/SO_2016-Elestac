#include "CPU.h"

int main(int argc, char *argv[]){
	int exitCode = EXIT_SUCCESS;
	char *configurationFile = NULL;
	char* messageRcv = NULL;
	char *logFile = NULL;

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS TODO => Agregar logs con librerias

	//get parameter
	int i;
	for( i = 0; i < argc; i++){
		if (strcmp(argv[i], "-c") == 0){
			configurationFile = argv[i+1];
			printf("Configuration File: '%s'\n",configurationFile);
		}
		//check log file parameter
		if (strcmp(argv[i], "-l") == 0){
			logFile = argv[i+1];
			printf("Log File: '%s'\n",logFile);
		}
	}

	//ERROR if not configuration parameter was passed
	assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS TODO => Agregar logs con librerias

	//ERROR if not log parameter was passed
	assert(("ERROR - NOT log file was passed as argument", logFile != NULL));//Verifies if was passed the log file as parameter, if DONT FAILS

	//get configuration from file
	crearArchivoDeConfiguracion(configurationFile);

	//Creo Archivo de Log
	logCPU = log_create(logFile,"CPU",0,LOG_LEVEL_TRACE);

	//TODO CPU 2.5.2 - Si fue captada la señal SIGUSR1 mientras se estaba ejecutando una instruccion se debe finalizar la misma y acto seguido desconectarse del Nucleo

	exitCode = connectTo(UMC,&socketUMC);
	if(exitCode == EXIT_SUCCESS){
		printf("CPU connected to UMC successfully\n");
	}

	exitCode = connectTo(NUCLEO,&socketNucleo);

	while (exitCode == EXIT_SUCCESS){

		if(exitCode == EXIT_SUCCESS){
			printf("CPU connected to NUCLEO successfully\n");
			messageRcv = malloc(sizeof(int));//for receiving message size
			waitRequestFromNucleo(&socketNucleo, messageRcv);
		}

		//exitCode=receiveMessage(&socketNucleo,PCBrecibido,sizeof(t_PCB));

		if(messageRcv !=  NULL){
			int messageSize = 0;
			int receivedBytes = 0;
			//deserealizar y construir PCB
			t_MessageNucleo_CPU* messageWithBasicPCB = malloc(sizeof(t_MessageNucleo_CPU));

			deserializeCPU_Nucleo(messageWithBasicPCB, messageRcv);

			log_info(logCPU,"Construyendo PCB para PID #%d\n",messageWithBasicPCB->processID);

			PCBRecibido->PID = messageWithBasicPCB->processID;
			PCBRecibido->ProgramCounter = messageWithBasicPCB->programCounter;
			PCBRecibido->StackPointer = messageWithBasicPCB->stackPointer;
			PCBRecibido->cantidadDePaginas = messageWithBasicPCB->cantidadDePaginas;
			PCBRecibido->indiceDeEtiquetas = messageWithBasicPCB->indiceDeEtiquetas;
			PCBRecibido->indiceDeCodigo = list_create();
			PCBRecibido->indiceDeStack = list_create();
			QUANTUM = messageWithBasicPCB->quantum;
			QUANTUM_SLEEP = messageWithBasicPCB->quantum_sleep;
			ultimoPosicionPC = PCBRecibido->ProgramCounter;

			//receive tamaño de lista indice codigo
			messageRcv = realloc(messageRcv, sizeof(messageSize));
			receivedBytes = receiveMessage(&socketNucleo, messageRcv, sizeof(messageSize));

			//receive lista indice codigo
			messageRcv = realloc(messageRcv, atoi(messageRcv));
			receivedBytes = receiveMessage(&socketNucleo, messageRcv, atoi(messageRcv));

			//deserializar estructuras del indice de codigo
			deserializarListaIndiceDeCodigo(PCBRecibido->indiceDeCodigo, messageRcv);

			//receive tamaño de lista indice stack
			messageRcv = realloc(messageRcv, sizeof(messageSize));
			receivedBytes = receiveMessage(&socketNucleo, messageRcv, sizeof(messageSize));

			//receive lista indice stack
			messageRcv = realloc(messageRcv, atoi(messageRcv));
			receivedBytes = receiveMessage(&socketNucleo, messageRcv, atoi(messageRcv));

			//deserializar estructuras del stack
			deserializarListaStack(PCBRecibido->indiceDeStack, messageRcv);

			//Create list for IndiceDeEtiquetas
			deserializarListaIndiceDeEtiquetas(PCBRecibido->indiceDeEtiquetas, messageWithBasicPCB->indiceDeEtiquetasTamanio);

			printf("El PCB fue recibido correctamente\n");

			int j;
			for(j=0;j < QUANTUM;j++){

				exitCode = ejecutarPrograma();

				if (exitCode == EXIT_SUCCESS){

					sleep(QUANTUM_SLEEP);

					if(j == QUANTUM){

						log_error(logCPU, "Corte por quantum cumplido - Proceso %d ", PCBRecibido->PID);

						t_MessageCPU_Nucleo* corteQuantum = malloc( sizeof(t_MessageCPU_Nucleo));
						corteQuantum->operacion = 5;//operacion 5 es por quantum
						corteQuantum->processID = PCBRecibido->PID;

						int payloadSize = sizeof(corteQuantum->operacion) + sizeof(corteQuantum->processID);
						int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

						char* bufferRespuesta = malloc(bufferSize);
						serializeCPU_Nucleo(corteQuantum, bufferRespuesta, payloadSize);
						sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

						free(bufferRespuesta);

					}
					if(j == PCBRecibido->indiceDeCodigo->elements_count){
						//TODO esto esta mal, esta enviando 2 mensajes seguidos con dos operaciones distintas al NUCLEO - REORGANIZAR!!!!!!!
						if(PCBRecibido->ProgramCounter == PCBRecibido->indiceDeCodigo->elements_count){

							log_info(logCPU, "Proceso %d - Finalizado correctamente", PCBRecibido->PID);
							//Envia aviso que finaliza correctamente el proceso a NUCLEO
							t_MessageCPU_Nucleo* respuestaFinOK = malloc(sizeof(t_MessageCPU_Nucleo));
							respuestaFinOK->operacion = 2;
							respuestaFinOK->processID = PCBRecibido->PID;

							int payloadSize = sizeof(respuestaFinOK->operacion) + sizeof(respuestaFinOK->processID);
							int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

							char* bufferRespuesta = malloc(bufferSize);
							serializeCPU_Nucleo(respuestaFinOK, bufferRespuesta, payloadSize);
							sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

							free(bufferRespuesta);

							j = QUANTUM;
						}

					}

				}

			}

			free(messageRcv);
			//TODO Destruir PCBRecibido
			//TODO Destruir listaIndiceEtiquetas

		}

	}//llave agregada, faltaba para cerrar el main

	return EXIT_SUCCESS;
}

int ejecutarPrograma(){
	int exitCode = EXIT_SUCCESS;
	t_registroIndiceCodigo* instruccionActual = malloc(sizeof(t_registroIndiceCodigo));

	instruccionActual = (t_registroIndiceCodigo*) list_get(PCBRecibido->indiceDeCodigo,PCBRecibido->ProgramCounter);

	int i;
	int offsetInstruccionesSize = 0;
	t_registroIndiceCodigo* instruccion = malloc(sizeof(t_registroIndiceCodigo));

	//For getting the correct page in which the current instruction has to be located we need the offset from all the previous lines executed
	//NOTE: we follow the code line because doesn't matter if there was a function call (this is because after a function call all the variables in there are deleted from memory)
	for(i = 0; i < instruccionActual->inicioDeInstruccion; i++){
		instruccion = (t_registroIndiceCodigo*) list_get(PCBRecibido->indiceDeCodigo, i);
		offsetInstruccionesSize += instruccion->longitudInstruccionEnBytes;
	}

	free(instruccion);

	char* codigoRecibido = malloc(instruccionActual->longitudInstruccionEnBytes);

	log_info(logCPU,"Contexto de ejecucion recibido - Process ID : %d - PC : %d", PCBRecibido->PID, PCBRecibido->ProgramCounter);

	//serializar mensaje para UMC y solicitar codigo porcion de codigo
	int bufferSize = 0;
	int payloadSize = 0;

	//overwrite page content in swap (swap out)
	t_MessageCPU_UMC *message = malloc(sizeof(t_MessageCPU_UMC));
	message->virtualAddress = (t_memoryLocation*) malloc(sizeof(t_memoryLocation));
	message->PID = PCBRecibido->PID;
	message->operation = lectura_pagina;
	message->virtualAddress->pag = offsetInstruccionesSize/frameSize;
	message->virtualAddress->offset = offsetInstruccionesSize%frameSize; //TODO double check this line
	message->virtualAddress->size = instruccionActual->longitudInstruccionEnBytes;

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(t_memoryLocation);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char *buffer = malloc(bufferSize);

	serializeCPU_UMC(message, buffer, payloadSize);

	//Send information to UMC - message serialized with virtualAddress information
	sendMessage(&socketUMC, buffer, bufferSize);

	int returnCode = EXIT_SUCCESS;//DEFAULT

	//First answer from UMC is the exit code from the operation
	exitCode = receiveMessage(&socketUMC,&returnCode, sizeof(exitCode));

	if( returnCode == EXIT_FAILURE){

		//Envia aviso que finaliza incorrectamente el proceso a NUCLEO
		log_error(logCPU, "No se pudo obtener la solicitud a ejecutar - Proceso %d - Error al finalizar", PCBRecibido->PID);

		t_MessageCPU_Nucleo* respuestaFinFalla = malloc( sizeof(t_MessageCPU_Nucleo));
		respuestaFinFalla->operacion = 4;
		respuestaFinFalla->processID = PCBRecibido->PID;

		int payloadSize = sizeof(respuestaFinFalla->operacion) + sizeof(respuestaFinFalla->processID);
		int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

		char* bufferRespuesta = malloc(bufferSize);
		serializeCPU_Nucleo(respuestaFinFalla, bufferRespuesta, payloadSize);
		sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

		free(bufferRespuesta);

		exitCode = returnCode;

	}else{
		//Receiving information from UMC when the operation was successfully accomplished
		exitCode = receiveMessage(&socketUMC, &codigoRecibido, instruccionActual->longitudInstruccionEnBytes);

		analizadorLinea(codigoRecibido, funciones, funciones_kernel);
		ultimoPosicionPC++;
	}

	free(buffer);
	free(codigoRecibido);

	return exitCode;
}

int connectTo(enum_processes processToConnect, int *socketClient){
	int exitcode = EXIT_FAILURE;//DEFAULT VALUE
	int port = 0;
	char *ip = string_new();

	switch (processToConnect){
	case UMC:{
		string_append(&ip,configuration.ip_UMC);
		port= configuration.port_UMC;
		break;
	}
	case NUCLEO:{
		string_append(&ip,configuration.ip_Nucleo);
		port= configuration.port_Nucleo;
		break;
	}
	default:{
		log_info(logCPU,"Process '%s' NOT VALID to be connected by NUCLEO.\n", getProcessString(processToConnect));
		break;
	}
	}

	exitcode = openClientConnection(ip, port, socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == EXIT_SUCCESS){

		// ***1) Send handshake
		exitcode = sendClientHandShake(socketClient,CPU);

		if (exitcode == EXIT_SUCCESS){

			// ***2)Receive handshake response
			//Receive message size
			int messageSize = 0;
			char *messageRcv = malloc(sizeof(messageSize));
			int receivedBytes = receiveMessage(socketClient, messageRcv, sizeof(messageSize));

			if ( receivedBytes > 0 ){
				//Receive message using the size read before
				memcpy(&messageSize, messageRcv, sizeof(int));
				messageRcv = realloc(messageRcv,messageSize);
				receivedBytes = receiveMessage(socketClient, messageRcv, messageSize);

				//starting handshake with client connected
				t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
				deserializeHandShake(message, messageRcv);

				free(messageRcv);

				switch (message->process){
				case ACCEPTED:{
					switch(processToConnect){
					case UMC:{
						log_info(logCPU, "Connected to UMC - Message: %s\n",message->message);
						printf("Receiving frame size\n");
						//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
						receivedBytes = receiveMessage(socketClient, &frameSize, sizeof(messageSize));

						printf("Tamanio de pagina: %d\n",frameSize);
						break;
					}
					case NUCLEO:{
						log_info(logCPU, "Connected to NUCLEO - Message: %s\n",message->message);
						printf("Receiving stack size (number of pages for PID stack)\n");
						//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
						receivedBytes = receiveMessage(socketClient, &stackSize, sizeof(messageSize));
						break;
					}
					default:{
						log_error(logCPU,
								"Handshake not accepted when tried to connect your '%s' with '%s'\n",
								getProcessString(processToConnect),	getProcessString(message->process));
						close(*socketClient);
						exitcode = EXIT_FAILURE;
						break;
					}
					}

					break;
				}
				default:{
					log_error(logCPU,"Process couldn't connect to SERVER - Not able to connect to server %s. Please check if it's down.\n",ip);
					close(*socketClient);
					break;
				}
				}

			}else if (receivedBytes == 0 ){
				//The client is down when bytes received are 0
				log_error(logCPU,"The client went down while receiving! - Please check the client '%d' is down!\n",*socketClient);
				close(*socketClient);
			}else{
				log_error(logCPU, "Error - No able to received - Error receiving from socket '%d', with error: %d\n",*socketClient, errno);
				close(*socketClient);
			}
		}

	}else{
		log_error(logCPU, "I'm not able to connect to the server! - My socket is: '%d'\n", *socketClient);
		close(*socketClient);
	}

	return exitcode;
}

void waitRequestFromNucleo(int *socketClient, char * messageRcv){
	//Receive message size
	int messageSize = 0;
	int receivedBytes = receiveMessage(socketClient, messageRcv, sizeof(messageSize));

	if ( receivedBytes > 0 ){
		//Get Payload size
		messageSize = atoi(messageRcv);

		//Receive process from which the message is going to be interpreted
		enum_processes fromProcess;
		messageRcv = realloc(messageRcv, sizeof(fromProcess));
		receivedBytes = receiveMessage(socketClient, messageRcv, sizeof(fromProcess));
		fromProcess = (enum_processes) messageRcv;

		//Receive message using the size read before
		messageRcv = realloc(messageRcv,messageSize);
		receivedBytes = receiveMessage(socketClient, messageRcv, messageSize);

		log_info(logCPU,"Bytes received from process '%s': %d\n",getProcessString(fromProcess),receivedBytes);

	}else{
		messageRcv = NULL;
	}

}

void deserializarListaIndiceDeEtiquetas(char* charEtiquetas, int listaSize){
	int offset = 0;

	listaIndiceEtiquetas = list_create();

	while ( offset < listaSize ){
		t_registroIndiceEtiqueta *regIndiceEtiqueta = malloc(sizeof(t_registroIndiceEtiqueta));

		int j = 0;
		for (j = 0; charEtiquetas[offset + j] != '\0'; j++);

		regIndiceEtiqueta->funcion = malloc(j);
		memset(regIndiceEtiqueta->funcion,'\0', j);

		regIndiceEtiqueta->funcion = string_substring(charEtiquetas, offset, j);
		offset += j + 1;//+1 por '\0's

		log_info(logCPU,"funcion: %s\n", regIndiceEtiqueta->funcion);

		memcpy(&regIndiceEtiqueta->posicionDeLaEtiqueta, charEtiquetas +offset, sizeof(regIndiceEtiqueta->posicionDeLaEtiqueta));
		offset += sizeof(regIndiceEtiqueta->posicionDeLaEtiqueta);

		log_info(logCPU,"posicionDeLaEtiqueta: %d\n", regIndiceEtiqueta->posicionDeLaEtiqueta);

		list_add(listaIndiceEtiquetas,(void*)regIndiceEtiqueta);

	}

}

void crearArchivoDeConfiguracion(char *configFile){
	t_config* configurationFile;
	configurationFile = config_create(configFile);
	configuration.ip_Nucleo = config_get_string_value(configurationFile,"IP_NUCLEO");
	configuration.port_Nucleo = config_get_int_value(configurationFile,"PUERTO_NUCLEO");
	configuration.ip_UMC = config_get_string_value(configurationFile,"IP_UMC");
	configuration.port_UMC = config_get_int_value(configurationFile,"PUERTO_UMC");
}

void serializarES(t_es *value, t_nombre_dispositivo buffer, int valueSize){
	int offset = 0;

	//valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//tiempo
	memcpy(buffer + offset, (void*) value->tiempo, sizeof(int));
	offset += sizeof(int);

	//ProgramCounter
	memcpy(buffer + offset, (void*) value->ProgramCounter, sizeof(int));
	offset += sizeof(int);

	//dispositivo length
	int dispositivoLen = strlen(value->dispositivo) + 1;
	memcpy(buffer + offset, &dispositivoLen, sizeof(dispositivoLen));
	offset += sizeof(dispositivoLen);

	//Dispositivo
	memcpy(buffer + offset, value->dispositivo, dispositivoLen);

}

void finalizar(void){

	//TODO 1) ANALIZA SI ES EL FINAL DEL PROGRAMA
	//TODO 2) RETORNAR A PUNTO PREVIO DE PROGRAM COUNTER Y ELIMINAR REGISTRO DE STACK CORRESPONDIENTE A LA FUNCION (BUSCAR EL QUE TENGA RETPOS)

}

t_puntero definirVariable(t_nombre_variable identificador){
	t_puntero posicionDeLaVariable;
	t_memoryLocation* ultimaPosicionOcupada = NULL;
	t_vars* variableAAgregar = malloc(sizeof(t_vars));
	variableAAgregar->direccionValorDeVariable = malloc(sizeof(t_memoryLocation));
	variableAAgregar->identificador = identificador;

	if(!list_is_empty(PCBRecibido->indiceDeStack)){

		ultimaPosicionOcupada = buscarEnElStackPosicionPagina(PCBRecibido);

		if( (ultimoPosicionPC == PCBRecibido->ProgramCounter) && (ultimaPosicionOcupada != NULL)){ //check if is the first row in stack
			t_registroStack* ultimoRegistro = malloc(sizeof(t_registroStack));

			cargarValoresNuevaPosicion(ultimaPosicionOcupada, variableAAgregar->direccionValorDeVariable);

			ultimoRegistro = list_get(PCBRecibido->indiceDeStack, PCBRecibido->indiceDeStack->elements_count);
			list_add(ultimoRegistro->vars, (void*)variableAAgregar);

			posicionDeLaVariable= (t_puntero) &variableAAgregar->direccionValorDeVariable;

			free(ultimoRegistro);
		}else{//executing different program counter
			t_registroStack* registroAAgregar = malloc(sizeof(t_registroStack));
			registroAAgregar->retVar = NULL; //Memory position to return BY DEFAULT
			registroAAgregar->retPos = -1; //DEFAULT VALUE
			registroAAgregar->args = list_create();
			registroAAgregar->vars = list_create();
			registroAAgregar->pos = PCBRecibido->indiceDeStack->elements_count - 1;

			cargarValoresNuevaPosicion(ultimaPosicionOcupada, variableAAgregar->direccionValorDeVariable);

			//Every time a record to IndiceStack is created have to be loaded all its variables
			list_add(registroAAgregar->vars,(void*)variableAAgregar);

			list_add(PCBRecibido->indiceDeStack,registroAAgregar);

			posicionDeLaVariable= (t_puntero) &variableAAgregar->direccionValorDeVariable;
		}

	}else{
		t_registroStack* registroAAgregar = malloc(sizeof(t_registroStack));
		registroAAgregar->retVar = NULL; //Memory position to return BY DEFAULT
		registroAAgregar->retPos = -1; //DEFAULT VALUE
		registroAAgregar->args = list_create();
		registroAAgregar->vars = list_create();
		registroAAgregar->pos = 0;//first element in registro stack

		cargarValoresNuevaPosicion(ultimaPosicionOcupada, variableAAgregar->direccionValorDeVariable);

		//Every time a record to IndiceStack is created have to be loaded all its variables
		list_add(registroAAgregar->vars,(void*)variableAAgregar);

		list_add(PCBRecibido->indiceDeStack,registroAAgregar);

		posicionDeLaVariable= (t_puntero) &variableAAgregar->direccionValorDeVariable;

	}

	return posicionDeLaVariable;
}

void cargarValoresNuevaPosicion(t_memoryLocation* ultimaPosicionOcupada, t_memoryLocation* variableAAgregar){

	if (ultimaPosicionOcupada == NULL){
		ultimaPosicionOcupada = malloc(sizeof(ultimaPosicionOcupada));
		//TODO 1) TENER EN CUENTA la pagina donde arranca el stack
		ultimaPosicionOcupada->pag = 0; //DEFAULT value for first row
		ultimaPosicionOcupada->offset = 0; //DEFAULT value for first row
		ultimaPosicionOcupada->size = sizeof(int); //DEFAULT value for first row

	}else{

		if(ultimaPosicionOcupada->offset == (frameSize - sizeof(int)) ){//(frameSize - sizeof(int)) is because the offset is always the start position
			variableAAgregar->pag = ultimaPosicionOcupada->pag + 1; //increasing 1 page to last one
			variableAAgregar->offset = 0 ; // sizeof(int) because of all variables values in AnsisOP are integer
			variableAAgregar->size = ultimaPosicionOcupada->size;
		}else{//we suppossed that the variable value is NEVER going to be greater than the page size
			variableAAgregar->pag = ultimaPosicionOcupada->pag;
			variableAAgregar->offset = ultimaPosicionOcupada->offset + sizeof(int); // sizeof(int) because of all variables values in AnsisOP are integer
			variableAAgregar->size = ultimaPosicionOcupada->size;
		}
	}

}

t_memoryLocation* buscarEnElStackPosicionPagina(t_PCB* pcb){

	t_memoryLocation* ultimaPosicionLlena = malloc(sizeof(t_memoryLocation));

	if(!list_is_empty(pcb->indiceDeStack)){//first row in stack
		t_registroStack* ultimoRegistro = malloc(sizeof(t_registroStack));
		ultimoRegistro = list_get(pcb->indiceDeStack, pcb->indiceDeStack->elements_count -1 );

		if(list_is_empty(ultimoRegistro->args)){//first row in stack
			t_vars* ultimaRegistroVars = NULL;
			ultimaRegistroVars = list_get(ultimoRegistro->vars, ultimoRegistro->vars->elements_count -1);
			memcpy(ultimaPosicionLlena, ultimaRegistroVars->direccionValorDeVariable, sizeof(t_memoryLocation));
		}else{
			if (list_is_empty(ultimoRegistro->vars)){
				ultimaPosicionLlena = list_get(ultimoRegistro->args, ultimoRegistro->args->elements_count -1);
			}else{
				t_vars* ultimaRegistroVars = NULL;
				ultimaRegistroVars = list_get(ultimoRegistro->vars, ultimoRegistro->vars->elements_count -1);
				memcpy(ultimaPosicionLlena, ultimaRegistroVars->direccionValorDeVariable, sizeof(t_memoryLocation));
			}
		}

		free(ultimoRegistro);
	}

	return ultimaPosicionLlena;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	//TODO REVISAR Y CONFIRMAR SI ESTA BIEN

	bool condicionVariable(t_vars* unaVariable){
		return (unaVariable->identificador==identificador_variable);
	}
	int i;
	t_registroStack* registroBuscado=malloc(sizeof(t_registroStack));
	t_vars* posicionBuscada=malloc(sizeof(t_memoryLocation));
	t_puntero posicionEncontrada;
	for(i=0; i < PCBRecibido->indiceDeStack->elements_count; i++){
		registroBuscado = list_get(PCBRecibido->indiceDeStack,i);
		posicionBuscada = list_find(registroBuscado->vars,(void*)condicionVariable);
	}
	posicionEncontrada =(int) &posicionBuscada->direccionValorDeVariable;

	return posicionEncontrada;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){

	//TODO MAL!!! SERIALIZAR A UMC COMO CORRESPONDE (ENVIAR \0 PARA TOTAL DEL CONTENIDO DE LA VARIABLE) ---> SE PUEDE TOMAR COMO EJEMPLO EL ASIGNAR

	t_valor_variable *varValue = malloc(sizeof(t_valor_variable));
	t_memoryLocation *posicionSenialada = malloc(sizeof(t_memoryLocation));
	/*memcpy(&posicionSenialada->offset,&direccion_variable,4);
	memcpy(&posicionSenialada->pag,(&direccion_variable+4),4);
	memcpy(&posicionSenialada->size,(&direccion_variable+8),4);*/
	sendMessage(&socketUMC, &direccion_variable, sizeof(t_memoryLocation));
	char* valorRecibido = NULL;
	receiveMessage(&socketUMC,valorRecibido,sizeof(int));
	memcpy(varValue,valorRecibido,4);
	return *varValue;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){
	//Envio la posicion que tengo que asignar a la umc

	// inform new program to swap and check if it could write it.
	int bufferSize = 0;
	int payloadSize = 0;

	//overwrite page content in swap (swap out)
	t_MessageCPU_UMC *message = malloc(sizeof(t_MessageCPU_UMC));
	message->virtualAddress = (t_memoryLocation*) malloc(sizeof(t_memoryLocation));
	message->PID = PCBRecibido->PID;
	message->operation = escritura_pagina;
	message->virtualAddress = (t_memoryLocation*) direccion_variable;

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(t_memoryLocation);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char *buffer = malloc(bufferSize);

	serializeCPU_UMC(message, buffer, payloadSize);

	//Send information to UMC - message serialized with virtualAddress information
	sendMessage(&socketUMC, buffer, bufferSize);

	//1) First send with size of the message
	sendMessage(&socketUMC, (void*) sizeof(t_valor_variable), sizeof(t_valor_variable));
	//2) Second send with value with previous size sent
	sendMessage(&socketUMC,&valor,sizeof(t_valor_variable));

	free(message->virtualAddress);
	free(message);
	free(buffer);
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){

	//Envia info al proceso a NUCLEO
	t_MessageCPU_Nucleo* respuesta = malloc(sizeof(t_MessageCPU_Nucleo));
	respuesta->operacion = 6;//Le digo al Nucleo obtener valor
	respuesta->processID = PCBRecibido->PID;

	int payloadSize = sizeof(respuesta->operacion) + sizeof(respuesta->processID);
	int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char* bufferRespuesta = malloc(bufferSize);
	serializeCPU_Nucleo(respuesta, bufferRespuesta, payloadSize);

	sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

	//1) Envia el tamanio de la variable al proceso NUCLEO
	string_append(&variable,"\0");
	int variableLen = strlen(variable) + 1;
	sendMessage(&socketNucleo, &variableLen, sizeof(variableLen));

	//2) Envia variable al proceso NUCLEO
	sendMessage(&socketNucleo, variable, variableLen);

	t_valor_variable valorVariableDeserializado;
	char* valorVariableSerializado = NULL;

	int valorDeError;

	valorDeError=sendMessage(&socketNucleo, variable, variableLen);
	if(valorDeError!=-1){
		printf("Los datos se enviaron correctamente");
		if(receiveMessage(&socketNucleo,valorVariableSerializado,sizeof(t_valor_variable))!=-1){
			//TODO verificar que no es necesario deserializar
			memcpy(&valorVariableDeserializado, valorVariableSerializado, sizeof(t_valor_variable));
		}
	}else{
		printf("Los datos no pudieron ser enviados");

	}
	return valorVariableDeserializado;


}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){

	//Envia info al proceso a NUCLEO
	t_MessageCPU_Nucleo* respuesta = malloc(sizeof(t_MessageCPU_Nucleo));
	respuesta->operacion = 7; //Le digo al Nucleo grabar valor en la variable
	respuesta->processID = PCBRecibido->PID;

	int payloadSize = sizeof(respuesta->operacion) + sizeof(respuesta->processID);
	int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char* bufferRespuesta = malloc(bufferSize);
	serializeCPU_Nucleo(respuesta, bufferRespuesta, payloadSize);

	sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

	//1) Envia mensaje con el valor
	sendMessage(&socketNucleo, &valor, sizeof(valor));

	//2) Envia el tamanio de la variable al proceso NUCLEO
	string_append(&variable,"\0");
	int variableLen = strlen(variable) + 1;
	sendMessage(&socketNucleo, &variableLen, sizeof(variableLen));

	//3) Envia variable al proceso NUCLEO
	sendMessage(&socketNucleo, variable, variableLen);

	return valor;
}

void irAlLabel(t_nombre_etiqueta etiqueta){
	t_registroIndiceEtiqueta* registroBuscado = malloc(sizeof(t_registroIndiceEtiqueta));

	int condicionEtiquetas(t_registroIndiceEtiqueta registroIndiceEtiqueta){
		return (registroIndiceEtiqueta.funcion == etiqueta);
	}

	registroBuscado = (t_registroIndiceEtiqueta*) list_find(listaIndiceEtiquetas,(void*)condicionEtiquetas);

	//get program counter needed
	ultimoPosicionPC = registroBuscado->posicionDeLaEtiqueta;

	free(registroBuscado);

}

//AFTER THIS FUNCTION IS ALWAYS CALLED definirVariable
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){

	//Saving context and adding new entry to indiceStack
	t_registroStack* registroAAgregar = malloc(sizeof(t_registroStack));
	registroAAgregar->retVar = malloc(sizeof(t_memoryLocation));
	registroAAgregar->retVar = (t_memoryLocation*) donde_retornar; //Memory position where the return has to load the function output
	registroAAgregar->retPos = ultimoPosicionPC; //At the end of the function it has to return to the current program counter
	registroAAgregar->args = list_create();
	registroAAgregar->vars = list_create();

	if(!list_is_empty(PCBRecibido->indiceDeStack)){
		t_registroStack* ultimoRegistro = malloc(sizeof(t_registroStack));
		ultimoRegistro = list_get(PCBRecibido->indiceDeStack, PCBRecibido->indiceDeStack->elements_count - 1);
		registroAAgregar->pos = ultimoRegistro->pos + 1;
		free(ultimoRegistro);
	}else{
		registroAAgregar->pos = 0;
	}

	t_memoryLocation* ultimaPosicionOcupada = NULL;
	ultimaPosicionOcupada = buscarEnElStackPosicionPagina(PCBRecibido);

	//WE SUPPOSED THAT ALWAYS THE ARGS CREATION IS GOING TO BE THE FIRST ONE (before vargs records)
	t_memoryLocation* nuevoRegistroArgs = malloc(sizeof(t_memoryLocation));

	cargarValoresNuevaPosicion(ultimaPosicionOcupada, nuevoRegistroArgs);

	list_add(registroAAgregar->args,(void*)nuevoRegistroArgs);

	list_add(PCBRecibido->indiceDeStack,registroAAgregar);

	//Cambiar de program counter segun etiqueta
	irAlLabel(etiqueta);

}

void retornar(t_valor_variable retorno){

	t_registroStack* registroARegresar;
	t_registroStack* registroActual;

	bool condicionRetorno(t_registroStack* unRegistro){
		return (unRegistro->retPos == registroARegresar->pos);
	}

	registroARegresar = (t_registroStack*)list_find(PCBRecibido->indiceDeStack,(void*)condicionRetorno);
	ultimoPosicionPC = registroARegresar->retPos;
	PCBRecibido->StackPointer = registroARegresar->pos;
	registroActual = list_get(PCBRecibido->indiceDeStack,PCBRecibido->StackPointer);

	char* valorRetorno = malloc(sizeof(registroActual->retVar));
	int offset = 0;

	//1) Send memory location to UMC
	serializeMemoryLocation(registroActual->retVar, valorRetorno, &offset);
	sendMessage(&socketUMC, valorRetorno, sizeof(registroActual));

	//2) Send value to UMC (variable values are always int)
	valorRetorno = realloc(valorRetorno, sizeof(t_valor_variable));
	memcpy(valorRetorno, &retorno, sizeof(t_valor_variable));
	sendMessage(&socketUMC, valorRetorno, sizeof(t_valor_variable));

	free(valorRetorno);
}

void imprimir(t_valor_variable valor_mostrar){

	//Envia info al proceso a NUCLEO
	t_MessageCPU_Nucleo* respuesta = malloc(sizeof(t_MessageCPU_Nucleo));
	respuesta->operacion = 10;
	respuesta->processID = PCBRecibido->PID;

	int payloadSize = sizeof(respuesta->operacion) + sizeof(respuesta->processID);
	int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char* bufferRespuesta = malloc(bufferSize);
	serializeCPU_Nucleo(respuesta, bufferRespuesta, payloadSize);

	sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

	//Envio mensaje con valor a imprimir - send to Nucleo valueChar to be printed on Consola
	sendMessage(&socketNucleo, &valor_mostrar, sizeof(valor_mostrar));

}

void imprimirTexto(char *texto){

	//Envia info al proceso a NUCLEO
	t_MessageCPU_Nucleo* respuesta = malloc(sizeof(t_MessageCPU_Nucleo));
	respuesta->operacion = 11;
	respuesta->processID = PCBRecibido->PID;

	int payloadSize = sizeof(respuesta->operacion) + sizeof(respuesta->processID);
	int bufferSize = sizeof(bufferSize) + payloadSize ;

	char* bufferRespuesta = malloc(bufferSize);
	serializeCPU_Nucleo(respuesta, bufferRespuesta, payloadSize);

	sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

	// Envia el tamanio del texto al proceso NUCLEO
	string_append(&texto,"\0");
	int textoLen = strlen(texto) + 1;
	sendMessage(&socketNucleo, &textoLen, sizeof(textoLen));

	// Envia el texto al proceso NUCLEO
	sendMessage(&socketNucleo, texto, textoLen);

}

void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo) {
	printf("ENTRADA SALIDA...\n");

	//Prepara estructuras para mandarle al NUCLEO
	t_MessageCPU_Nucleo* entradaSalida = malloc(sizeof(t_MessageCPU_Nucleo));
	entradaSalida->operacion = 1;
	entradaSalida->processID = PCBRecibido->PID;
	//*banderaFinQuantum = 1;//TODO esto se puede hacer afuera

	int payloadSize = sizeof(entradaSalida->operacion) + sizeof(entradaSalida->processID);
	int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char* bufferRespuesta = malloc(bufferSize);
	//Serializar y enviar al NUCLEO
	serializeCPU_Nucleo(entradaSalida, bufferRespuesta, payloadSize);
	sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

	t_es* datosParaPlanifdeES = malloc(sizeof(t_es));
	datosParaPlanifdeES->ProgramCounter = ultimoPosicionPC;
	datosParaPlanifdeES->tiempo = tiempo;

	int payloadSizeES = sizeof(datosParaPlanifdeES->tiempo) + sizeof(datosParaPlanifdeES->ProgramCounter)
			+ strlen(datosParaPlanifdeES->dispositivo)+1;
	int bufferSizeES = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char* bufferDatosES = malloc(bufferSizeES);
	//Entrada Salida: Serializar y enviar al NUCLEO
	serializarES(datosParaPlanifdeES, bufferDatosES,payloadSizeES);
	sendMessage(&socketNucleo, bufferDatosES, bufferSizeES);

	log_info(logCPU, "proceso: %d en entrada-salida de tiempo: %d \n", PCBRecibido->PID,tiempo);

	free(bufferRespuesta);
	free(entradaSalida);
	free(bufferDatosES);
	free(datosParaPlanifdeES);

}

void wait(t_nombre_semaforo identificador_semaforo){

	//Envia info al proceso a NUCLEO
	t_MessageCPU_Nucleo* respuesta = malloc(sizeof(t_MessageCPU_Nucleo));
	respuesta->operacion = 8;//8 Grabar semaforo (WAIT)
	respuesta->processID = PCBRecibido->PID;

	int payloadSize = sizeof(respuesta->operacion) + sizeof(respuesta->processID);
	int bufferSize = sizeof(bufferSize) + payloadSize ;

	char* bufferRespuesta = malloc(bufferSize);
	serializeCPU_Nucleo(respuesta, bufferRespuesta, payloadSize);

	sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

	//send to Nucleo to execute WAIT function for "identificador_semaforo"
	int semaforoLen = strlen(identificador_semaforo) + 1;
	char* respuestaNucleo = malloc(sizeof(int));
	int respuestaRecibida;

	// envio al Nucleo el tamanio del semaforo
	sendMessage(&socketNucleo, &semaforoLen , sizeof(semaforoLen));

	// envio al Nucleo el semaforo
	string_append(&identificador_semaforo, "\0");
	sendMessage(&socketNucleo, identificador_semaforo, semaforoLen);

	// recibir respuesta del Nucleo
	receiveMessage(&socketNucleo,respuestaNucleo,sizeof(int));
	//TODO verificar que no hace falta deserializar
	memcpy(&respuestaRecibida,respuestaNucleo,sizeof(int));
	if(respuestaRecibida==1){
		//TODO devolver PCB al nucleo
	}

}

void signal(t_nombre_semaforo identificador_semaforo){

	//Envia info al proceso a NUCLEO
	t_MessageCPU_Nucleo* respuesta = malloc(sizeof(t_MessageCPU_Nucleo));
	respuesta->operacion = 9;//9 es liberarSemafro (SIGNAL)
	respuesta->processID = PCBRecibido->PID;

	int payloadSize = sizeof(respuesta->operacion) + sizeof(respuesta->processID);
	int bufferSize = sizeof(bufferSize) + payloadSize ;

	char* bufferRespuesta = malloc(bufferSize);
	serializeCPU_Nucleo(respuesta, bufferRespuesta, payloadSize);

	sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

	//send to Nucleo to execute SIGNAL function for "identificador_semaforo"
	int semaforoLen=strlen(identificador_semaforo)+1;

	// envio al Nucleo el tamanio del semaforo
	sendMessage(&socketNucleo, &semaforoLen , sizeof(semaforoLen));

	// envio al Nucleo el semaforo
	string_append(&identificador_semaforo, "\0");
	sendMessage(&socketNucleo, identificador_semaforo, semaforoLen);

}

//TODO VERIFICAR SI ESTA FUNCION SE USA O SE PUEDE BORRAR
t_memoryLocation* buscarUltimaPosicionOcupada(t_PCB* pcbEjecutando){
	t_registroStack* ultimoRegistro = malloc(sizeof(t_registroStack));
	t_vars* ultimaPosicionDeMemoriaOcupadaVars;
	t_memoryLocation* ultimaPosicionDeMemoriaOcupadaArgs;

	ultimoRegistro = list_get(pcbEjecutando->indiceDeStack,pcbEjecutando->indiceDeStack->elements_count);
	ultimaPosicionDeMemoriaOcupadaArgs = list_get(ultimoRegistro->args,ultimoRegistro->args->elements_count);
	ultimaPosicionDeMemoriaOcupadaVars = list_get(ultimoRegistro->vars,ultimoRegistro->vars->elements_count);

	if(ultimaPosicionDeMemoriaOcupadaArgs->pag > ultimaPosicionDeMemoriaOcupadaVars->direccionValorDeVariable->pag){
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


// TODO Tener en cuenta en donde se hacen los free
