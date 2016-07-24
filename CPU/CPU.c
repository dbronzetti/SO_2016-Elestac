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
	assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS

	//ERROR if not log parameter was passed
	assert(("ERROR - NOT log file was passed as argument", logFile != NULL));//Verifies if was passed the log file as parameter, if DONT FAILS

	//get configuration from file
	crearArchivoDeConfiguracion(configurationFile);

	//Creo Archivo de Log
	logCPU = log_create(logFile,"CPU",0,LOG_LEVEL_TRACE);


	exitCode = connectTo(UMC,&socketUMC);
	if(exitCode == EXIT_SUCCESS){
		log_info(logCPU,"CPU connected to UMC successfully");
	}


	exitCode = connectTo(NUCLEO,&socketNucleo);

	PCBRecibido = malloc(sizeof(t_PCB));
	while ((exitCode == EXIT_SUCCESS) && !(SignalActivated)){//No wait for more messages if signal was activated during processing

		if(exitCode == EXIT_SUCCESS){
			log_info(logCPU,"CPU connected to NUCLEO successfully");
			messageRcv = malloc(sizeof(int));//for receiving message size
			waitRequestFromNucleo(&socketNucleo, &messageRcv);
		}

		//exitCode=receiveMessage(&socketNucleo,PCBrecibido,sizeof(t_PCB));

		if(messageRcv !=  NULL){
			int messageSize = 0;
			int receivedBytes = 0;
			//deserealizar y construir PCB
			t_MessageNucleo_CPU* messageWithBasicPCB = malloc(sizeof(t_MessageNucleo_CPU));

			deserializeCPU_Nucleo(messageWithBasicPCB, messageRcv);

			log_info(logCPU,"Construyendo PCB para PID #%d",messageWithBasicPCB->processID);
			//TODO verificar que se este recibiendo lo mismo que se envia desde el proceso NUCLEO
			PCBRecibido->PID = messageWithBasicPCB->processID;
			PCBRecibido->ProgramCounter = messageWithBasicPCB->programCounter;
			PCBRecibido->StackPointer = messageWithBasicPCB->stackPointer;
			PCBRecibido->cantidadDePaginas = messageWithBasicPCB->cantidadDePaginas;
			PCBRecibido->indiceDeCodigo = list_create();
			PCBRecibido->indiceDeStack = list_create();
			QUANTUM = messageWithBasicPCB->quantum;
			QUANTUM_SLEEP = messageWithBasicPCB->quantum_sleep;
			ultimoPosicionPC = PCBRecibido->ProgramCounter;

			//receive tamaño de lista indice codigo
			messageRcv = realloc(messageRcv, sizeof(messageSize));
			receivedBytes = receiveMessage(&socketNucleo, &messageSize, sizeof(messageSize));

			//receive lista indice codigo
			messageRcv = realloc(messageRcv, messageSize);
			receivedBytes = receiveMessage(&socketNucleo, messageRcv, messageSize);

			//deserializar estructuras del indice de codigo
			deserializarListaIndiceDeCodigo(PCBRecibido->indiceDeCodigo, messageRcv);

			log_info(logCPU,"Tamanio indice Codigo %d - Cantidad de elementos indice de codigo %d - Proceso '%d'",messageSize, PCBRecibido->indiceDeCodigo->elements_count,PCBRecibido->PID);

			//receive tamaño de lista indice stack
			messageRcv = realloc(messageRcv, sizeof(messageSize));
			messageSize = -1;//Reseting message size before receiving the new one
			receivedBytes = receiveMessage(&socketNucleo, &messageSize, sizeof(messageSize));

			if (messageSize > 0 ){
				//receive lista indice stack
				messageRcv = realloc(messageRcv, messageSize);
				receivedBytes = receiveMessage(&socketNucleo, messageRcv, messageSize);

				//deserializar estructuras del stack
				deserializarListaStack(PCBRecibido->indiceDeStack, messageRcv);
			}

			log_info(logCPU,"Tamanio indice stack %d - Proceso '%d'", messageSize, PCBRecibido->PID);

			//receive tamaño de lista indice etiquetas
			messageRcv = realloc(messageRcv, sizeof(messageSize));
			messageSize = -1;//Reseting message size before receiving the new one
			receivedBytes = receiveMessage(&socketNucleo, &messageSize, sizeof(messageSize));

			PCBRecibido->indiceDeEtiquetasTamanio = messageSize;

			if (PCBRecibido->indiceDeEtiquetasTamanio > 0){
				//receive indice de etiquetas if > 0
				messageRcv = realloc(messageRcv, PCBRecibido->indiceDeEtiquetasTamanio);
				receivedBytes = receiveMessage(&socketNucleo, messageRcv, PCBRecibido->indiceDeEtiquetasTamanio);

				PCBRecibido->indiceDeEtiquetas = malloc(PCBRecibido->indiceDeEtiquetasTamanio);
				memcpy(PCBRecibido->indiceDeEtiquetas, messageRcv, PCBRecibido->indiceDeEtiquetasTamanio);

				//Create list for IndiceDeEtiquetas
				deserializarListaIndiceDeEtiquetas(PCBRecibido->indiceDeEtiquetas, PCBRecibido->indiceDeEtiquetasTamanio);
			}else{
				PCBRecibido->indiceDeEtiquetas = string_new();//initializing indice etiquetas if size is 0
			}

			log_info(logCPU,"Tamanio indice de Etiquetas %d - Proceso '%d'", messageSize, PCBRecibido->PID);
			log_info(logCPU,"El PCB del proceso '%d' fue recibido correctamente", PCBRecibido->PID);

			int j = 0;
			while (j < QUANTUM){

				signal(SIGUSR1, sighandler);

				exitCode = ejecutarPrograma();
				j++;// increasing QUANTUM control

				if (exitCode == EXIT_SUCCESS){

					sleep(QUANTUM_SLEEP);

					// 1) check if the isn't the last code line from the program
					if(PCBRecibido->finalizar == 0){

						if((j == QUANTUM) || (SignalActivated)){//check if signal is activated or QUANTUM is over

							int quantumUsed = 0;
							t_MessageCPU_Nucleo* corteQuantum = malloc( sizeof(t_MessageCPU_Nucleo));

							if(SignalActivated){//Si fue captada la señal SIGUSR1 mientras se estaba ejecutando una instruccion se debe finalizar la misma y acto seguido desconectarse del Nucleo
								corteQuantum->operacion = 72;//operacion 72 es por Desconexion del CPU
								quantumUsed = j;
							}else{// When j == QUANTUM
								log_info(logCPU, "Corte por quantum cumplido - Proceso '%d'", PCBRecibido->PID);
								corteQuantum->operacion = 5;//operacion 5 es por quantum
								quantumUsed = QUANTUM;
							}

							corteQuantum->processID = PCBRecibido->PID;

							int payloadSize = sizeof(corteQuantum->operacion) + sizeof(corteQuantum->processID);
							int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

							char* bufferRespuesta = malloc(bufferSize);
							serializeCPU_Nucleo(corteQuantum, bufferRespuesta, payloadSize);
							sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

							//send quantum used by this process
							sendMessage(&socketNucleo, &quantumUsed, sizeof(quantumUsed));

							//Enviar PCB (indiceStack actualizado) solamente
							char* bufferIndiceStack =  malloc(sizeof(PCBRecibido->indiceDeStack->elements_count));
							serializarListaStack(PCBRecibido->indiceDeStack, &bufferIndiceStack);

							//send tamaño de lista indice stack
							int indiceStackSize = strlen(bufferIndiceStack);
							sendMessage(&socketNucleo, &indiceStackSize, sizeof(int));
							//send lista indice stack
							sendMessage(&socketNucleo, bufferIndiceStack, indiceStackSize);

							free(corteQuantum);
							free(bufferIndiceStack);
							free(bufferRespuesta);

							if(SignalActivated){//Si fue captada la señal SIGUSR1 mientras se estaba ejecutando una instruccion se debe finalizar la misma y acto seguido desconectarse del Nucleo
								log_info(logCPU,"Information from PID '%d' sent to Nucleo... Now I'm going down, but....I'LL BE BACK!!!\n", PCBRecibido->PID);
								break;
							}
						}else if (waitSemActivated){

							//Enviar PCB (indiceStack actualizado) solamente al nucleo si el proceso se bloqueo por wait
							char* bufferIndiceStack =  malloc(sizeof(PCBRecibido->indiceDeStack->elements_count));
							serializarListaStack(PCBRecibido->indiceDeStack, &bufferIndiceStack);

							//send tamaño de lista indice stack
							int indiceStackSize = strlen(bufferIndiceStack);
							sendMessage(&socketNucleo, &indiceStackSize, sizeof(int));
							//send lista indice stack
							sendMessage(&socketNucleo, bufferIndiceStack, indiceStackSize);
							free(bufferIndiceStack);
						}

					}else{
						//Program finished by primitive
						break;
					}
				}

			}

			free(messageRcv);
			//Destruir PCBRecibido
			destruirPCB(PCBRecibido);

			//Destruir listaIndiceEtiquetas
			destroyIndiceEtiquetas();
		}

	}//llave agregada, faltaba para cerrar el main
	free(PCBRecibido);
	return EXIT_SUCCESS;
}

int ejecutarPrograma(){
	int exitCode = EXIT_SUCCESS;
	t_registroIndiceCodigo* instruccionActual = malloc(sizeof(t_registroIndiceCodigo));

	instruccionActual = list_get(PCBRecibido->indiceDeCodigo, ultimoPosicionPC);

	int i;
	int offsetInstruccionesSize = 0;
	t_registroIndiceCodigo* instruccion = malloc(sizeof(t_registroIndiceCodigo));

	//For getting the correct page in which the current instruction has to be located we need the offset from all the previous lines executed
	//NOTE: we follow the code line because doesn't matter if there was a function call (this is because after a function call all the variables in there are deleted from memory)
	for(i = 0; i < instruccionActual->inicioDeInstruccion; i++){
		instruccion = list_get(PCBRecibido->indiceDeCodigo, i);
		offsetInstruccionesSize += instruccion->longitudInstruccionEnBytes;
	}

	free(instruccion);

	char* codigoRecibido = string_new(); //malloc(instruccionActual->longitudInstruccionEnBytes);

	log_info(logCPU,"Contexto de ejecucion recibido - Process ID : %d - PC : %d", PCBRecibido->PID, ultimoPosicionPC);

	//ver de pedir varias veces hasta que cumpla la longitud de instrucciones en bytes e ir concatenando el contenido
	int returnCode = EXIT_SUCCESS;//DEFAULT
	int remainingInstruccion = instruccionActual->longitudInstruccionEnBytes;

	while((returnCode == EXIT_SUCCESS) && (remainingInstruccion > 0)){
		//serializar mensaje para UMC y solicitar codigo porcion de codigo
		int bufferSize = 0;
		int payloadSize = 0;

		//overwrite page content in swap (swap out)
		t_MessageCPU_UMC *message = malloc(sizeof(t_MessageCPU_UMC));
		message->virtualAddress = (t_memoryLocation*) malloc(sizeof(t_memoryLocation));
		message->PID = PCBRecibido->PID;
		message->operation = lectura_pagina;
		message->virtualAddress->pag = (int) ceil(((double) offsetInstruccionesSize/ (double) frameSize));
		message->virtualAddress->offset = 0; //Request ALWAYS the start of a page
		message->virtualAddress->size = frameSize; //ALWAYS REQUEST TO UMC the size of a page and when it is received a memcpy with "instruccionActual->longitudInstruccionEnBytes" has to be done

		payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(t_memoryLocation);
		bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

		char *buffer = malloc(bufferSize);

		serializeCPU_UMC(message, buffer, payloadSize);

		free(message->virtualAddress);
		free(message);

		//Send information to UMC - message serialized with virtualAddress information
		sendMessage(&socketUMC, buffer, bufferSize);

		//First answer from UMC is the exit code from the operation
		exitCode = receiveMessage(&socketUMC,&returnCode, sizeof(exitCode));

		if(returnCode == EXIT_FAILURE){

			//Envia aviso que finaliza incorrectamente el proceso a NUCLEO
			log_error(logCPU, "No se pudo obtener la solicitud a ejecutar - Proceso '%d' - Error al finalizar", PCBRecibido->PID);

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

			char *bufferCode = malloc(frameSize);
			//Receiving information from UMC when the operation was successfully accomplished
			exitCode = receiveMessage(&socketUMC, bufferCode, frameSize);

			if(remainingInstruccion >= frameSize){
				//if the remaining size is greater than frame size then we can append the full buffer received from UMC
				string_append(&codigoRecibido, bufferCode);
			}else{
				//if the remaining size is lower than frame size then we have to append the remaining size from the buffer received from UMC
				memcpy(codigoRecibido, bufferCode, remainingInstruccion );
			}

			offsetInstruccionesSize += frameSize; //moving offset for next request
			remainingInstruccion -= frameSize; //reducing the remaining size to be read on next request

			free(bufferCode);

			exitCode = EXIT_SUCCESS;
		}

		free(buffer);
	}

	if(returnCode == EXIT_SUCCESS){
		analizadorLinea(codigoRecibido, &funciones, &funciones_kernel);

		if(!waitSemActivated){//check if the process was blocked by a wait, in that case the program counter doesn't have to be increased
			ultimoPosicionPC++;
		}

		if(functionCall){//Reseting functionCall boolean if analizadorLinea executed a function call
			functionCall = false;
		}
	}

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
		log_info(logCPU,"Process '%s' NOT VALID to be connected by NUCLEO.", getProcessString(processToConnect));
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
						log_info(logCPU, "Connected to UMC - Message: %s",message->message);
						log_info(logCPU,"Receiving frame size");
						//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
						receivedBytes = receiveMessage(socketClient, &frameSize, sizeof(frameSize));

						log_info(logCPU,"Tamanio de pagina: %d",frameSize);
						break;
					}
					case NUCLEO:{
						log_info(logCPU, "Connected to NUCLEO - Message: %s",message->message);
						log_info(logCPU,"Receiving stack size (number of pages for PID stack)");
						//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
						receivedBytes = receiveMessage(socketClient, &stackSize, sizeof(stackSize));
						log_info(logCPU,"Tamanio de stack: %d",stackSize);
						break;
					}
					default:{
						log_error(logCPU,
								"\nHandshake not accepted when tried to connect your '%s' with '%s'\n",
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

void waitRequestFromNucleo(int *socketClient, char **messageRcv){
	//Receive message size
	int messageSize = 0;
	//Get Payload size
	int receivedBytes = receiveMessage(socketClient, &messageSize, sizeof(messageSize));

	if ( receivedBytes > 0 ){

		//Receive process from which the message is going to be interpreted
		enum_processes fromProcess;
		receivedBytes = receiveMessage(socketClient, &fromProcess, sizeof(fromProcess));

		//Receive message using the size read before
		*messageRcv = realloc(*messageRcv,messageSize);
		receivedBytes = receiveMessage(socketClient, *messageRcv, messageSize);

		//TODO ver que hace con messageRcv despues de recibirlo!!
		log_info(logCPU, "Message size received from process '%s' in socket cliente '%d': %d",getProcessString(fromProcess), socketClient, messageSize);

	}else{
		*messageRcv = NULL;
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

		log_info(logCPU,"funcion: '%s'", regIndiceEtiqueta->funcion);

		memcpy(&regIndiceEtiqueta->posicionDeLaEtiqueta, charEtiquetas +offset, sizeof(regIndiceEtiqueta->posicionDeLaEtiqueta));
		offset += sizeof(regIndiceEtiqueta->posicionDeLaEtiqueta);

		log_info(logCPU,"posicionDeLaEtiqueta: %d", regIndiceEtiqueta->posicionDeLaEtiqueta);

		list_add(listaIndiceEtiquetas,(void*)regIndiceEtiqueta);

	}

}

void destruirRegistroIndiceEtiquetas(t_registroIndiceEtiqueta* registroEtiqueta){
	free(registroEtiqueta->funcion);
	free(registroEtiqueta);
}

void destroyIndiceEtiquetas(){
	list_destroy_and_destroy_elements(listaIndiceEtiquetas,(void*)destruirRegistroIndiceEtiquetas);
}

void crearArchivoDeConfiguracion(char *configFile){
	t_config* configurationFile;
	configurationFile = config_create(configFile);
	configuration.ip_Nucleo = config_get_string_value(configurationFile,"IP_NUCLEO");
	configuration.port_Nucleo = config_get_int_value(configurationFile,"PUERTO_NUCLEO");
	configuration.ip_UMC = config_get_string_value(configurationFile,"IP_UMC");
	configuration.port_UMC = config_get_int_value(configurationFile,"PUERTO_UMC");
}

void sighandler(int signum){
	log_info(logCPU,"Caught signal %d, coming out after sending information to Nucleo...", signum);
	//Activating flag for shutting down CPU
	SignalActivated = true;
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

	//Request to nucleo fin programa information by PID
	t_MessageCPU_Nucleo* message = malloc(sizeof(t_MessageCPU_Nucleo));
	message->operacion = 13;
	message->processID = PCBRecibido->PID;

	int payloadSize = sizeof(message->operacion) + sizeof(message->processID);
	int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char* bufferRespuesta = malloc(bufferSize);
	serializeCPU_Nucleo(message, bufferRespuesta, payloadSize);
	sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

	free(bufferRespuesta);

	//Received fin programa information from Nucleo
	int finDePrograma = -1;
	receiveMessage(&socketNucleo, &finDePrograma, sizeof(finDePrograma));

	//ANALIZA SI ES EL FINAL DEL PROGRAMA
	if(finDePrograma == ultimoPosicionPC){
		//Fin Programa main
		log_info(logCPU, "Proceso '%d' - Finalizado correctamente", PCBRecibido->PID);

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

		//This is for breaking the loop from QUANTUM
		PCBRecibido->finalizar = 1;

	}else{
		//RETORNAR A PUNTO PREVIO DE PROGRAM COUNTER Y ELIMINAR REGISTRO DE STACK CORRESPONDIENTE A LA FUNCION (BUSCAR EL REGISTRO MAS RECIENTE QUE TENGA RETPOS)

		t_registroStack* registroARegresar = NULL;

		getReturnStackRegister(registroARegresar);

		int i;
		int elementsToRemove = PCBRecibido->indiceDeStack->elements_count - registroARegresar->pos; //Calculating how many elements need to be deleted

		// destroy elements from registroARegresar (inclusive) to last stack position
		for (i = 0; i < elementsToRemove ; i++ ){
			//Always remove last stack element
			//TODO Ver si es necesario enviar a la UMC que pise estas paginas que se estan borrando
			int lastListElementIndex = PCBRecibido->indiceDeStack->elements_count - 1;
			list_remove_and_destroy_element(PCBRecibido->indiceDeStack, lastListElementIndex, (void*) destruirRegistroIndiceDeStack);
		}

	}

}


void getReturnStackRegister(t_registroStack* registroARegresar){

	bool getStackForReturning(t_registroStack* unRegistro){
		//look for the registers to be
		return (unRegistro->retVar != NULL);
	}

	bool orderLatestStackForReturning(t_registroStack *unRegistro, t_registroStack *siguienteRegistro) {
		return unRegistro->pos > siguienteRegistro->pos;
	}

	//Creating a new list with all stack registers that have retVar values
	t_list * returningList = list_create();
	returningList = list_filter(PCBRecibido->indiceDeStack,(void*)getStackForReturning);

	//Sort the list created from last to first
	//This is for having the nearest stack element for returning to the current executing stack register, this prevents wrong returning if a function calls another function inside
	list_sort(returningList,(void*)orderLatestStackForReturning);

	//get the first element after sorting the list from last to first
	registroARegresar = list_get(returningList, 0);

	//Returning to program counter from function call
	ultimoPosicionPC = registroARegresar->retPos;
	PCBRecibido->StackPointer = registroARegresar->pos;

	//destroy the list created without erasing the elements from the stack
	list_destroy(returningList);
}

t_puntero definirVariable(t_nombre_variable identificador){
	t_puntero posicionDeLaVariable;
	t_memoryLocation* ultimaPosicionOcupada = NULL;
	t_vars* variableAAgregar = malloc(sizeof(t_vars));
	variableAAgregar->direccionValorDeVariable = malloc(sizeof(t_memoryLocation));
	variableAAgregar->identificador = identificador;

	if(!list_is_empty(PCBRecibido->indiceDeStack)){

		ultimaPosicionOcupada = buscarEnElStackPosicionPagina(PCBRecibido);
		cargarValoresNuevaPosicion(ultimaPosicionOcupada, variableAAgregar->direccionValorDeVariable);

		t_registroStack* ultimoRegistro;
		ultimoRegistro = list_get(PCBRecibido->indiceDeStack, PCBRecibido->indiceDeStack->elements_count);

		if(functionCall){

			//adding arguments if is a function call
			list_add(ultimoRegistro->args, (void*)variableAAgregar);

			posicionDeLaVariable= (t_puntero) &variableAAgregar->direccionValorDeVariable;

		}else{
			if (ultimoPosicionPC == PCBRecibido->ProgramCounter){

				//add vars to same list if executing the same line
				list_add(ultimoRegistro->vars, (void*)variableAAgregar);

				posicionDeLaVariable= (t_puntero) &variableAAgregar->direccionValorDeVariable;
			}else{
				//add a new register to Indice Stack if is a different line
				t_registroStack* registroAAgregar = malloc(sizeof(t_registroStack));
				registroAAgregar->retVar = NULL; //Memory position to return BY DEFAULT
				registroAAgregar->retPos = -1; //DEFAULT VALUE
				registroAAgregar->args = list_create();
				registroAAgregar->vars = list_create();
				registroAAgregar->pos = PCBRecibido->indiceDeStack->elements_count - 1;

				//Every time a record to IndiceStack is created have to be loaded all its variables
				list_add(registroAAgregar->vars,(void*)variableAAgregar);

				list_add(PCBRecibido->indiceDeStack,registroAAgregar);

				posicionDeLaVariable= (t_puntero) &variableAAgregar->direccionValorDeVariable;

			}
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

	PCBRecibido->StackPointer = PCBRecibido->indiceDeStack->elements_count -1;

	return posicionDeLaVariable;
}

void cargarValoresNuevaPosicion(t_memoryLocation* ultimaPosicionOcupada, t_memoryLocation* variableAAgregar){

	if (ultimaPosicionOcupada == NULL){
		ultimaPosicionOcupada = malloc(sizeof(ultimaPosicionOcupada));
		//1) TENER EN CUENTA la pagina donde arranca el stack
		int firstStackPage = PCBRecibido->cantidadDePaginas + 1;
		ultimaPosicionOcupada->pag = firstStackPage; //DEFAULT value for first row
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

// BEFORE calling this function must be ensure "!list_is_empty(PCBRecibido->indiceDeStack)"
t_memoryLocation* buscarEnElStackPosicionPagina(t_PCB* pcb){

	t_memoryLocation* ultimaPosicionLlena;
	t_registroStack* ultimoRegistro;

	ultimoRegistro = list_get(pcb->indiceDeStack, pcb->indiceDeStack->elements_count -1 );

	if(ultimoRegistro->pos == 0){//first row in stack
		t_vars* ultimaRegistroVars = NULL;
		ultimaRegistroVars = list_get(ultimoRegistro->vars, ultimoRegistro->vars->elements_count -1);
		ultimaPosicionLlena = ultimaRegistroVars->direccionValorDeVariable;

	}else if(functionCall){// check if definir variable was called after llamarConRetorno
		if (list_is_empty(ultimoRegistro->args)){
			t_vars* ultimaRegistroVars = NULL;
			ultimaRegistroVars = list_get(ultimoRegistro->vars, ultimoRegistro->vars->elements_count -1);
			ultimaPosicionLlena = ultimaRegistroVars->direccionValorDeVariable;
		}else{
			ultimaPosicionLlena = list_get(ultimoRegistro->args, ultimoRegistro->args->elements_count -1);
		}

	}else{
		if (list_is_empty(ultimoRegistro->vars)){
			ultimaPosicionLlena = list_get(ultimoRegistro->args, ultimoRegistro->args->elements_count -1);
		}else{
			t_vars* ultimaRegistroVars = NULL;
			ultimaRegistroVars = list_get(ultimoRegistro->vars, ultimoRegistro->vars->elements_count -1);
			ultimaPosicionLlena = ultimaRegistroVars->direccionValorDeVariable;
		}
	}

	return ultimaPosicionLlena;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){

	bool condicionVariable(t_vars* unaVariable){
		return (unaVariable->identificador==identificador_variable);
	}
	int i;
	t_registroStack* registroBuscado;
	t_vars* posicionBuscada;
	t_puntero posicionEncontrada;
	for(i=0; i < PCBRecibido->indiceDeStack->elements_count; i++){
		registroBuscado = list_get(PCBRecibido->indiceDeStack,i);
		posicionBuscada = list_find(registroBuscado->vars,(void*)condicionVariable);
	}

	//TODO ver que no se use en ningun otro lado en el parser la funcion obtenerPosicionVariable
	PCBRecibido->StackPointer = registroBuscado->pos;

	posicionEncontrada =(int) &posicionBuscada->direccionValorDeVariable;

	return posicionEncontrada;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){
	t_valor_variable varValue;
	int returnCode = EXIT_SUCCESS;//DEFAULT
	int exitCode = EXIT_SUCCESS;
	// inform new program to swap and check if it could write it.
	int bufferSize = 0;
	int payloadSize = 0;

	//overwrite page content in swap (swap out)
	t_MessageCPU_UMC *message = malloc(sizeof(t_MessageCPU_UMC));
	message->virtualAddress = (t_memoryLocation*) malloc(sizeof(t_memoryLocation));
	message->PID = PCBRecibido->PID;
	message->operation = lectura_pagina;
	message->virtualAddress = (t_memoryLocation*) direccion_variable;
	message->virtualAddress->pag += PCBRecibido->cantidadDePaginas;// como voy a leer una variable del stack debo sumarle la cantidad de paginas de codigo.

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(t_memoryLocation);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char *buffer = malloc(bufferSize);

	serializeCPU_UMC(message, buffer, payloadSize);

	free(message->virtualAddress);
	free(message);

	//Send information to UMC - message serialized with virtualAddress information
	sendMessage(&socketUMC, buffer, bufferSize);

	free(buffer);

	//First answer from UMC is the exit code from the operation
	exitCode = receiveMessage(&socketUMC,&returnCode, sizeof(exitCode));

	if(returnCode == EXIT_SUCCESS){
		char* valorRecibido = malloc(sizeof(t_valor_variable));
		//Receiving information from UMC when the operation was successfully accomplished
		exitCode = receiveMessage(&socketUMC, valorRecibido, sizeof(t_valor_variable));

		memcpy(&varValue,valorRecibido,sizeof(t_valor_variable));
		free(valorRecibido);
	}

	return varValue;
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
	message->virtualAddress->pag += PCBRecibido->cantidadDePaginas; // agrego cantidad de paginas del codigo para que me guarde en la pagina de stack que quiero

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(t_memoryLocation);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char *buffer = malloc(bufferSize);

	serializeCPU_UMC(message, buffer, payloadSize);

	//Send information to UMC - message serialized with virtualAddress information
	sendMessage(&socketUMC, buffer, bufferSize);

	//1) First send with size of the message
	int messageSize = sizeof(t_valor_variable);
	sendMessage(&socketUMC, &messageSize , sizeof(t_valor_variable));
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

	int valorDeError;

	valorDeError = sendMessage(&socketNucleo, variable, variableLen);

	if(valorDeError!=-1){
		printf("Los datos se enviaron correctamente");
		char* valorVariableSerializado = malloc(sizeof(t_valor_variable));
		if( receiveMessage(&socketNucleo,valorVariableSerializado,sizeof(t_valor_variable)) != -1){
			memcpy(&valorVariableDeserializado, valorVariableSerializado, sizeof(t_valor_variable));
		}
		free(valorVariableSerializado);
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

	functionCall = true; //activating functionCall for adding arguments to stack

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

	list_add(PCBRecibido->indiceDeStack,registroAAgregar);

	PCBRecibido->StackPointer = PCBRecibido->indiceDeStack->elements_count -1;

	//Cambiar de program counter segun etiqueta --> Se llama directamente a llamarSinRetorno ya que realiza el cambio de etiqueta
	llamarSinRetorno(etiqueta);

}

void retornar(t_valor_variable retorno){

	/*	1) buscar ultimo registro del IndiceDeStack --> DONE IN obtenerPosicionVariable
	 * 	2) buscar la variable del return --> DONE IN obtenerPosicionVariable
	 * 	3) pedir a UMC el contenido de esa variable (lectura con la serializacion adecuada) --> DONE IN dereferenciar
	 * 	4) Buscar registro del stack en donde me diga donde tengo que regresar (para esto podes guiarte con lo que hize en finalizar() ) --> DONE in getReturnStackRegister()
	 * 	5) copiar el contenido de la variable pedida a la UMC dentro de retVar del registro que se obtuvo en 4) -->DONE in primitiva asignar()
	 * 	6) enviar a la UMC que pise el contenido de la posicion de memoria retVar (escritura con la serializacion adecuada) --> DONE in primitiva asignar()
	 */

	t_registroStack* registroARegresar = NULL;

	getReturnStackRegister(registroARegresar);

	asignar((int) &registroARegresar->retVar, retorno);
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

	log_info(logCPU, "Proceso: '%d' en entrada-salida de tiempo: %d", PCBRecibido->PID,tiempo);

	free(bufferRespuesta);
	free(entradaSalida);
	free(bufferDatosES);
	free(datosParaPlanifdeES);

}

void waitPrimitive(t_nombre_semaforo identificador_semaforo){

	//Envia info al proceso a NUCLEO
	t_MessageCPU_Nucleo* respuesta = malloc(sizeof(t_MessageCPU_Nucleo));
	respuesta->operacion = 8;//8 Grabar semaforo (WAIT)
	respuesta->processID = PCBRecibido->PID;

	int payloadSize = sizeof(respuesta->operacion) + sizeof(respuesta->processID);
	int bufferSize = sizeof(bufferSize) + payloadSize ;

	char* bufferRespuesta = malloc(bufferSize);
	serializeCPU_Nucleo(respuesta, bufferRespuesta, payloadSize);

	sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

	free(bufferRespuesta);

	//send to Nucleo to execute WAIT function for "identificador_semaforo"
	int semaforoLen = strlen(identificador_semaforo) + 1;
	int respuestaRecibida;
	char* respuestaNucleo = malloc(sizeof(respuestaRecibida));


	// envio al Nucleo el tamanio del semaforo
	sendMessage(&socketNucleo, &semaforoLen , sizeof(semaforoLen));

	// envio al Nucleo el semaforo
	string_append(&identificador_semaforo, "\0");
	sendMessage(&socketNucleo, identificador_semaforo, semaforoLen);

	// recibir respuesta del Nucleo
	receiveMessage(&socketNucleo,respuestaNucleo,sizeof(respuestaRecibida));
	memcpy(&respuestaRecibida,respuestaNucleo,sizeof(respuestaRecibida));

	free(respuestaNucleo);

	if(respuestaRecibida==1){
		waitSemActivated = true;
	}

}

void signalPrimitive(t_nombre_semaforo identificador_semaforo){

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

void llamarSinRetorno(t_nombre_etiqueta etiqueta){
 	//Cambiar de program counter segun etiqueta
	irAlLabel(etiqueta);
}
