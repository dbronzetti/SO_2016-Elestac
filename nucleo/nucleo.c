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
	pthread_t consolaThread;

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS

	//get parameter
	int i;
	for( i = 0; i < argc; i++){
		//check config file parameter
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

	//ERROR if not Log parameter was passed
	assert(("ERROR - NOT log file was passed as argument", logFile != NULL));//Verifies if was passed the Log file as parameter, if DONT FAILS

	//Creo el archivo de Log
		logNucleo = log_create(logFile, "NUCLEO", 0, LOG_LEVEL_TRACE);
	//Creo archivo de configuracion
		//configurationFile = "/home/utnso/git/tp-2016-1c-YoNoFui/nucleo/configuracion.nucleo";
		crearArchivoDeConfiguracion(configurationFile);
	//Creo la lista de CPUs
		listaCPU = list_create();
	//Creo la lista de Consolas
		listaConsola = list_create();
	//Creo Lista Procesos
		listaProcesos = list_create();
	//Creo la Cola de Listos
		colaListos = queue_create();
	//Creo cola de Procesos a Finalizar (por Finalizar PID).
		colaFinalizar = queue_create();

	// Inicializacion de los mutex
	pthread_mutex_init(&listadoCPU, NULL);
	pthread_mutex_init(&listadoConsola, NULL);
	pthread_mutex_init(&listadoProcesos, NULL);
	pthread_mutex_init(&cListos, NULL);
	pthread_mutex_init(&cBloqueados, NULL);
	pthread_mutex_init(&cFinalizar, NULL);
	pthread_mutex_init(&cSemaforos, NULL);
	pthread_mutex_init(&globalMutex, NULL);
	pthread_mutex_init(&activeProcessMutex, NULL);
	pthread_mutex_init(&mutex_config, NULL);

	pthread_create(&consolaThread, NULL, (void*) startNucleoConsole, NULL);

	//Create thread for server start
	pthread_create(&serverThread, NULL, (void*) startServerProg, NULL);
	pthread_create(&serverConsolaThread, NULL, (void*) startServerCPU, NULL);

	exitCode = connectTo(UMC,&socketUMC);
	if (exitCode == EXIT_SUCCESS) {
		log_info(logNucleo, "NUCLEO connected to UMC successfully\n");
	}else{
		log_error(logNucleo, "No server available - shutting down proces!!\n");
		return EXIT_FAILURE;
	}

	pthread_join(consolaThread, NULL);
	pthread_join(serverThread, NULL);
	pthread_join(serverConsolaThread, NULL);

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
				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){// Si uso connectTo => debo considerar este bloque
					//Sending stackSize to CPU
					sendMessage(&serverData->socketClient, &configNucleo.stack_size, sizeof(configNucleo.stack_size));
					//Adding CPU socket to list
					t_datosCPU *datosCPU = malloc(sizeof(t_datosCPU));
					datosCPU->numSocket = serverData->socketClient;
					datosCPU->estadoCPU = 0;
					pthread_mutex_lock(&listadoCPU);
					list_add(listaCPU, (void*)datosCPU);
					pthread_mutex_unlock(&listadoCPU);
					log_info(logNucleo, "se agrego CPU: %d a la listaCPU cuyo size es: %d\n",datosCPU->numSocket, list_size(listaCPU));
				}

				break;
			}
			case CONSOLA:{
				log_info(logNucleo, "Message from '%s': %s\n", getProcessString(message->process), message->message);
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
	free(message);
}

void processMessageReceived (void *parameter){

	t_serverData *serverData = (t_serverData*) parameter;
	log_info(logNucleo, "Socket client connected to NUCLEO: %d\n",serverData->socketClient);

	//Receive message size
	int messageSize = 0;
	char *messageRcv = malloc(sizeof(messageSize));
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(messageSize));

	if ( receivedBytes > 0 ){

		//Get Payload size
		memcpy(&messageSize, messageRcv, sizeof(messageSize));
		log_info(logNucleo, "Tamanio messageSize recibido: %d \n", messageSize);

		//Receive process from which the message is going to be interpreted
		enum_processes fromProcess;
		messageRcv = realloc(messageRcv, sizeof(fromProcess));
		receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(fromProcess));
		memcpy(&fromProcess, messageRcv, sizeof(fromProcess));

		log_info(logNucleo,"Bytes received from process '%s': %d\n",getProcessString(fromProcess),receivedBytes);

		switch (fromProcess){
			case CONSOLA:{
				log_info(logNucleo, "Processing %s message received\n", getProcessString(fromProcess));
				pthread_mutex_lock(&activeProcessMutex);
				/*int* tamanio = malloc(sizeof(int));
				receiveMessage(&serverData->socketClient, tamanio, sizeof(int));*/

				int opFinalizar = messageSize;
				if (opFinalizar == -1) { 	//Finaliza Proceso
					int PID = buscarPIDConsola(serverData->socketClient);
					if (PID==-1){
						log_error(logNucleo,"No se encontro Consola para el socket: %d \n",serverData->socketClient);
						pthread_mutex_unlock(&activeProcessMutex);
						break;
					}
					log_info(logNucleo,"Solicitando finalizar el programa:%d para el socket: %d \n",PID, serverData->socketClient);
					finalizarPid(PID);
					//OJO con los DEADLOCKS - Si se retorna sin desbloquear puede bloquear el proceso.
					pthread_mutex_unlock(&activeProcessMutex);
					break;
				}

				//Recibo el codigo del programa (messageRcv) usando el tamanio leido antes
				messageRcv = realloc(messageRcv, messageSize);
				receiveMessage(&serverData->socketClient, messageRcv, messageSize);

				log_info(logNucleo,"El Nucleo recibe el codigo del programa:\n %s del socket: %d \n",messageRcv, serverData->socketClient);
				socketConsola = serverData->socketClient;
				runScript(messageRcv);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			case CPU:{
				log_info(logNucleo, "Processing %s message received\n", getProcessString(fromProcess));
				pthread_mutex_lock(&activeProcessMutex);
				processCPUMessages(messageRcv, messageSize, serverData->socketClient);
				//free(parameter);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			default:{
			log_error(logNucleo,
					"Process not allowed to connect - Invalid process '%s' send a message to NUCLEO \n",
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

	//free(messageRcv);
}

void runScript(char* codeScript){
	//Creo el PCB del proceso.
	t_PCB* PCB = malloc(sizeof(t_PCB));
	t_proceso* datosProceso = malloc(sizeof(t_proceso));
	log_info(logNucleo,"creo el PCB del proceso\n");

	//Armo Indice de codigo, etiquetas y stack
	t_metadata_program *miMetaData = metadata_desde_literal(codeScript);

	pthread_mutex_lock(&globalMutex);
	PCB->PID = idProcesos;
	idProcesos++;
	pthread_mutex_unlock(&globalMutex);

	PCB->ProgramCounter = miMetaData->instruccion_inicio;
	int contentLen = strlen(codeScript) + 1;	//+1 debido al '\0'
	PCB->cantidadDePaginas = (int) ceil((double) contentLen/ (double) frameSize);
	PCB->StackPointer = 0;
	PCB->estado = 1;
	PCB->finalizar = 0;
	log_info(logNucleo,"se inicializan los campos que no son listas\n");
	log_info(logNucleo,"Cantidad de paginas que ocupa el codigo: %d\n",PCB->cantidadDePaginas);

	PCB->indiceDeCodigo = list_create();
	armarIndiceDeCodigo(PCB, miMetaData);
	armarIndiceDeEtiquetas(PCB, miMetaData);
	PCB->indiceDeStack = list_create();
	log_info(logNucleo,"se arman las listas");

	//Get last program line from main
	int index = miMetaData->instruccion_inicio ;
	t_registroIndiceCodigo* instruccionActual;

	while (index < miMetaData->instrucciones_size){

		instruccionActual = (t_registroIndiceCodigo*) list_get(PCB->indiceDeCodigo, index);

		char *line = malloc(instruccionActual->longitudInstruccionEnBytes);
		memcpy(line, codeScript + instruccionActual->inicioDeInstruccion, instruccionActual->longitudInstruccionEnBytes);

		if (string_starts_with(line,TEXT_END)){
			free(line);
			break;
		}

		free(line);
		index++;
	}

	PCB->finDePrograma = index;
	log_info(logNucleo,"se asigna el index: %d al PCB que indica el fin del Programa",index);

	pthread_mutex_lock(&listadoProcesos);
	list_add(listaProcesos,(void*)PCB);
	pthread_mutex_unlock(&listadoProcesos);
	log_info(logNucleo, "myProcess %d - Iniciado  Script: \n %s \n",PCB->PID, codeScript);

	//Agrego a la Cola de Listos
	datosProceso->ProgramCounter = PCB->ProgramCounter;
	datosProceso->PID = PCB->PID;

	pthread_mutex_lock(&cListos);
	queue_push(colaListos, (void*)datosProceso);
	pthread_mutex_unlock(&cListos);
	log_info(logNucleo, "size colaListos: %d",queue_size(colaListos));

	//Agrego a la lista de Consolas
	t_datosConsola* datosConsola = malloc(sizeof(t_datosConsola));
	datosConsola->numSocket = socketConsola;
	datosConsola->PID = PCB->PID;
	pthread_mutex_lock(&listadoConsola);
	list_add(listaConsola, (void*) datosConsola);
	pthread_mutex_unlock(&listadoConsola);
	log_info(logNucleo, "size listaConsola: %d",list_size(listaConsola));

	metadata_destruir(miMetaData);

	iniciarPrograma(datosConsola->PID, codeScript);

	planificarProceso();
}

void planificarProceso() {
	//Veo si hay procesos para planificar en la cola de Listos y en la cola Finalizar
	if (queue_is_empty(colaListos) && (queue_is_empty(colaFinalizar))) {
		return;
	}
	//Veo si hay CPU libre para asignarle
	int libreCPU = buscarCPULibre();

	if (libreCPU == -1) {
		log_error(logNucleo,"No hay CPU libre \n");
		return;
	}
	t_serverData *serverData = malloc(sizeof(t_serverData));
	serverData->socketClient = libreCPU;

	//Le envio la informacion a la CPU
	t_PCB* datosPCB;
	t_MessageNucleo_CPU* contextoProceso = malloc(sizeof(t_MessageNucleo_CPU));
	t_proceso* datosProceso;

	pthread_mutex_lock(&cListos);
	datosProceso = (t_proceso*) queue_peek(colaListos);
	pthread_mutex_unlock(&cListos);

	int posicion = buscarPCB(datosProceso->PID);
	if (posicion != -1) {
		pthread_mutex_lock(&listadoProcesos);
		datosPCB = (t_PCB*) list_get(listaProcesos, posicion);
		pthread_mutex_unlock(&listadoProcesos);

		pthread_mutex_lock(&mutex_config);
		contextoProceso->quantum = configNucleo.quantum;
		contextoProceso->quantum_sleep=configNucleo.quantum_sleep;
		pthread_mutex_unlock(&mutex_config);
		log_info(logNucleo,"Nucleo se prepara para enviar informacion del processID: %d al proceso CPU \n",datosProceso->PID);

		enviarMsjCPU(datosPCB, contextoProceso, serverData);
		free(contextoProceso);
	} else {
		log_info(logNucleo,"Proceso no encontrado\n");
	}

	/*if (queue_is_empty(colaFinalizar)) {

		//Aca iba lo anerior en el caso de que se le avise al CPU operacion=0

	} else {
		pthread_mutex_lock(&cFinalizar);
		datosProceso = (t_proceso*) queue_peek(colaFinalizar);
		pthread_mutex_unlock(&cFinalizar);
		int posicion = buscarPCB(datosProceso->PID);
		if (posicion != -1) {
			pthread_mutex_lock(&listadoProcesos);
			datosPCB = (t_PCB*) list_get(listaProcesos, posicion);
			pthread_mutex_unlock(&listadoProcesos);

		// Aviso al CPU que finalice el proceso (op = 1) y luego voy a esperar la Respuesta(processCPUMessages)
			contextoProceso->operacion = 1;
			enviarMsjCPU(datosPCB, contextoProceso, serverData);

		} else {
			printf("Proceso no encontrado en la lista.\n");
		}
	}*/
}

void enviarMsjCPU(t_PCB* datosPCB,t_MessageNucleo_CPU* contextoProceso, t_serverData* serverData){

		contextoProceso->programCounter = datosPCB->ProgramCounter;
		contextoProceso->processID = datosPCB->PID;
		contextoProceso->stackPointer = datosPCB->StackPointer;
		contextoProceso->cantidadDePaginas = datosPCB->cantidadDePaginas;

		log_info(logNucleo,"se realiza el traspaso de datosPCB para en enviarle al proceso CPU\n");

		int payloadSize = sizeof(contextoProceso->programCounter) + sizeof(contextoProceso->processID)
			+ sizeof(contextoProceso->stackPointer)+ sizeof(contextoProceso->cantidadDePaginas)
			+ sizeof(contextoProceso->quantum) + sizeof(contextoProceso->quantum_sleep);

		int bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;
		char* bufferEnviar = malloc(bufferSize);
		//Serializar mensaje basico del PCB
		serializeNucleo_CPU(contextoProceso, bufferEnviar, payloadSize);

		//Hacer send al CPU de lo que se serializo arriba
		sendMessage(&serverData->socketClient, bufferEnviar, bufferSize);
		log_info(logNucleo,"Se envia el mensaje basico del PCB al proceso CPU de bufferSize: %d\n", bufferSize);

		//serializar estructuras del indice de codigo
		char* bufferIndiceCodigo = NULL;
		int tamanioIndiceCodigo = serializarListaIndiceDeCodigo(datosPCB->indiceDeCodigo, bufferIndiceCodigo);

		//send tamaño de lista indice codigo
		sendMessage(&serverData->socketClient, &tamanioIndiceCodigo, sizeof(tamanioIndiceCodigo));
		//send lista indice codigo
		sendMessage(&serverData->socketClient, bufferIndiceCodigo, tamanioIndiceCodigo);

		free(bufferIndiceCodigo);
		log_info(logNucleo,"se envia la lista de indice de codigo de %d elementos al proceso CPU - Tamanio indice Codigo: %d\n",datosPCB->indiceDeCodigo->elements_count, tamanioIndiceCodigo);

		//serializar estructuras del stack
		char* bufferIndiceStack =  malloc(sizeof(datosPCB->indiceDeStack->elements_count));
		serializarListaStack(datosPCB->indiceDeStack, bufferIndiceStack);
		log_info(logNucleo,"serializo la lista indice de stack para preparar su envio al proceso CPU\n");

		//send tamaño de lista indice stack
		sendMessage(&serverData->socketClient, (void*) strlen(bufferIndiceStack), sizeof(int));
		log_info(logNucleo,"Se envia el tamanio %d (strlen) del buffer indice de Stack al proceso CPU\n",strlen(bufferIndiceStack));

		if (datosPCB->indiceDeStack->elements_count > 0 ){
			//send lista indice stack
			sendMessage(&serverData->socketClient, bufferIndiceStack, strlen(bufferIndiceStack));
			log_info(logNucleo,"se envia la lista de indice de stack de %d elementos al proceso CPU - Tamanio indice Stack: %d\n", datosPCB->indiceDeStack->elements_count ,bufferIndiceStack);
		}

		free(bufferIndiceStack);

		//send tamaño de indice de etiquetas
		sendMessage(&serverData->socketClient, &datosPCB->indiceDeEtiquetasTamanio, sizeof(datosPCB->indiceDeEtiquetasTamanio));

		if (datosPCB->indiceDeEtiquetasTamanio > 0 ){
			//send indice de etiquetas if > 0
			sendMessage(&serverData->socketClient, datosPCB->indiceDeEtiquetas, datosPCB->indiceDeEtiquetasTamanio);
			log_info(logNucleo,"se envia el indice de etiquetas al proceso CPU - Tamanio indice Etiquetas: %d\n", datosPCB->indiceDeEtiquetasTamanio);

		}

		//Saco el primer elemento de la cola, porque ya lo planifique.
		pthread_mutex_lock(&cListos);
		t_proceso* proceso = queue_pop(colaListos);
		pthread_mutex_unlock(&cListos);

		//Cambio Estado del Proceso
		int estado = 2;
		cambiarEstadoProceso(datosPCB->PID, estado);

		free(bufferEnviar);
		free(proceso);

		//1) Recibo mensajes del CPU
		log_info(logNucleo,"El NUCLEO se queda esperando una respuesta del proceso CPU\n");
		processMessageReceived(&serverData);

		//2) processCPUMessages se hace dentro de processMessageReceived (case CPU)

}

void processCPUMessages(char* messageRcv,int messageSize,int socketCPULibre){
	log_info(logNucleo, "Processing CPU message \n");

	t_MessageCPU_Nucleo *message=malloc(sizeof(t_MessageCPU_Nucleo));

	//Receive message using the size read before
	messageRcv = realloc(messageRcv, messageSize);
	receiveMessage(&socketCPULibre, messageRcv, messageSize);

	//Deserializo messageRcv
	deserializeNucleo_CPU(message, messageRcv);

	switch (message->operacion) {
	case 1:{ 	//Entrada Salida
		t_es infoES;
		char *datosEntradaSalida = malloc(sizeof(t_es));
		//change active PID
		activePID = message->processID;

		receiveMessage(&socketCPULibre, datosEntradaSalida, sizeof(t_es));
		deserializarES(&infoES, datosEntradaSalida);

		//Libero la CPU que ocupaba el proceso
		liberarCPU(socketCPULibre);

		//Cambio el PC del Proceso
		actualizarPC(message->processID, infoES.ProgramCounter);

		//Cambio el estado del proceso a bloqueado
		int estado = 3;
		cambiarEstadoProceso(message->processID, estado);

		EntradaSalida(infoES.dispositivo, infoES.tiempo);
		free(datosEntradaSalida);
		break;
	}
	case 2:{ 	//Finaliza Proceso Bien
		finalizaProceso(socketCPULibre, message->processID,message->operacion);
		break;
	}
	case 3:{ 	//Finaliza Proceso Mal
		finalizaProceso(socketCPULibre, message->processID,message->operacion);
		break;
	}
	case 4:{
		log_error(logNucleo, "No se pudo obtener la solicitud a ejecutar - Error al finalizar");
		break;
	}
	case 5:{ 	//Corte por Quantum
		log_info(logNucleo, "Se procesa la atencion del corte por Quantum");
		atenderCorteQuantum(socketCPULibre, message->processID);
		break;
	}
	//TODO mutex configNucleo
	case 6:{	//Obtener valor y enviarlo al CPU

		// 1) Recibir tamanio de la variable
		int* variableLen = malloc(sizeof(int));
		receiveMessage(&socketCPULibre, variableLen, sizeof(int));

		// 2) Recibir la variable
		t_nombre_compartida variable = malloc(*variableLen);
		receiveMessage(&socketCPULibre, variable, *variableLen);

		t_valor_variable* valorVariable = obtenerValor(variable);

		if (valorVariable == NULL){
			log_error(logNucleo, "No se encontro la variable: %s id, con el tamanio: %d  \n",variable, *variableLen);
			break;
		}

		sendMessage(&socketCPULibre, valorVariable, sizeof(t_valor_variable));
		free(variableLen);
		free(valorVariable);
		break;
	}
	case 7:{	//Grabar valor

		// 1) Recibir valor
		t_valor_variable* valor = malloc(sizeof(t_valor_variable));
		receiveMessage(&socketCPULibre, valor, sizeof(t_valor_variable));

		// 2) Recibir tamanio de la variable
		int* variableLen = malloc(sizeof(int));
		receiveMessage(&socketCPULibre, variableLen, sizeof(int));

		// 3) Recibir la variable
		t_nombre_compartida variable = malloc(*variableLen);
		receiveMessage(&socketCPULibre, variable, *variableLen);

		grabarValor(variable, valor);
		log_info(logNucleo, "Se graba el valor: %d en la variable: %s id, con el tamanio: %d  \n",valor, variable, *variableLen);

		free(variableLen);
		free(variable);
		free(valor);

		break;
	}
	case 8:{	// WAIT - Grabar semaforo y enviar al CPU
		//Recibo el tamanio del wait
		int* tamanio = malloc(sizeof(int));
		receiveMessage(&socketCPULibre, tamanio, sizeof(int));

		//Recibo el semaforo wait
		t_nombre_semaforo semaforo = malloc(*tamanio);
		receiveMessage(&socketCPULibre, semaforo, *tamanio);

		log_info(logNucleo, "Se recibe el semaforo WAIT: %s, con el tamanio: %d  \n",semaforo, *tamanio);

		if (estaEjecutando(message->processID)==1){ // 1: Programa ejecutandose (no esta en ninguna cola)
			int * valorSemaforo = pideSemaforo(semaforo);
			int valorAEnviar;
			if(*valorSemaforo<=0){

				valorAEnviar = 1;
				log_info(logNucleo, "Recibi proceso %d mando a bloquear por semaforo: %d \n", (message->processID)%6+1, semaforo);
				sendMessage(&socketCPULibre, &valorAEnviar,sizeof(int));// 1 si se bloquea. 0 si no se bloquea

				//TODO Verificar: recibir PCB del CPU para volver a planificarlo
				//agregarlo a donde corresponda: (se hace en bloqueoSemaforo) para volver a enviarlo cuando se desbloquee el semaforo

				int* tamanioStack = malloc(sizeof(int));
				receiveMessage(&socketCPULibre, tamanioStack, sizeof(int));

				char* listaStackSerializada = malloc(sizeof(*tamanioStack));
				receiveMessage(&socketCPULibre, listaStackSerializada, sizeof(int));

				t_list* listaIndiceDeStack = list_create();
				deserializarListaStack(listaIndiceDeStack, listaStackSerializada);

				bloqueoSemaforo(message->processID, semaforo);

				//Libero la CPU que ocupaba el proceso
				liberarCPU(socketCPULibre);
				free(tamanioStack);
				free(listaStackSerializada);
			}else{
				grabarSemaforo(semaforo,*pideSemaforo(semaforo)-1);
				valorAEnviar=0;
				sendMessage(&socketCPULibre, &valorAEnviar, sizeof(int));
			}
		}
		free(tamanio);
		free(semaforo);
		break;
	}
	case 9:{	// SIGNAL	- 	Libera semaforo

		//Recibo el tamanio del signal
		int* tamanio = malloc(sizeof(int));
		receiveMessage(&socketCPULibre, tamanio, sizeof(int));

		//Recibo el semaforo wait
		t_nombre_semaforo semaforo = malloc(*tamanio);
		receiveMessage(&socketCPULibre, semaforo, *tamanio);
		log_info(logNucleo, "Proceso %d libera semaforo:%s \n", message->processID, semaforo);
		liberaSemaforo(semaforo);

		free(tamanio);
		free(semaforo);
		break;
	}
	case 10:{	//Imprimir VALOR por Consola
		int socketConsola = buscarSocketConsola(message->processID);

		if (socketConsola==-1){
			log_error(logNucleo, "No se encontro Consola para el PID: %d \n",message->processID);
			break;
		}

		//Received value from CPU
		t_valor_variable* valor = malloc(sizeof(t_valor_variable));
		receiveMessage(&socketCPULibre, valor, sizeof(t_valor_variable));

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
			log_error(logNucleo,"No se encontro Consola para el PID: %d \n",message->processID);
			break;
		}

		int* tamanio = malloc(sizeof(int));
		char* texto = malloc(*tamanio);

		//Recibo el tamanio del texto
		receiveMessage(&socketCPULibre, tamanio,sizeof(int));

		//Recibo el texto
		receiveMessage(&socketCPULibre, texto, *tamanio);

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
		break;
	}
	case 13:{ //Get last main line from a PID
		//get PCB from a given PID
		t_PCB* infoProceso;
		int buscar = buscarPCB(message->processID);
		pthread_mutex_lock(&listadoProcesos);
		infoProceso = (t_PCB*)list_get(listaProcesos,buscar);
		pthread_mutex_unlock(&listadoProcesos);

		//send back the last line information to the CPU requestor
		sendMessage(&socketCPULibre, &infoProceso->finDePrograma, sizeof(infoProceso->finDePrograma));

		break;
	}
	case 72:{
		alertFlag = 1;
		atenderCorteQuantum(socketCPULibre, message->processID);
		break;
	}
	default:
		log_error(logNucleo, "Mensaje recibido invalido. \n");
		//printf("CPU desconectado.\n");
		//abort();
	}

	free(message);
}

void atenderCorteQuantum(int socketCPU,int PID){
	//Libero la CPU
	if (alertFlag == 0){
		liberarCPU(socketCPU);
	}else{
		int pos = buscarCPU(socketCPU);
		t_datosCPU* datosCPU = list_remove(listaCPU,pos);
		free(datosCPU);
	}
	//Cambio el PC del Proceso, le sumo el quantum al PC actual.
	t_PCB* infoProceso;
	int buscar = buscarPCB(PID);
	pthread_mutex_lock(&listadoProcesos);
	infoProceso = (t_PCB*)list_get(listaProcesos,buscar);
	pthread_mutex_unlock(&listadoProcesos);

	//Receiving quantum used by CPU
	int quantumUsed = 0;
	receiveMessage(&socketCPU, &quantumUsed, sizeof(int));

	int pcnuevo = infoProceso->ProgramCounter + quantumUsed;
	infoProceso->ProgramCounter = pcnuevo;

	log_info(logNucleo, "Recibiendo PCB para el PID: %d modificado por el CPU de socket: %d \n",infoProceso->PID, socketCPU);

	//1) receive tamanioBuffer
	int tamanioBuffer;
	receiveMessage(&socketCPU, &tamanioBuffer, sizeof(tamanioBuffer));
	//2) receive buffer segun tamanioBuffer
	char* buffer = malloc(tamanioBuffer);
	receiveMessage(&socketCPU, buffer, tamanioBuffer);
	//3) malloc de listaARecibir segun el tamanio recibido.
	t_list* listaARecibir = list_create();
	//4) deserializarListaStack(listaARecibir, buffer);
	deserializarListaStack(listaARecibir, buffer);
	//5) borrar lista en infoProceso->indiceDeStack
	list_clean_and_destroy_elements(infoProceso->indiceDeStack, (void*) cleanIndiceDeStack);
	//6) infoProceso->indiceDeStack = listaARecibir;
	list_add_all(infoProceso->indiceDeStack, (void*) listaARecibir);

	free(buffer);
	//RECIBIR PCB MODIFICADO DEL CPU! (lo que hace falta en realidad es el stack fundamentalmente y ver si es necesario algo mas que haya modificado el CPU)
	/*
	 * 1) receive tamanioBuffer
	 * 2) receive buffer segun tamanioBuffer
	 * 3) malloc de listaARecibir segun el tamanio recibido.
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
	//Aviso a la UMC para finalizar
	finalizarPrograma(PID);

	int socketConsola = buscarSocketConsola(PID);
	//Enviar operacion=3 que imprime texto y exit(0);
	int operacion = 3;
	sendMessage(&socketConsola, &operacion, sizeof(int));
	// Envia el tamanio del texto y luego el texto al proceso Consola
	char texto[] = "Se finaliza el proceso por peticion de la Consola";
	int textoLen = strlen(texto)+1;
	sendMessage(&socketConsola, &textoLen, sizeof(textoLen));

	log_info(logNucleo, "Enviar texto: '%s', con tamanio: '%d' a PID #%d\n", texto, textoLen, PID);
	sendMessage(&socketConsola, texto, textoLen);

	destruirPCB(datosProceso);

	int pos = buscarConsola(socketConsola);
	list_remove(listaConsola,pos);

	//Mando a revisar si hay alguno en la lista para ejecutar.
	planificarProceso();

}

int buscarCPULibre() {
	int cantCPU, i = 0;
	t_datosCPU* datosCPU;
	cantCPU = list_size(listaCPU);
	log_info(logNucleo, "size de listaCPU: %d\n",list_size(listaCPU));
	for (i = 0; i < cantCPU; i++) {
		pthread_mutex_lock(&listadoCPU);
		datosCPU = (t_datosCPU*) list_get(listaCPU, 0);
		pthread_mutex_unlock(&listadoCPU);

		if (datosCPU->estadoCPU == 0) {
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

int buscarSocketConsola(int PID) {
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

int buscarConsola(int socket) {
	t_datosConsola* datosConsola;
	int i = 0;
	int cantConsolas = list_size(listaConsola);
	for (i = 0; i < cantConsolas; i++) {
		pthread_mutex_lock(&listadoConsola);
		datosConsola = list_get(listaConsola, i);
		pthread_mutex_unlock(&listadoConsola);
		if (datosConsola->numSocket == socket) {
			return i;
		}
	}
	return -1;
}

int estaEjecutando(int PID){
	int i;
	t_proceso* proceso;
	int cantElemFin = queue_size(colaFinalizar);
	int cantElemListos = queue_size(colaListos);
	for (i = 0; i < strlen((char*)configNucleo.io_ids) / sizeof(char*); i++) {
		pthread_mutex_lock(&cBloqueados);
		proceso = list_get(colas_IO[i]->elements,0);
		pthread_mutex_unlock(&cBloqueados);
		if(proceso->PID == PID){
			return -1;
		}
	}
	for (i = 0; i < cantElemFin; i++) {

		pthread_mutex_lock(&cFinalizar);
		proceso = queue_peek(colaFinalizar);
		pthread_mutex_unlock(&cFinalizar);
		if (proceso->PID == PID){
			return -1;
		}
	}
	for (i = 0; i < cantElemListos; i++) {

		pthread_mutex_lock(&cListos);
		proceso = queue_peek(colaListos);
		pthread_mutex_unlock(&cListos);
		if (proceso->PID == PID){
			return -1;
		}
	}
	return 1;
}

void EntradaSalida(t_nombre_dispositivo dispositivo, int tiempo){
	int i = 0;
	t_bloqueado* infoBloqueado = malloc(sizeof(t_bloqueado));
	log_info(logNucleo,"Envio %s a bloquear por IO\n",dispositivo);
	while (configNucleo.io_ids[i] != NULL){
		if (strcmp(configNucleo.io_ids[i], dispositivo) == 0) {
			infoBloqueado->PID = activePID;
			infoBloqueado->dispositivo = dispositivo;
			infoBloqueado->tiempo = tiempo;

			//Agrego a la cola de Bloqueados
			if (queue_size(colas_IO[i]) == 0) {
				pthread_mutex_lock(&cBloqueados);
				queue_push(colas_IO[i], (void*) infoBloqueado);
				pthread_mutex_unlock(&cBloqueados);

				makeTimer(timers[i], configNucleo.io_ids_values[i] * tiempo);
				return;
			}else{
				pthread_mutex_lock(&cBloqueados);
				queue_push(colas_IO[i], (void*) infoBloqueado);
				pthread_mutex_unlock(&cBloqueados);

				return;
			}
		}
			i++;
		}
}

void atenderIO(int sig, siginfo_t *si, void *uc) {
	int i = 0, io;
	for (i = 0; i < strlen((char*)configNucleo.io_sleep) / sizeof(char*); i++) {
		if (timers[i] == si->si_value.sival_ptr) {
			io = i;
		}
	}
	//printf("deberia entrar una vez\n");
	t_bloqueado *proceso;
	pthread_mutex_lock(&cBloqueados);
	proceso = (t_bloqueado*) queue_pop(colas_IO[io]);
	pthread_mutex_unlock(&cBloqueados);

	//Cambio el proceso a estado ready y agrego a la cola de listos
	log_info(logNucleo,
			"Saco el proceso '%d' de cola IO %d y lo transfiero a cola de Listos \n",
			proceso->PID, io);
	int estado = 1;
	cambiarEstadoProceso(proceso->PID, estado);
	int pos = buscarPCB(proceso->PID);
	if (pos != -1) {
		pthread_mutex_lock(&listadoProcesos);
		t_PCB* datos = (t_PCB*) list_get(listaProcesos, pos);
		pthread_mutex_unlock(&listadoProcesos);

		t_proceso* infoProceso = (t_proceso*) malloc(sizeof(t_proceso));
		infoProceso->PID = datos->PID;
		infoProceso->ProgramCounter = datos->ProgramCounter;
		if (datos->finalizar == 0) {
			pthread_mutex_lock(&cListos);
			queue_push(colaListos, (void*) infoProceso);
			pthread_mutex_unlock(&cListos);
		} else {
			pthread_mutex_lock(&cFinalizar);
			queue_push(colaFinalizar, (void*) infoProceso);
			pthread_mutex_unlock(&cFinalizar);
		}
	}else {
		log_error(logNucleo,"Proceso no encontrado en la lista.\n");
	}

	if (queue_size(colas_IO[io]) != 0) {
		proceso = (t_bloqueado*) list_get(colas_IO[io]->elements, queue_size(colas_IO[io]) - 1);
		makeTimer(timers[io], configNucleo.io_ids_values[io] * proceso->tiempo);
	}
}

static int makeTimer (timer_t *timerID, int expireMS){
	struct sigevent evp;
	struct itimerspec its;
	struct sigaction sa;
	int sigNo = SIGRTMIN;
	sa.sa_flags = SA_SIGINFO|SA_RESTART;

	sa.sa_sigaction = atenderIO;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sigNo, &sa, NULL) == -1) {
		perror("sigaction");
	}
	evp.sigev_notify = SIGEV_SIGNAL;
	evp.sigev_signo = sigNo;
	evp.sigev_value.sival_ptr = timerID;

	timer_create(CLOCK_REALTIME, &evp, timerID);
	its.it_value.tv_sec =  floor((double) (expireMS / 1000));
	its.it_value.tv_nsec = expireMS % 1000 * 1000000;
	timer_settime(*timerID, 0, &its, NULL);

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
		//planificarProceso();
	} else {
		log_error(logNucleo, "Error al liberar CPU \n CPU de socket: %d no encontrado en la lista.\n", socket);
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
		log_error(logNucleo,"Error al cambiar estado de proceso, proceso no encontrado en la lista.\n");
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
		log_error(logNucleo,"Error al cambiar el PC del proceso, proceso no encontrado en la lista.\n");
	}
}

t_valor_variable *obtenerValor(t_nombre_compartida variable) {
	log_info(logNucleo, "Nucleo, obteniendo valor para la variable: %s\n", variable);
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

void grabarValor(t_nombre_compartida variable, t_valor_variable* valor) {
	log_info(logNucleo, "Nucleo, grabando valor: %d para la variable: %s\n", *valor, variable);
	int i = 0;
	while (configNucleo.shared_vars[i] != NULL) {

		if (strcmp(configNucleo.shared_vars[i], variable) == 0) {
			//return (int*)configNucleo.shared_vars[i];
			memcpy(&configNucleo.shared_vars_values[i], valor, sizeof(t_valor_variable));
			return;
		}
		i++;
	}
	log_error(logNucleo,"No se pudo grabar el valor: %d, en la variable: %s \n",valor, variable);
}

int *pideSemaforo(t_nombre_semaforo semaforo) {
	int i = 0;
	int *valorVariable = NULL;

	log_info(logNucleo,"Nucleo, obteniendo semaforo:  %s\n", semaforo);

	while (configNucleo.sem_ids[i] != NULL) {
		if (strcmp(configNucleo.sem_ids[i], semaforo) == 0) {

			//if (configNucleo.sem_ids_values[i] == -1) {return &configNucleo.sem_ids_values[i];}
			//configNucleo.sem_ids_values[i]--;
			return (&configNucleo.sem_ids_values[i]);
		}
		i++;
	}
	log_error(logNucleo, "No se encontro el semaforo: %s \n", semaforo);
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
	log_error(logNucleo,"No se encontro el semaforo: %s \n", semaforo);
}


void liberaSemaforo(t_nombre_semaforo semaforo) {
	int i=0;
	t_proceso *proceso;

	while (configNucleo.sem_ids[i] != NULL){
		if (strcmp(configNucleo.sem_ids[i], semaforo) == 0) {

			if(list_size(colas_semaforos[i]->elements)){
				pthread_mutex_lock(&cSemaforos);
				proceso = queue_pop(colas_semaforos[i]);
				pthread_mutex_unlock(&cSemaforos);

				//Me aseguro que el estado sea Ready lo pongo en la cola de listos
				int estado = 1;
				cambiarEstadoProceso(proceso->PID, estado);
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
	log_error(logNucleo,"No se encontro el semaforo: %s \n", semaforo);
}

void bloqueoSemaforo(int processID, t_nombre_semaforo semaforo){

	log_info(logNucleo,"Procesando el bloqueo por el semaforo: %s, para el PID: %d \n", semaforo, processID);
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
			log_info(logNucleo,"Se bloquea el PID: %d, por semaforo: %s \n",processID, semaforo);
			return;
		}
		i++;
	}
	log_error(logNucleo,"No se encontro el semaforo: %s \n", semaforo);
}

void armarIndiceDeCodigo(t_PCB *unBloqueControl,t_metadata_program* miMetaData){
	int i;
	log_info(logNucleo,"instrucciones_size: %d\n",miMetaData->instrucciones_size);

	//First instruction
	for (i=0; i < miMetaData->instrucciones_size ; i++){
		t_registroIndiceCodigo* registroAAgregar = malloc(sizeof(t_registroIndiceCodigo));
		registroAAgregar->inicioDeInstruccion= miMetaData->instrucciones_serializado[i].start;
		registroAAgregar->longitudInstruccionEnBytes = miMetaData->instrucciones_serializado[i].offset;
		list_add(unBloqueControl->indiceDeCodigo,(void*)registroAAgregar);
	}
	log_info(logNucleo,"size de la lista indice de codigo: %d\n",list_size(unBloqueControl->indiceDeCodigo));
}

void armarIndiceDeEtiquetas(t_PCB *unBloqueControl,t_metadata_program* miMetaData){

	unBloqueControl->indiceDeEtiquetas = string_new();
	unBloqueControl->indiceDeEtiquetasTamanio = miMetaData->etiquetas_size;
	if (unBloqueControl->indiceDeEtiquetasTamanio > 0 ){
		unBloqueControl->indiceDeEtiquetas = malloc(unBloqueControl->indiceDeEtiquetasTamanio);
		memcpy(unBloqueControl->indiceDeEtiquetas, miMetaData->etiquetas, unBloqueControl->indiceDeEtiquetasTamanio);
		log_info(logNucleo,"'sizeindiceDeEtiquetas' : %d\n 'indiceDeEtiquetas' : %s\n", unBloqueControl->indiceDeEtiquetasTamanio, unBloqueControl->indiceDeEtiquetas);
	}

	log_info(logNucleo,"'sizeindiceDeEtiquetas' : %d\n", unBloqueControl->indiceDeEtiquetasTamanio);
}

void finalizarPid(int pid){
	if((pid<0) || (pid >idProcesos)){
		log_error(logNucleo, "Error, PID: %d inexistente.\n", pid);
		return;
	}
	int ret = buscarPCB(pid);
	if (ret == -1){
		log_error(logNucleo,"Error al buscar el proceso de PID: %d.\n", pid);
		return;
	}
	t_PCB* datosProceso;
	pthread_mutex_lock(&listadoProcesos);
	datosProceso = (t_PCB*)list_get(listaProcesos,ret);
	pthread_mutex_unlock(&listadoProcesos);
	switch (datosProceso->estado) {
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

	case 4:
		log_error(logNucleo, "Error, el proceso ya finalizo correctamente.\n");
		break;

	case 5:
		log_error(logNucleo,"Error, el proceso ya finalizo de forma incorrecta.\n");
		break;
	default:
		log_error(logNucleo,"Error, estado invalido.\n");
		break;

	}
}

//TODO la UMC esta recibiendo null del fromProcess. Verificar esta funcion:
void iniciarPrograma(int PID, char *codeScript) {
	log_info(logNucleo,"Para el processID: %d, aviso al proceso UMC el inicio  el programa: \n %s \n", PID, codeScript);
	int bufferSize = 0;
	int payloadSize = 0;
	int contentLen = strlen(codeScript) + 1;	//+1 because of '\0'
	int cantPages = (int) ceil((double) contentLen /(double) frameSize);

	t_MessageNucleo_UMC *message = malloc(sizeof(t_MessageNucleo_UMC));

	message->operation = agregar_proceso;
	message->PID = PID;
	message->cantPages = cantPages + configNucleo.stack_size;

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->cantPages);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize;

	log_info(logNucleo, "payloadSize a enviar a la UMC: %d", payloadSize);
	log_info(logNucleo, "bufferSize a enviar a la UMC: %d", bufferSize);

	char *buffer = malloc(bufferSize);

	//Serializar mensaje
	serializeNucleo_UMC(message, buffer, payloadSize);

	//1) Enviar info a la UMC - message serializado
	sendMessage(&socketUMC, buffer, bufferSize);

	//2) Despues de enviar la info, Enviar el tamanio del prgrama
	sendMessage(&socketUMC, (void*) contentLen, sizeof(contentLen));

	//3) Enviar programa
	string_append(&codeScript, "\0");	//ALWAYS put \0 for finishing the string
	sendMessage(&socketUMC, (void*) codeScript, contentLen);

	free(message);
	free(buffer);
	//TODO free(codeScript);
	log_info(logNucleo,"Se realiza correctamente el envio del programa al proceso UMC");

}

void finalizarPrograma(int PID){
	log_info(logNucleo,"Aviso al proceso UMC que se finalice el programa cuyo processID asignado es: %d \n", PID);
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

	//Se esta enviando el tamanio del dispositivo que es un char* y se esta considerando el \0 en el offset
}

void crearArchivoDeConfiguracion(char *configFile){
	log_info (logNucleo, "Creando el archivo de configuracion: %s \n", configFile);
	t_config* configuration;

	configuration = config_create(configFile);
	configNucleo.puerto_prog = config_get_int_value(configuration,"PUERTO_PROG");
	configNucleo.puerto_cpu = config_get_int_value(configuration,"PUERTO_CPU");
	configNucleo.ip_umc = config_get_string_value(configuration,"IP_UMC");
	configNucleo.puerto_umc = config_get_int_value(configuration,"PUERTO_UMC");
	configNucleo.quantum = config_get_int_value(configuration,"QUANTUM");
	printf("quantum = %d \n", configNucleo.quantum);
	configNucleo.quantum_sleep = config_get_int_value(configuration,"QUANTUM_SLEEP");
	printf("quantum_sleep = %d \n", configNucleo.quantum_sleep);
	configNucleo.sem_ids = config_get_array_value(configuration,"SEM_IDS");
	//printf("sem_ids = ");
	//imprimirArray(configNucleo.sem_ids);
	configNucleo.sem_init = config_get_array_value(configuration,"SEM_INIT");
	//printf("sem_init = ");
	//imprimirArray(configNucleo.sem_init);
	configNucleo.io_ids = config_get_array_value(configuration,"IO_IDS");
	configNucleo.io_sleep = config_get_array_value(configuration,"IO_SLEEP");
	printf("io_sleep = ");
	imprimirArray(configNucleo.io_sleep);
	configNucleo.shared_vars = config_get_array_value(configuration,"SHARED_VARS");
	//printf("shared_vars = ");
	//imprimirArray(configNucleo.shared_vars);
	configNucleo.stack_size = config_get_int_value(configuration,"STACK_SIZE");
	printf("stack_size = %d \n", configNucleo.stack_size);

	int i = 0;
	int len = 0;

	if(timers!=0){
		free(timers);
	}
	len = (int) strlen((char*)configNucleo.io_sleep);
	timers = initialize(len * sizeof(char*));
	if(colas_IO!=0){
		free(colas_IO);
	}
	colas_IO = initialize(len * sizeof(char*));
	//printf("*timers = ");
	while (configNucleo.io_sleep[i] != NULL){
		timers[i] = initialize(sizeof(timer_t));
		//imprimirTimer(timers, i, len);
		colas_IO[i] = initialize(sizeof(t_queue*));
		colas_IO[i] = queue_create();
		i++;
	}
	//printf("\n");

	//initializing (0 default values) shared variables values (int*)
	len  = (int) strlen((char*)configNucleo.shared_vars) / sizeof(char*);
	configNucleo.shared_vars_values = initialize(len * sizeof(int));
	printf("shared_vars_values = ");
	i=0;
	while (configNucleo.shared_vars[i] != NULL) {
		imprimirValores(configNucleo.shared_vars_values,i, len);
		i++;
	}
	//initializing io_ids_values (int*)
	len  = (int) strlen((char*)configNucleo.io_sleep) / sizeof(char*);
	configNucleo.io_ids_values= initialize(len * sizeof(int));
	printf("io_ids_values = ");
	i=0;
	while (configNucleo.io_sleep[i] != NULL) {
		configNucleo.io_ids_values[i] = atoi(configNucleo.io_sleep[i]);
		imprimirValores(configNucleo.io_ids_values,i, len);
		i++;
	}
	//initializing sem_ids_values (int*)
	len  = (int) strlen((char*)configNucleo.io_sleep) / sizeof(char*);
	configNucleo.sem_ids_values= initialize(len * sizeof(int));
	printf("sem_ids_values = ");
	i=0;
	while (configNucleo.sem_init[i] != NULL) {
		configNucleo.sem_ids_values[i] = atoi(configNucleo.sem_init[i]);
		imprimirValores(configNucleo.sem_ids_values,i, len);
		i++;
	}

	if(colas_semaforos!=0){
		free(colas_semaforos);
	}
	len = (int) strlen((char*)configNucleo.sem_init);//TODO len esta retornando 0
	colas_semaforos = initialize(len * sizeof(char*));

	i = 0;
	while (configNucleo.sem_init [i] != NULL){
		colas_semaforos[i] = initialize(sizeof(t_queue*));
		colas_semaforos[i] = queue_create();
		i++;
	}
	log_info(logNucleo, "El archivo de configuracion: '%s' se creo correctamente \n", configFile);
}

void *initialize(int tamanio){
	int i;
	void * retorno = malloc(tamanio);
	for(i=0;i<tamanio;i++){
		((char*)retorno)[i]=0;
	}
	return retorno;
}

void imprimirArray(char** array){
	int i = 0;
	while(array[i] != NULL) {
		if (i==0){
			printf("[%s", array[i]);
			printf(", ");
		}else if (i == (strlen((char*)array)/ sizeof(char*))-1){
			printf("%s]", array[i]);
		}else{
			printf("%s", array[i]);
			printf(", ");
		}
		i++;
	}
	printf(" \n");
}

void imprimirValores(int* array, int i, int arrayLen){
	if (i==0){
		printf("[%d",array[i]);
		printf(", ");
	}else if (i == arrayLen - 1){
		printf("%d]", array[i]);
		printf(" \n");
	}else{
		printf("%d", array[i]);
		printf(", ");
	}
}

void imprimirTimer(timer_t** timer, int i, int arrayLen){
	if (i==0){
		printf("[%li", (time_t)(*timers[i]));
		printf(", ");
	}else if(i == arrayLen - 1){
		printf("%li]", (time_t)(*timers[i]));
	}
	else{
		printf("%li", (time_t)(*timers[i]));
	}
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

void startNucleoConsole(){
	char command[50];
	char option[50];

	consoleMessageNucleo();

	while (1){
		scanf("%s %s", command, option);

		//cleaning screen
		system("clear");

		int i;
		//lower case options
		for(i = 0; option[i] != '\0'; i++){
			option[i] = tolower(option[i]);
		}

		if (strcmp(command,"quantum") == 0 ){
			pthread_mutex_lock(&mutex_config);
			configNucleo.quantum = atoi(option);
			pthread_mutex_unlock(&mutex_config);
			printf("The quantum Nucleo was successfully changed to: %d\n", configNucleo.quantum);
			log_info(logNucleo, "The quantum Nucleo was successfully changed to: %d\n", configNucleo.quantum);

			printf("\nAhora por favor ingrese un valor de quantum sleep\n");
			printf("===> quantum_sleep = [OPTIONS]\n");
			printf("== [OPTIONS]\n");
			printf("<numericValue>\t::Cantidad de milisegundos por QUANTUM que el sistema debe esperar por cada ejecucion de QUANTUM\n\n");
			printf("[OPTIONS] =  ");
			scanf("%s", option);

			pthread_mutex_lock(&mutex_config);
			configNucleo.quantum_sleep = atoi(option);
			pthread_mutex_unlock(&mutex_config);
			printf("The quantum sleep in Nucleo was successfully changed to: %d\n", configNucleo.quantum_sleep);

			log_info(logNucleo, "The quantum sleep in Nucleo was successfully changed to: %d\n", configNucleo.quantum_sleep);

		}else{
			printf("\nCommand entered NOT recognized: '%s %s'\n", command,option);
			printf("Please take a look to the correct commands!\n");
		}

		consoleMessageNucleo();

	}

}

void consoleMessageNucleo(){
	printf("\n***********************************\n");
	printf("* Nucleo Console ready for your use! *");
	printf("\n***********************************\n\n");
	printf("COMMANDS USAGE:\n\n");

	printf("===> COMMAND:\tquantum [OPTIONS]\n");
	printf("== [OPTIONS]\n");
	printf("<numericValue>\t::Cantidad de QUANTUMS por solicitud de ejecucion\n\n");

	printf("Nucleo console >> $ ");
}
