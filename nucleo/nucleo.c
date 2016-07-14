/*
 * nucleo.c
 *
 */
 
 #include "nucleo.h"

int main(int argc, char *argv[]) {
	int exitCode = EXIT_FAILURE; //DEFAULT failure
	char *configurationFile = NULL;
	char *logFile = NULL;
	pthread_t serverThread;
	pthread_t serverConsolaThread;
	pthread_t hiloBloqueados;

/*

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS

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
	}

	//ERROR if not configuration parameter was passed
		assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS

	//ERROR if not Log parameter was passed
		assert(("ERROR - NOT log file was passed as argument", logNucleo != NULL));//Verifies if was passed the Log file as parameter, if DONT FAILS

	//Creo archivo de configuracion
		crearArchivoDeConfiguracion(configurationFile);
		*/
	//Creo el archivo de Log
		logNucleo = log_create(logFile, "NUCLEO", 0, LOG_LEVEL_TRACE);
	//Creo la lista de CPUs
		listaCPU = list_create();
	//Creo la lista de Consolas
		listaConsola = list_create();
	//Creo Lista Procesos
		listaProcesos = list_create();
	//Creo la Cola de Listos
		colaListos = queue_create();
	//Creo la Cola de Bloqueados
		colaBloqueados = queue_create();
	//Creo cola de Procesos a Finalizar (por Finalizar PID).
		colaFinalizar = queue_create();

	pthread_mutex_init(&activeProcessMutex, NULL);

	//Create thread for server start
	pthread_create(&serverThread, NULL, (void*) startServerProg, NULL);
	pthread_create(&serverConsolaThread, NULL, (void*) startServerCPU, NULL);
	//Create thread for blocked processes
	pthread_create(&hiloBloqueados, NULL, (void*)atenderBloqueados, NULL);

	exitCode = connectTo(UMC,&socketUMC);
	if (exitCode == EXIT_SUCCESS) {
		 log_info(logNucleo, "NUCLEO connected to UMC successfully\n");
	}else{
		printf("No server available - shutting down proces!!\n");
		return EXIT_FAILURE;
	}

	pthread_join(serverThread, NULL);
	pthread_join(serverConsolaThread, NULL);
	pthread_join(hiloBloqueados, NULL);

	return exitCode;
}

void startServerProg(){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	t_serverData serverData;

	exitCode = openServerConnection(configNucleo.puerto_prog, &serverData.socketServer);
	log_info(logNucleo, "SocketServer: %d\n",serverData.socketServer);

	//If exitCode == 0 the server connection is opened and listening
	if (exitCode == 0) {
		log_info(logNucleo, "the server is opened\n");

		exitCode = listen(serverData.socketServer, SOMAXCONN);

		if (exitCode < 0 ){
			log_error(logNucleo,"Failed to listen server Port.\n");
			return;
		}

		while (1){
			newClients((void*) &serverData);
		}
	}

}

void startServerCPU(){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	t_serverData serverData;

	exitCode = openServerConnection(configNucleo.puerto_cpu, &serverData.socketServer);
	log_info(logNucleo, "SocketServer: %d\n",serverData.socketServer);

	//If exitCode == 0 the server connection is opened and listening
	if (exitCode == 0) {
		log_info(logNucleo, "the server is opened\n");

		exitCode = listen(serverData.socketServer, SOMAXCONN);

		if (exitCode < 0 ){
			log_error(logNucleo,"Failed to listen server Port.\n");
			return;
		}

		while (1){
			newClients((void*) &serverData);
		}
	}

}

void newClients (void *parameter){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure

	t_serverData *serverData = (t_serverData*) parameter;

	// disparar un thread para acceptar cada cliente nuevo (debido a que el accept es bloqueante) y para hacer el handshake
	/**************************************/
	//Create thread attribute detached
	//			pthread_attr_t acceptClientThreadAttr;
	//			pthread_attr_init(&acceptClientThreadAttr);
	//			pthread_attr_setdetachstate(&acceptClientThreadAttr, PTHREAD_CREATE_DETACHED);
	//
	//			//Create thread for checking new connections in server socket
	//			pthread_t acceptClientThread;
	//			pthread_create(&acceptClientThread, &acceptClientThreadAttr, (void*) acceptClientConnection1, &serverData);
	//
	//			//Destroy thread attribute
	//			pthread_attr_destroy(&acceptClientThreadAttr);
	/************************************/

	exitCode = acceptClientConnection(&serverData->socketServer, &serverData->socketClient);

	if (exitCode == EXIT_FAILURE){
		log_warning(logNucleo,"There was detected an attempt of wrong connection\n");
		close(serverData->socketClient);
	}else{
		//Create thread attribute detached
		pthread_attr_t handShakeThreadAttr;
		pthread_attr_init(&handShakeThreadAttr);
		pthread_attr_setdetachstate(&handShakeThreadAttr, PTHREAD_CREATE_DETACHED);

		//Create thread for checking new connections in server socket
		pthread_t handShakeThread;
		pthread_create(&handShakeThread, &handShakeThreadAttr, (void*) handShake, parameter);

		//Destroy thread attribute
		pthread_attr_destroy(&handShakeThreadAttr);

	}// END handshakes

}

void handShake (void *parameter){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure

	t_serverData *serverData = (t_serverData*) parameter;

	//Receive message size
	int messageSize = 0;
	char *messageRcv = malloc(sizeof(messageSize));
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(messageSize));

	//Receive message using the size read before
	memcpy(&messageSize, messageRcv, sizeof(int));
	messageRcv = realloc(messageRcv,messageSize);
	receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

	//starting handshake with client connected
	t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
	deserializeHandShake(message, messageRcv);

	//Now it's checked that the client is not down
	if ( receivedBytes == 0 ){
		log_error(logNucleo,
				"The client went down while handshaking! - Please check the client '%d' is down!\n",
				serverData->socketClient);
		close(serverData->socketClient);
	}else{
		switch ((int) message->process){
			case CPU:{
				log_info(logNucleo,"Message from '%s': %s\n", getProcessString(message->process), message->message);
				close(serverData->socketClient);
				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){// Si uso connectTo => debo considerar este bloque
					t_datosCPU *datosCPU = malloc(sizeof(t_datosCPU));
					datosCPU->numSocket = serverData->socketClient;
					pthread_mutex_lock(&listadoCPU);
					list_add(listaCPU, (void*)datosCPU);
					pthread_mutex_unlock(&listadoCPU);
				}

				break;
			}
			case CONSOLA:{
			log_info(logNucleo, "Message from '%s': %s\n",
					getProcessString(message->process), message->message);
				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){

					//Create thread attribute detached
					pthread_attr_t processMessageThreadAttr;
					pthread_attr_init(&processMessageThreadAttr);
					pthread_attr_setdetachstate(&processMessageThreadAttr, PTHREAD_CREATE_DETACHED);

					//Create thread for checking new connections in server socket
					pthread_t processMessageThread;
					pthread_create(&processMessageThread, &processMessageThreadAttr, (void*) processMessageReceived, parameter);

					//Destroy thread attribute
					pthread_attr_destroy(&processMessageThreadAttr);
				}

				break;
			}
			default:{
			log_error(logNucleo,
					"Process not allowed to connect - Invalid process '%s' tried to connect to NUCLEO\n",
					getProcessString(message->process));
				close(serverData->socketClient);
				break;
			}
		}
 	}

	free(messageRcv);
	free(message->message);
	free(message);
}

void processMessageReceived (void *parameter){

	t_serverData *serverData = (t_serverData*) parameter;

	//Receive message size
	int messageSize = 0;
	char *messageRcv = malloc(sizeof(messageSize));
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(messageSize));

	if ( receivedBytes > 0 ){

		//Get Payload size
		messageSize = atoi(messageRcv);

		//Receive process from which the message is going to be interpreted
		enum_processes fromProcess;
		messageRcv = realloc(messageRcv, sizeof(fromProcess));
		receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(fromProcess));
		fromProcess = (enum_processes) messageRcv;

		log_info(logNucleo,"Bytes received from process '%s': %d\n",getProcessString(fromProcess),receivedBytes);

		switch (fromProcess){
			case CONSOLA:{	//TODO en ningun momento la consola me esta enviando todoo lo anterior
				log_info(logNucleo, "Processing CONSOLA message received\n");
				pthread_mutex_lock(&activeProcessMutex);

				/*//Receive message using the size read before
				messageRcv = realloc(messageRcv, messageSize);
				receiveMessage(&serverData->socketClient, messageRcv, messageSize);

				char *message = malloc(sizeof(messageSize));*/

				int* tamanio = malloc(sizeof(int));
				receiveMessage(&serverData->socketClient, tamanio, sizeof(int));
				int opFinalizar = *tamanio;

				int PID = buscarPIDConsola(serverData->socketClient);
				if (PID==-1){

					printf("No se encontro Consola para el socket: %d \n",serverData->socketClient);

				}else if (opFinalizar == -1) { 	//Finaliza Proceso

					finalizarPid(PID);

					return;
				}

				//Recibo el codigo del programa
				char* codeScript = malloc(*tamanio);
				receiveMessage(&serverData->socketClient, codeScript, *tamanio);

				iniciarPrograma(PID, codeScript);
				runScript(codeScript,socketConsola);

				pthread_mutex_unlock(&activeProcessMutex);
			break;
			}
			case CPU:{
				log_info(logNucleo, "Processing CPU message received\n");
				pthread_mutex_lock(&activeProcessMutex);
				processCPUMessages(messageRcv, messageSize, serverData->socketClient);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			default:{
			log_error(logNucleo,
					"Process not allowed to connect - Invalid process '%s' send a message to UMC\n",
					getProcessString(fromProcess));
			close(serverData->socketClient);
				break;
			}
		}

	}else if (receivedBytes == 0 ){
		//The client is down when bytes received are 0
		log_error(logNucleo,
				"The client went down while receiving! - Please check the client '%d' is down!\n",
				serverData->socketClient);
		close(serverData->socketClient);
	}else{
		log_error(logNucleo,
				"Error - No able to received - Error receiving from socket '%d', with error: %d\n",
				serverData->socketClient, errno);
		close(serverData->socketClient);
	}

	free(messageRcv);

}

void runScript(char* codeScript, int socketConsola){
	//Creo el PCB del proceso.
	t_PCB* PCB = malloc(sizeof(t_PCB));
	t_proceso* datosProceso = malloc(sizeof(t_proceso));
	//Armo Indice de codigo y etiquetas
	t_metadata_program *miMetaData = metadata_desde_literal(codeScript);

	PCB->PID = idProcesos;
	PCB->ProgramCounter = miMetaData->instruccion_inicio;
	//En el caso de usar la siguiente linea: VER por que tira error si esta incluida math.h (en nucleo.h)
	//PCB->cantidadDePaginas = ceil((double) (strlen(codeScript) + 1)/ (double) frameSize);
	PCB->cantidadDePaginas = (strlen(codeScript) + 1) / frameSize;
	PCB->StackPointer = 0;
	PCB->estado = 1;
	PCB->finalizar = 0;
	armarIndiceDeCodigo(*PCB, miMetaData);
	armarIndiceDeEtiquetas(*PCB, miMetaData);
	PCB->indiceDeStack = list_create();

	idProcesos++;

	pthread_mutex_lock(&listadoProcesos);
	list_add(listaProcesos,(void*)PCB);
	pthread_mutex_unlock(&listadoProcesos);
	log_info(logNucleo, "myProcess %d - Iniciado  Script: %s",PCB->PID, codeScript);

	//Agrego a la Cola de Listos
	datosProceso->ProgramCounter = PCB->ProgramCounter;
	datosProceso->PID = PCB->PID;

	pthread_mutex_lock(&cListos);
	queue_push(colaListos, (void*)datosProceso);
	pthread_mutex_unlock(&cListos);

	//Agrego a la lista de Consolas
	t_datosConsola* datosConsola = malloc(sizeof(t_datosConsola));
	datosConsola->numSocket = socketConsola;
	datosConsola->PID = PCB->PID;
	pthread_mutex_lock(&listadoConsola);
	list_add(listaConsola, (void*) datosConsola);
	pthread_mutex_unlock(&listadoConsola);

	planificarProceso();

	metadata_destruir(miMetaData);

}

void planificarProceso() {
	//Veo si hay procesos para planificar en la cola de Listos y en la cola Finalizar
	if (queue_is_empty(colaListos) && (queue_is_empty(colaFinalizar))) {
		return;
	}
	//Veo si hay CPU libre para asignarle
	int libreCPU = buscarCPULibre();

	if (libreCPU == -1) {
		printf("No hay CPU libre.\n");
		return;
	}
	t_serverData *serverData;
	serverData->socketClient = libreCPU;

	//Le envio la informacion a la CPU
	t_PCB* datosPCB;
	t_MessageNucleo_CPU* contextoProceso = malloc(sizeof(t_MessageNucleo_CPU));
	t_proceso* datosProceso;

	contextoProceso->operacion = 0; //No finaliza el proceso
	contextoProceso->quantum = 0;
	contextoProceso->quantum_sleep=0;

	if (queue_is_empty(colaFinalizar)) {

		pthread_mutex_lock(&cListos);
		datosProceso = (t_proceso*) queue_peek(colaListos);
		pthread_mutex_unlock(&cListos);

		int posicion = buscarPCB(datosProceso->PID);
		if (posicion != -1) {
			pthread_mutex_lock(&listadoProcesos);
			datosPCB = (t_PCB*) list_get(listaProcesos, posicion);
			pthread_mutex_unlock(&listadoProcesos);

			contextoProceso->quantum = configNucleo.quantum;
			contextoProceso->quantum_sleep=configNucleo.quantum_sleep;

			enviarMsjCPU(datosPCB, contextoProceso, serverData);

		} else {
			printf("Proceso no encontrado en la lista.\n");
		}
	} else {
		pthread_mutex_lock(&cFinalizar);
		datosProceso = (t_proceso*) queue_peek(colaFinalizar);
		pthread_mutex_unlock(&cFinalizar);
		int posicion = buscarPCB(datosProceso->PID);
		if (posicion != -1) {
			pthread_mutex_lock(&listadoProcesos);
			datosPCB = (t_PCB*) list_get(listaProcesos, posicion);
			pthread_mutex_unlock(&listadoProcesos);

// TODO Ver esto en la CPU: Le aviso al CPU que finalice el proceso (op = 1) y luego voy a esperar la Respuesta(processCPUMessages)
			contextoProceso->operacion = 1;
			enviarMsjCPU(datosPCB, contextoProceso, serverData);

		} else {
			printf("Proceso no encontrado en la lista.\n");
		}
	}
	//free(contextoProceso);
}

void enviarMsjCPU(t_PCB* datosPCB,t_MessageNucleo_CPU* contextoProceso, t_serverData* serverData){

		contextoProceso->programCounter = datosPCB->ProgramCounter;
		contextoProceso->processID = datosPCB->PID;
		contextoProceso->stackPointer = datosPCB->StackPointer;
		contextoProceso->cantidadDePaginas = datosPCB->cantidadDePaginas;
		strcpy(contextoProceso->indiceDeEtiquetas, datosPCB->indiceDeEtiquetas);
		contextoProceso->indiceDeEtiquetasTamanio = strlen(datosPCB->indiceDeEtiquetas) + 1;

		int payloadSize = sizeof(contextoProceso->programCounter) + (sizeof(contextoProceso->processID))
			+ sizeof(contextoProceso->stackPointer)+ sizeof(contextoProceso->cantidadDePaginas) + sizeof(contextoProceso->operacion)
			+ sizeof(contextoProceso->quantum) + sizeof(contextoProceso->quantum_sleep)
			+ sizeof(contextoProceso->indiceDeEtiquetasTamanio) + strlen(datosPCB->indiceDeEtiquetas) + 1;// +1 because '\0'

		int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

		char* bufferEnviar = malloc(bufferSize);
		//Serializar mensaje basico del PCB
		serializeNucleo_CPU(contextoProceso, bufferEnviar, payloadSize);

		//Hacer send al CPU de lo que se serializo arriba
		sendMessage(&serverData->socketClient, bufferEnviar, bufferSize);

		//serializar estructuras del indice de codigo
		char* bufferIndiceCodigo =  malloc(sizeof(datosPCB->indiceDeCodigo->elements_count));
		serializarListaIndiceDeCodigo(datosPCB->indiceDeCodigo, bufferIndiceCodigo);

		//send tamaño de lista indice codigo
		sendMessage(&serverData->socketClient, (void*) strlen(bufferIndiceCodigo), sizeof(int));
		//send lista indice codigo
		sendMessage(&serverData->socketClient, bufferIndiceCodigo, strlen(bufferIndiceCodigo));

		free(bufferIndiceCodigo);

		//serializar estructuras del stack
		char* bufferIndiceStack =  malloc(sizeof(datosPCB->indiceDeStack->elements_count));
		serializarListaStack(datosPCB->indiceDeStack, bufferIndiceStack);

		//send tamaño de lista indice stack
		sendMessage(&serverData->socketClient, (void*) strlen(bufferIndiceStack), sizeof(int));
		//send lista indice stack
		sendMessage(&serverData->socketClient, bufferIndiceStack, strlen(bufferIndiceStack));

		free(bufferIndiceStack);

		//Saco el primer elemento de la cola, porque ya lo planifique.
		pthread_mutex_lock(&cListos);
		t_proceso* proceso = queue_pop(colaListos);
		pthread_mutex_unlock(&cListos);

		//Cambio Estado del Proceso
		int estado = 2;
		cambiarEstadoProceso(datosPCB->PID, estado);

		//1) rcv();

		processMessageReceived(&serverData);

		//2) processCPUMessages se hace dentro de processMessageReceived (case CPU)

		free(bufferEnviar);
		free(proceso);
}

void processCPUMessages(char* messageRcv,int messageSize,int socketLibre){
	printf("Processing CPU message \n");

	t_MessageCPU_Nucleo *message=malloc(sizeof(t_MessageCPU_Nucleo));

	messageRcv = realloc(messageRcv, messageSize);
	receiveMessage(&socketLibre,(void*)messageRcv, messageSize);

	//Deserializo messageRcv
	deserializeNucleo_CPU(message, messageRcv);

	switch (message->operacion) {
	case 1:{ 	//Entrada Salida
		t_es infoES;
		char *datosEntradaSalida = malloc(sizeof(t_es));
		//change active PID
		activePID = message->processID;

		receiveMessage(&socketLibre, (void*) datosEntradaSalida, sizeof(t_es));
		deserializarES(&infoES, datosEntradaSalida);

		//Libero la CPU que ocupaba el proceso
		liberarCPU(socketLibre);

		//Cambio el PC del Proceso
		actualizarPC(message->processID, infoES.ProgramCounter);

		//Cambio el estado del proceso
		int estado = 3;
		cambiarEstadoProceso(message->processID, estado);

		EntradaSalida(infoES.dispositivo, infoES.tiempo);
		free(datosEntradaSalida);
		break;}
	case 2:{ 	//Finaliza Proceso Bien
		finalizaProceso(socketLibre, message->processID,message->operacion);
		break;}
	case 3:{ 	//Finaliza Proceso Mal
		finalizaProceso(socketLibre, message->processID,message->operacion);
		break;}
	case 4:{
		printf("No se pudo obtener la solicitud a ejecutar - Error al finalizar");
		break;}
	case 5:{ 	//Corte por Quantum
		printf("Corto por Quantum.\n");
		atenderCorteQuantum(socketLibre, message->processID);
		break;}
	//TODO CHEQUEAR QUE DE ACA HASTA EL FINAL DEL SWITCH (EN CADA CASE) SEA CORRECTO EL MANEJO DE LOS MENSAJES
	case 6:{	//Obtener valor y enviarlo al CPU

		// 1) Recibir tamanio de la variable
		int* variableLen = malloc(sizeof(int));
		receiveMessage(&socketLibre, variableLen, sizeof(int));

		// 2) Recibir la variable
		t_nombre_compartida variable = malloc(*variableLen);
		receiveMessage(&socketLibre, variable, *variableLen);

		t_valor_variable* valorVariable = obtenerValor(variable);

		if (valorVariable == NULL){
			printf("No encontre variable %s %d id \n",variable, *variableLen);
		}

		sendMessage(&socketLibre, valorVariable, sizeof(t_valor_variable));
		//TODO verificar que esten bien estos free
		free(variableLen);
		free(valorVariable);
		break;}
	case 7:{	//Grabar valor

		// 1) Recibir valor
		t_valor_variable* valor = malloc(sizeof(t_valor_variable));
		receiveMessage(&socketLibre, valor, sizeof(t_valor_variable));

		// 2) Recibir tamanio de la variable
		int* variableLen = malloc(sizeof(int));
		receiveMessage(&socketLibre, variableLen, sizeof(int));

		// 3) Recibir la variable
		t_nombre_compartida variable = malloc(*variableLen);
		receiveMessage(&socketLibre, variable, *variableLen);

		grabarValor(variable, valor);

		//TODO verificar que esten bien estos free, valor no se hace el free porque se asigna adentro de grabarValor
		free(variableLen);
		free(variable);

		break;}
	case 8:{	// WAIT - Grabar semaforo y enviar al CPU

		//Recibo el tamanio del wait
		int* tamanio = malloc(sizeof(int));
		receiveMessage(&socketLibre, tamanio, sizeof(int));

		//Recibo el semaforo wait
		t_nombre_semaforo semaforo = malloc(*tamanio);
		receiveMessage(&socketLibre, semaforo, *tamanio);

		if (estaEjecutando(message->processID)==1){ // 1: Programa ejecutandose (no esta en ninguna cola)
			int * valorSemaforo = pideSemaforo(semaforo);
			int valorAEnviar;
			if(*valorSemaforo<=0){

				valorAEnviar = 1;
				printf("Recibi proceso %d mando a bloquear por semaforo \n", (message->processID)%6+1);
				sendMessage(&socketLibre, &valorAEnviar,sizeof(int));// 1 si se bloquea. 0 si no se bloquea
				bloqueoSemaforo(message->processID,semaforo);
				//Libero la CPU que ocupaba el proceso
				liberarCPU(socketLibre);
			}else{
				grabarSemaforo(semaforo,*pideSemaforo(semaforo)-1);
				valorAEnviar=0;
				sendMessage(&socketLibre, &valorAEnviar, sizeof(int));
			}
		}
		//TODO verificar que esten bien estos free
		free(tamanio);
		free(semaforo);
		break;}
	case 9:{	// SIGNAL	- 	Libera semaforo

		//Recibo el tamanio del signal
		int* tamanio = malloc(sizeof(int));
		receiveMessage(&socketLibre, tamanio, sizeof(int));

		//Recibo el semaforo wait
		t_nombre_semaforo semaforo = malloc(*tamanio);
		receiveMessage(&socketLibre, semaforo, *tamanio);

		liberaSemaforo(semaforo);
		//TODO verificar que esten bien estos free
		free(tamanio);
		free(semaforo);
		break;}
	case 10:{	//Imprimir VALOR por Consola
		int socketConsola = buscarSocketConsola(message->processID);

		if (socketConsola==-1){
			printf("No se encontro Consola para el PID: %d \n",message->processID);
			return;
		}

		//Received value from CPU
		t_valor_variable* valor = malloc(sizeof(t_valor_variable));
		receiveMessage(&socketLibre, valor, sizeof(t_valor_variable));

		//Enviar operacion=2 para que la Consola sepa que es un valor
		int operacion = 2;
		sendMessage(&socketConsola, &operacion, sizeof(int));

		log_info(logNucleo, "Enviar valor '%s' a PID #%d\n", valor, message->processID);
		sendMessage(&socketConsola, valor, sizeof(t_valor_variable));

		free(valor);
		break;
	}
	case 11:{	//Imprimir TEXTO por Consola
		int socketConsola = buscarSocketConsola(message->processID);
		if (socketConsola==-1){
			printf("No se encontro Consola para el PID: %d \n",message->processID);
			return;
		}

		int* tamanio = malloc(sizeof(int));
		char* texto = malloc(*tamanio);

		//Recibo el tamanio del texto
		receiveMessage(&socketLibre, tamanio,sizeof(int));

		//Recibo el texto
		receiveMessage(&socketLibre, texto, *tamanio);

		//Enviar operacion=1 para que la Consola sepa que es un un texto
		int operacion = 1;
		sendMessage(&socketConsola, &operacion, sizeof(int));

		// Envia el tamanio del texto al proceso CONSOLA
		log_info(logNucleo, "Enviar tamanio: '%d', a PID #%d\n", *tamanio, message->processID);
		string_append(&texto,"\0");
		sendMessage(&socketConsola, tamanio, sizeof(int));

		// Envia el texto al proceso CONSOLA
		log_info(logNucleo, "Enviar texto: '%s', a PID #%d\n", texto, message->processID);
		sendMessage(&socketConsola, texto, *tamanio);

		free(tamanio);
		free(texto);
		break;}
	default:
		printf("Mensaje recibido invalido. \n");
		//printf("CPU desconectado.\n");
		//abort();
	}

	free(message);
}

void atenderCorteQuantum(int socket,int PID){
	//Libero la CPU
	liberarCPU(socket);

	//Cambio el PC del Proceso, le sumo el quantum al PC actual.
	t_PCB* infoProceso;
	int buscar = buscarPCB(PID);
	pthread_mutex_lock(&listadoProcesos);
	infoProceso = (t_PCB*)list_get(listaProcesos,buscar);
	pthread_mutex_unlock(&listadoProcesos);
	int pcnuevo;
	pcnuevo = infoProceso->ProgramCounter + configNucleo.quantum;
	infoProceso->ProgramCounter =  pcnuevo;

	//TODO RECIBIR PCB MODIFICADO DEL CPU! (lo que hace falta en realidad es el stack fundamentalmente y ver si es necesario algo mas que haya modificado el CPU)
	/*
	 * 1) receive tamanioBuffer
	 * 2) receive buffer segun tamanioBuffer
	 * 3) malloc de listaARecibir segundo el tamanio recibido.
	 * 4) deserializarListaStack(listaARecibir, buffer);
	 * 5) borrar lista en infoProceso->indiceDeStack OJO se tiene que crear el elementDestroyer para cada registro del indiceDeStack (LA MISMA QUE SE DEBERIA USAR PARA ELIMINAR UN PCB)
	 * 6) infoProceso->indiceDeStack = listaARecibir;
	 */

	//Agrego el proceso a la Cola de Listos
	t_proceso* datosProceso = malloc(sizeof(t_proceso));
	datosProceso->PID = PID;
	datosProceso->ProgramCounter = pcnuevo;
	if (infoProceso->finalizar == 0){

		//Cambio el estado del proceso
		int estado = 1;
		cambiarEstadoProceso(PID, estado);
		pthread_mutex_lock(&cListos);
		queue_push(colaListos,(void*)datosProceso);
		pthread_mutex_unlock(&cListos);
	} else {
		pthread_mutex_lock(&cFinalizar);
		queue_push(colaFinalizar, (void*) datosProceso);
		pthread_mutex_unlock(&cFinalizar);
	}

	//Mando nuevamente a PLanificar
	planificarProceso();

}

//Cada vez que se finaliza proceso se debe avisar a la consola ascociada
void finalizaProceso(int socket, int PID, int estado) {

	int posicion = buscarPCB(PID);
	int estadoProceso;
	//Cambio el estado del proceso, y guardo en Log que finalizo correctamente
	t_PCB* datosProceso;
	pthread_mutex_lock(&listadoProcesos);
	datosProceso = (t_PCB*) list_get(listaProcesos, posicion);
	pthread_mutex_unlock(&listadoProcesos);
	if (estado == 2) {
		estadoProceso = 4;
		cambiarEstadoProceso(PID, estadoProceso);
		log_info(logNucleo, "myProcess %d - Finalizo correctamente",datosProceso->PID);
	} else {
		if (estado == 3) {
			estadoProceso = 5;
			cambiarEstadoProceso(PID, estadoProceso);
			log_info(logNucleo, "myProcess %d - Finalizo incorrectamente por falta de espacio en Swap", datosProceso->PID);
		}
	}

	//Libero la CPU
	liberarCPU(socket);

	finalizarPrograma(PID);

	int socketConsola = buscarSocketConsola(PID);

	//Enviar operacion=1 para que la Consola sepa que es un un texto
	int operacion = 1;
	sendMessage(&socketConsola, &operacion, sizeof(int));

	// Envia el tamanio del texto y luego el texto al proceso Consola
	char texto[] = "Se finaliza el proceso por peticion de la Consola";
	int textoLen = strlen(texto)+1;
	sendMessage(&socketConsola, &textoLen, sizeof(textoLen));

	log_info(logNucleo, "Enviar texto: '%s', a PID #%d\n", texto, PID);
	sendMessage(&socketConsola, texto, textoLen);
	//TODO Destruir PCB

	//Mando a revisar si hay alguno en la lista para ejecutar.
	planificarProceso();

}

int buscarCPULibre() {
	int cantCPU, i=0;
	t_datosCPU* datosCPU;
	cantCPU = list_size(listaCPU);
	for(i=0;i<cantCPU;i++){
		pthread_mutex_lock(&listadoCPU);
		datosCPU = (t_datosCPU*) list_get(listaCPU, 0);
		pthread_mutex_unlock(&listadoCPU);

		if (datosCPU->estadoCPU == 0){
			datosCPU->estadoCPU = 1;
		return datosCPU->numSocket;
		}
	}
	return -1;
}

int buscarPCB(int pid) {
	t_PCB* datosPCB;
	int i = 0;
	int cantPCB = list_size(listaProcesos);
	for (i = 0; i < cantPCB; i++) {
		pthread_mutex_lock(&listadoProcesos);
		datosPCB = list_get(listaProcesos, i);
		pthread_mutex_unlock(&listadoProcesos);
		if (datosPCB->PID == pid) {
			return i;
		}
	}
	return -1;
}

int buscarCPU(int socket) {
	t_datosCPU* datosCPU;
	int i = 0;
	int cantCPU = list_size(listaCPU);
	for (i = 0; i < cantCPU; i++) {
		pthread_mutex_lock(&listadoCPU);
		datosCPU = list_get(listaCPU, i);
		pthread_mutex_unlock(&listadoCPU);
		if (datosCPU->numSocket == socket) {
			return i;
		}
	}
	return -1;
}

int buscarPIDConsola(int socket) {
	t_datosConsola* datosConsola;
	int i = 0;
	int cantConsolas = list_size(listaConsola);
	for (i = 0; i < cantConsolas; i++) {
		pthread_mutex_lock(&listadoConsola);
		datosConsola = list_get(listaConsola, i);
		pthread_mutex_unlock(&listadoConsola);
		if (datosConsola->numSocket == socket) {
			return datosConsola->PID;
		}
	}
	return -1;
}

int buscarSocketConsola(int PID){
	t_datosConsola* datosConsola;
	int i = 0;
	int cantConsolas = list_size(listaConsola);
	for (i = 0; i < cantConsolas; i++) {
		pthread_mutex_lock(&listadoConsola);
		datosConsola = list_get(listaConsola, i);
		pthread_mutex_unlock(&listadoConsola);
		if (datosConsola->PID == PID) {
			return datosConsola->numSocket;
		}
	}
	return -1;
}

int estaEjecutando(int PID){
	int i;
	t_proceso* proceso;
	int cantElemBloq = queue_size(colaBloqueados);
	int cantElemFin = queue_size(colaFinalizar);
	int cantElemListos = queue_size(colaListos);
	for (i = 0; i < cantElemBloq; i++) {

		pthread_mutex_lock(&cBloqueados);
		proceso = queue_peek(colaBloqueados);
		pthread_mutex_unlock(&cBloqueados);
		if(proceso->PID == PID){
			return -1;
		}
	}
	for (i = 0; i < cantElemFin; i++) {

		pthread_mutex_lock(&cBloqueados);
		proceso = queue_peek(colaFinalizar);
		pthread_mutex_unlock(&cBloqueados);
		if (proceso->PID == PID){
			return -1;
		}
	}
	for (i = 0; i < cantElemListos; i++) {

		pthread_mutex_lock(&cBloqueados);
		proceso = queue_peek(colaFinalizar);
		pthread_mutex_unlock(&cBloqueados);
		if (proceso->PID == PID){
			return -1;
		}
	}
	return 1;
}

void EntradaSalida(t_nombre_dispositivo dispositivo, int tiempo){
	int i = 0;
	t_bloqueado* infoBloqueado = malloc(sizeof(t_bloqueado));

	while (configNucleo.io_ids[i] != NULL){
		if (strcmp(configNucleo.io_ids[i], dispositivo) == 0) {
			// ojo con el \n
			infoBloqueado->PID = activePID;
			infoBloqueado->dispositivo = dispositivo;
			infoBloqueado->tiempo = tiempo;
		}
	}
	//Agrego a la cola de Bloqueados, y seteo el semaforo
	if (queue_size(colaBloqueados) == 0) {

		pthread_mutex_lock(&cBloqueados);
		queue_push(colaBloqueados, (void*) infoBloqueado);
		pthread_mutex_unlock(&cBloqueados);

		makeTimer(timers[i], configNucleo.io_ids_values[i] * tiempo); //2ms
		sem_post(&semBloqueados);

		return;
	}else{
		pthread_mutex_lock(&cBloqueados);
		queue_push(colaBloqueados, (void*) infoBloqueado);
		pthread_mutex_unlock(&cBloqueados);
		sem_post(&semBloqueados);

		return;
	}
}

void analizarIO(int sig, siginfo_t *si, void *uc) {
	int i=0, io;
	while (configNucleo.io_sleep[i] != NULL) {
		if (timers[i] == si->si_value.sival_ptr) {io = i;}
		i++;
	}
	//printf("deberia entrar una vez\n");

	t_bloqueado *proceso;
	pthread_mutex_lock(&cBloqueados);
	proceso = queue_pop(colaBloqueados);
	pthread_mutex_unlock(&cBloqueados);

	pthread_mutex_lock(&cListos);
	queue_push(colaListos, proceso);
	pthread_mutex_unlock(&cListos);

	if (queue_size(colaBloqueados) != 0) {
		proceso = (t_bloqueado*)list_get(colaBloqueados->elements, queue_size(colaBloqueados) - 1);
		makeTimer(timers[io], configNucleo.io_ids_values[io] * proceso->tiempo); //2ms
	}
}

static int makeTimer (timer_t *timerID, int expireMS){
	struct sigevent evp;
	struct itimerspec its;
	struct sigaction sa;
	int sigNo = SIGRTMIN;
	sa.sa_flags = SA_SIGINFO|SA_RESTART;

	sa.sa_sigaction = analizarIO;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sigNo, &sa, NULL) == -1) {
		perror("sigaction");
	}
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = sigNo;
	evp.sigev_value.sival_ptr = timerID;
	//TODO Descomentar las siguientes lineas y ver por que tira error si esta incluida time.h
//	timer_create(CLOCK_REALTIME, &evp, timerID);
//	its.it_value.tv_sec =  floor(expireMS / 1000);
	its.it_value.tv_nsec = expireMS % 1000 * 1000000;
//	timer_settime(*timerID, 0, &its, NULL);
	return 1;
}

void liberarCPU(int socket) {
	int liberar = buscarCPU(socket);
	if (liberar != -1) {
		t_datosCPU *datosCPU;
		pthread_mutex_lock(&listadoCPU);
		datosCPU = (t_datosCPU*) list_get(listaCPU, liberar);
		pthread_mutex_unlock(&listadoCPU);
		datosCPU->estadoCPU = 0;
		planificarProceso();
	} else {
		printf("Error al liberar CPU \n CPU no encontrada en la lista.\n");
	}
}

void cambiarEstadoProceso(int PID, int estado) {
	int cambiar = buscarPCB(PID);
	if (cambiar != -1) {
		t_PCB* datosProceso;
		pthread_mutex_lock(&listadoProcesos);
		datosProceso = (t_PCB*) list_get(listaProcesos, cambiar);
		pthread_mutex_lock(&listadoProcesos);
		datosProceso->estado = estado;
	} else {
		printf("Error al cambiar estado de proceso, proceso no encontrado en la lista.\n");
	}
}

void atenderBloqueados() {
	while (1) {
		sem_wait(&semBloqueados);
		t_bloqueado* datosProceso;
		printf("Entre a ejecutar E/S de Bloqueados.\n");
		pthread_mutex_lock(&cBloqueados);
		datosProceso = (t_bloqueado*) queue_peek(colaBloqueados);
		pthread_mutex_unlock(&cBloqueados);
		sleep(datosProceso->tiempo);
		//Proceso en estado ready
		int estado = 1;
		cambiarEstadoProceso(datosProceso->PID, estado);
		//Agrego a la cola de Listos el Proceso
		int buscar = buscarPCB(datosProceso->PID);
		pthread_mutex_lock(&listadoProcesos);
		t_PCB * informacionDeProceso = list_get(listaProcesos, buscar);
		pthread_mutex_unlock(&listadoProcesos);
		t_proceso* infoProceso = (t_proceso*) malloc(sizeof(t_proceso));
		t_PCB* datos;
		if (buscar != -1) {
			pthread_mutex_lock(&listadoProcesos);
			datos = (t_PCB*) list_get(listaProcesos, buscar);
			pthread_mutex_unlock(&listadoProcesos);
			infoProceso->PID = datos->PID;
			infoProceso->ProgramCounter = datos->ProgramCounter;
			pthread_mutex_lock(&cBloqueados);
			t_proceso* proceso = queue_pop(colaBloqueados);
			pthread_mutex_unlock(&cBloqueados);
			free(proceso);
			if (informacionDeProceso->finalizar == 0) {
				pthread_mutex_lock(&cListos);
				queue_push(colaListos, (void*) infoProceso);
				pthread_mutex_unlock(&cListos);
			} else {
				pthread_mutex_lock(&cFinalizar);
				queue_push(colaFinalizar, (void*) infoProceso);
				pthread_mutex_unlock(&cFinalizar);
			}
			planificarProceso();
		} else {
			printf("Proceso no encontrado en la lista.\n");
		}
	}

}

void actualizarPC(int PID, int ProgramCounter) {
	int cambiar = buscarPCB(PID);
	if (cambiar != -1) {
		t_PCB* datosProceso;
		pthread_mutex_lock(&listadoProcesos);
		datosProceso = (t_PCB*) list_get(listaProcesos, cambiar);
		pthread_mutex_unlock(&listadoProcesos);
		datosProceso->ProgramCounter = ProgramCounter;
	} else {
		printf("Error al cambiar el PC del proceso, proceso no encontrado en la lista.\n");
	}
}

t_valor_variable *obtenerValor(t_nombre_compartida variable) {
	printf("NUCLEO: obtener variable %s\n", variable);
	t_valor_variable *valorVariable = NULL;
	int i = 0;

	while (configNucleo.shared_vars[i] != NULL){

		if (strcmp(configNucleo.shared_vars[i], variable) == 0) {
			//return (int*)configNucleo.shared_vars[i];
			*valorVariable = (t_valor_variable) &configNucleo.shared_vars_values[i];
			break;
		}
		i++;
	}
	//TENER EN CUENTA que aca esta funcionando con el \n
	return valorVariable;
}

void grabarValor(t_nombre_compartida variable, t_valor_variable* valor){

	int i = 0;

	while (configNucleo.shared_vars[i] != NULL){

		if (strcmp(configNucleo.shared_vars[i], variable) == 0) {
			//return (int*)configNucleo.shared_vars[i];
			memcpy(&configNucleo.shared_vars_values[i], valor, sizeof(t_valor_variable));
			return;
		}
		i++;
	}
}

int *pideSemaforo(t_nombre_semaforo semaforo) {
	int i = 0;
	int *valorVariable = NULL;

	while (configNucleo.sem_ids[i] != NULL) {
		//TODO: mutex confignucleo??
		if (strcmp(configNucleo.sem_ids[i], semaforo) == 0) {

			//if (configNucleo.sem_ids_values[i] == -1) {return &configNucleo.sem_ids_values[i];}
			//configNucleo.sem_ids_values[i]--;
			return (&configNucleo.sem_ids_values[i]);
		}
		i++;
	}
	printf("No se encontro el id del semaforo. \n");
	return valorVariable;
}

void grabarSemaforo(t_nombre_semaforo semaforo, int valor){
	int i = 0;

	while (configNucleo.sem_ids[i] != NULL){

		if (strcmp(configNucleo.sem_ids[i], semaforo) == 0) {

			//if (configNucleo.sem_ids_values[i] == -1) return &configNucleo.sem_ids_values[i];
			configNucleo.sem_ids_values[i] = valor;
			return;
		}
		i++;
	}
	printf("No se encontro el id del semaforo. \n");
}


void liberaSemaforo(t_nombre_semaforo semaforo) {
	int i=0;
	t_proceso *proceso;

	while (configNucleo.sem_ids[i] != NULL){
		if (strcmp(configNucleo.sem_ids[i], semaforo) == 0) {

			if(list_size(colas_semaforos[i]->elements)){
				proceso = queue_pop(colas_semaforos[i]);
				pthread_mutex_lock(&cListos);
				queue_push(colaListos, (void*) proceso);
				pthread_mutex_unlock(&cListos);

			}else{
				configNucleo.sem_ids_values[i]++;
			}

			return;
/*
		//Aca esta funcionando con el \n OJO
			configNucleo.sem_ids_values[i]++;
			printf("VALOR SEM: %d\n",configNucleo->sem_ids_values[i]);
			if (proceso = queue_pop(colas_semaforos[i])) {
				//config_nucleo->VALOR_SEM[i]--;
				queue_push(cola_ready, proceso);
				sem_post(&sem_ready);
			}
			return;
			*/
		}
	}
	printf("No se encontro el id del semaforo. \n");
}

void bloqueoSemaforo(int processID, t_nombre_semaforo semaforo){
	int i=0;

	while (configNucleo.sem_ids[i] != NULL) {
		if (strcmp(configNucleo.sem_ids[i], semaforo) == 0) {
			//look for PCB for getting program counter
			t_PCB* PCB;
			int buscar = buscarPCB(processID);
			pthread_mutex_lock(&listadoProcesos);
			PCB = (t_PCB*)list_get(listaProcesos,buscar);
			pthread_mutex_unlock(&listadoProcesos);

			t_proceso* proceso = malloc(sizeof(t_proceso));
			proceso->PID = processID;
			proceso->ProgramCounter = PCB->ProgramCounter;

			//Aca esta funcionando con el \n  OJO
			pthread_mutex_lock(&cSemaforos);
			queue_push(colas_semaforos[i], proceso);
			pthread_mutex_unlock(&cSemaforos);
			return;
		}
		i++;
	}
	printf("No se encontro el id del semaforo. \n");
}

void armarIndiceDeCodigo(t_PCB unBloqueControl,t_metadata_program* miMetaData){
	int i;

	t_registroIndiceCodigo* registroAAgregar = malloc(sizeof(t_registroIndiceCodigo));

	//First instruction

	for (i=0; i < miMetaData->instrucciones_size ; i++){

		registroAAgregar->inicioDeInstruccion= miMetaData->instrucciones_serializado[i].start;
		registroAAgregar->longitudInstruccionEnBytes = miMetaData->instrucciones_serializado[i].offset;
		list_add(unBloqueControl.indiceDeCodigo,(void*)registroAAgregar);
	}
}

void armarIndiceDeEtiquetas(t_PCB unBloqueControl,t_metadata_program* miMetaData){

	strcpy(unBloqueControl.indiceDeEtiquetas, miMetaData->etiquetas);

	log_error(logNucleo,"'indiceDeEtiquetas' size: %d\n", miMetaData->etiquetas_size);
}

void finalizarPid(int pid){
	if((pid<0) || (pid >idProcesos)){
		printf ("Error, PID inexistente.\n");
		return;
	}
	int ret = buscarPCB(pid);
	if (ret == -1){
		printf("Error al buscar el proceso.\n");
		return;
	}
	t_PCB* datosProceso;
	pthread_mutex_lock(&listadoProcesos);
	datosProceso = (t_PCB*)list_get(listaProcesos,ret);
	pthread_mutex_unlock(&listadoProcesos);
	switch(datosProceso->estado){
	case 1:
		  	//EL proceso esta en la cola de Ready
			datosProceso->finalizar = 1;
			break;
	case 2:
			//El Proceso esta ejecutando seteo una bandera para avisar que vuelva a cola privilegiada.
			datosProceso->finalizar = 1;
			break;
	case 3:
			//EL Proceso esta bloqueado
			datosProceso->finalizar = 1;
			break;

	case 4: printf ("Error, el proceso ya finalizo correctamente.\n");
			break;

	case 5: printf ("Error, el proceso ya finalizo de forma incorrecta.\n");
			break;
	default:
			break;

	}
}

void iniciarPrograma(int PID, char *codeScript) {

	int bufferSize = 0;
	int payloadSize = 0;
	int contentLen = strlen(codeScript) + 1;	//+1 because of '\0'
	//En el caso de usar la siguiente linea: VER por que tira error si esta incluida math.h (en nucleo.h)
	//int cantPages = ceil((double) contentLen /(double) frameSize);
	int cantPages = (strlen(codeScript) + 1) / frameSize;

	t_MessageNucleo_UMC *message = malloc(sizeof(t_MessageNucleo_UMC));

	message->operation = agregar_proceso;
	message->PID = PID;
	message->cantPages = cantPages + configNucleo.stack_size;

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->cantPages);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize;
	char *buffer = malloc(bufferSize);

	//Serializar mensaje
	serializeNucleo_UMC(message, buffer, payloadSize);

	//1) Enviar info a la UMC - message serializado
	sendMessage(&socketUMC, buffer, bufferSize);

	//2) Despues de enviar la info, Enviar el tamanio del prgrama
	string_append(&codeScript, "\0");	//ALWAYS put \0 for finishing the string
	sendMessage(&socketUMC, (void*) contentLen, sizeof(contentLen));

	//3) Enviar programa
	sendMessage(&socketUMC, (void*) codeScript, contentLen);

	free(message);

}

void finalizarPrograma(int PID){

	int bufferSize = 0;
	int payloadSize = 0;

	t_MessageNucleo_UMC *message = malloc(sizeof(t_MessageNucleo_UMC));

	message->operation = finalizar_proceso;
	message->PID = PID;
	message->cantPages = -1; //DEFAULT value when the operation doesn't need it

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->cantPages);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize;

	char *buffer = malloc(bufferSize);

	//Serializar mensaje
	serializeNucleo_UMC(message, buffer, payloadSize);

	//1) Enviar info a la UMC - message serializado
	sendMessage(&socketUMC, buffer, bufferSize);

	free(message);
}

void deserializarES(t_es* datos, char* bufferReceived) {
	int dispositivoLen = 0;
	int offset = 0;

	//1) Tiempo
	memcpy(&datos->tiempo, bufferReceived + offset, sizeof(datos->tiempo));
	offset += sizeof(datos->tiempo);

	//2) ProgramCounter
	memcpy(&datos->ProgramCounter, bufferReceived + offset, sizeof(datos->ProgramCounter));
	offset += sizeof(datos->ProgramCounter);

	//3) DispositivoLen
	memcpy(&dispositivoLen, bufferReceived + offset, sizeof(dispositivoLen));
	offset += sizeof(dispositivoLen);

	//4) Dispositivo
	datos->dispositivo = malloc(dispositivoLen);
	memcpy(datos->dispositivo, bufferReceived + offset, dispositivoLen);

	//Verificar que se este enviando el tamanio del dispositivo que es un char* y que se este considerando el \0 en el offset
}

void crearArchivoDeConfiguracion(char *configFile){
	t_config* configuration;

	configuration = config_create(configFile);
	configNucleo.puerto_prog = config_get_int_value(configuration,"puerto_prog");
	configNucleo.puerto_cpu = config_get_int_value(configuration,"puerto_cpu");
	configNucleo.ip_umc = config_get_string_value(configuration,"ip_umc");
	configNucleo.puerto_umc = config_get_int_value(configuration,"puerto_umc");
	configNucleo.quantum = config_get_int_value(configuration,"quantum");
	configNucleo.quantum_sleep = config_get_int_value(configuration,"quantum_sleep");
	configNucleo.sem_ids = config_get_array_value(configuration,"sem_ids");
	configNucleo.sem_init = config_get_array_value(configuration,"sem_init");
	configNucleo.io_ids = config_get_array_value(configuration,"io_ids");
	configNucleo.io_sleep = config_get_array_value(configuration,"io_sleep");
	configNucleo.shared_vars = config_get_array_value(configuration,"shared_vars");
	configNucleo.stack_size = config_get_int_value(configuration,"stack_size");
	configNucleo.pageSize = config_get_int_value(configuration,"pageSize");
	int i = 0;

	if(timers!=0){
		free(timers);
	}
	timers = initialize(strlen((char*)configNucleo.io_sleep) * sizeof(char*));
	colaBloqueados = initialize(strlen((char*)configNucleo.io_sleep) * sizeof(char*));
	colaBloqueados = initialize(sizeof(t_queue*));

	while (configNucleo.io_sleep[i] != NULL){
		timers[i] = initialize(sizeof(timer_t));
		i++;
	}

	/*while (configNucleo.io_sleep[i] != NULL){
		printf("valor %d - %d\n",i,configNucleo.io_sleep[i]);
		i++;
	}*/

	//initializing shared variables values (int**)
	/*i = 0;
	while (configNucleo.shared_vars[i] != NULL){
		configNucleo.shared_vars_values[i] = 0; //DEFAULT Value
		i++;
	}*/

	//initializing shared variables values (int*)
	i=0;
	configNucleo.shared_vars_values = initialize(((strlen((char*)configNucleo.shared_vars)) / sizeof(char*)) * sizeof(int));
	while (configNucleo.shared_vars[i] != NULL) {
		configNucleo.shared_vars_values[i] = 0; //DEFAULT Value
		i++;
	}
	//initializing io_ids_values (int*)
	i=0;
	configNucleo.io_ids_values= initialize(((strlen((char*)configNucleo.io_sleep)) / sizeof(char*)) * sizeof(int));
	while (configNucleo.io_sleep[i] != NULL) {
		configNucleo.io_ids_values[i] = atoi(configNucleo.io_sleep[i]);
		i++;
	}
	//initializing sem_ids_values (int*)
	i=0;
	configNucleo.sem_ids_values= initialize(((strlen((char*)configNucleo.sem_init)) / sizeof(char*)) * sizeof(int));
	while (configNucleo.sem_init[i] != NULL) {
		configNucleo.sem_ids_values[i] = atoi(configNucleo.sem_init[i]);
		i++;
	}

	if(colas_semaforos!=0){
		free(colas_semaforos);
	}
	colas_semaforos = initialize(strlen((char*)configNucleo.sem_init) * sizeof(char*));

	i = 0;
	while (configNucleo.sem_init [i] != NULL){
		colas_semaforos[i] = initialize(sizeof(t_queue));
		colas_semaforos[i] = queue_create();
		i++;
	}
}

void *initialize(int tamanio){
	int i;
	void * retorno = malloc(tamanio);
	for(i=0;i<tamanio;i++){
		((char*)retorno)[i]=0;
	}
	return retorno;
}

int connectTo(enum_processes processToConnect, int *socketClient){
	int exitcode = EXIT_FAILURE;//DEFAULT VALUE
	int port = 0;
	char *ip = string_new();

	switch (processToConnect){
	case UMC:{
		string_append(&ip,configNucleo.ip_umc);
		port= configNucleo.puerto_umc;
		break;
	}
	default:{
		log_info(logNucleo,"Process '%s' NOT VALID to be connected by NUCLEO.\n", getProcessString(processToConnect));
		break;
	}
	}
	exitcode = openClientConnection(ip, port, socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == EXIT_SUCCESS){

		// ***1) Send handshake
		exitcode = sendClientHandShake(socketClient,NUCLEO);

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

				switch (message->process) {
				case ACCEPTED: {
					switch (processToConnect) {
						case UMC: {
							log_info(logNucleo, "Conectado a UMC - Message: %s\n",	message->message);
							printf("Receiving frame size\n");
							//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
							receivedBytes = receiveMessage(socketClient, &frameSize,sizeof(messageSize));

							printf("Tamanio de pagina: %d\n", frameSize);
							break;
						}
						default: {
							log_error(logNucleo,
									"Handshake not accepted when tried to connect your '%s' with '%s'\n",
									getProcessString(processToConnect),	getProcessString(message->process));
							close(*socketClient);
							exitcode = EXIT_FAILURE;
							break;
						}

					}
					break;
				}	//fin case ACCEPTED
				default: {
					log_error(logNucleo,"Process couldn't connect to SERVER - Not able to connect to server %s. Please check if it's down.\n",ip);
					close(*socketClient);
					break;
				}
				}
			}else if (receivedBytes == 0 ){
				//The client is down when bytes received are 0
				log_error(logNucleo, "The client went down while receiving! - Please check the client '%d' is down!\n",*socketClient);
				close(*socketClient);
			}else{
				log_error(logNucleo, "Error - No able to received - Error receiving from socket '%d', with error: %d\n",*socketClient, errno);
				close(*socketClient);
			}
		}
	}else{
		 log_error(logNucleo, "I'm not able to connect to the server! - My socket is: '%d'\n", *socketClient);
		 close(*socketClient);
	}

	return exitcode;
}
