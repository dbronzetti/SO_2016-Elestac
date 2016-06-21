/*
 * nucleo.c
 *
 */

#include "nucleo.h"

int main(int argc, char **argv) {

	int exitCode = EXIT_FAILURE; //DEFAULT failure
	pthread_t serverThread;
	//Creo archivo de configuracion
	crearArchivoDeConfiguracion();
	//Creo el archivo de Log
		logNucleo = log_create("logNucleo", "TP", 0, LOG_LEVEL_TRACE);
	//Creo la lista de CPUs
		listaCPU = list_create();
	//Creo Lista Procesos
		listaProcesos = list_create();
	//Creo la Cola de Listos
		colaListos = queue_create();
	//Creo la Cola de Bloqueados
		colaBloqueados = queue_create();
	//Creo cola de Procesos a Finalizar (por Finalizar PID).
		colaFinalizar = queue_create();

	pthread_mutex_init(&socketMutex, NULL);

	//Create thread for server start
	pthread_create(&serverThread, NULL, (void*) startServer, NULL);
	pthread_join(serverThread, NULL);

	return exitCode;
}

void startServer(){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	t_serverData serverData;

	exitCode = openServerConnection(configNucleo.puerto_prog, &serverData.socketServer);
	printf("socketServer: %d\n",serverData.socketServer);

	//If exitCode == 0 the server connection is opened and listening
	if (exitCode == 0) {
		puts ("the server is opened");

		exitCode = listen(serverData.socketServer, SOMAXCONN);

		if (exitCode < 0 ){
			perror("Failed to listen server Port"); //TODO => Agregar logs con librerias
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
	//TODO disparar un thread para acceptar cada cliente nuevo (debido a que el accept es bloqueante) y para hacer el handshake

	exitCode = acceptClientConnection(&serverData->socketServer, &serverData->socketClient);

	if (exitCode == EXIT_FAILURE){
		printf("There was detected an attempt of wrong connection\n");//TODO => Agregar logs con librerias
	}else{
		//TODO posiblemente aca haya que disparar otro thread para que haga el recv y continue recibiendo conexiones al mismo tiempo
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
		perror("The client went down while handshaking!"); //TODO => Agregar logs con librerias
		printf("Please check the client: %d is down!\n", serverData->socketClient);
		log_info(logNucleo, "Se conecto la CPU %d", message->message);
		close(serverData->socketClient);
	}else{
		switch ((int) message->process){
			case CPU:{
				printf("%s\n",message->message);
				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){
					t_MessageNucleo_CPU *datosCPU = malloc(sizeof(t_MessageNucleo_CPU));
					datosCPU->id = numCPU;
					datosCPU->numSocket = serverData->socketClient;
					list_add(listaCPU, (void*)datosCPU);
					numCPU++;

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
			case CONSOLA:{
				printf("%s\n",message->message);
				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){

					//After sending ACCEPTATION has to be sent the "Tamanio de pagina" information
					exitCode = sendMessage(&serverData->socketClient, &configNucleo.frames_size , sizeof(configNucleo.frames_size));


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
			case UMC:{
				printf("%s\n",message->message);
				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){

					//After sending ACCEPTATION has to be sent the "Tamanio de pagina" information
					exitCode = sendMessage(&serverData->socketClient, &configNucleo.frames_size , sizeof(configNucleo.frames_size));
				}
			break;
			}
			default:{
				perror("Process not allowed to connect");//TODO => Agregar logs con librerias
				printf("Invalid process '%d' tried to connect to UMC\n",(int) message->process);
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
	int exitCode = EXIT_FAILURE; //DEFAULT Failure

	t_serverData *serverData = (t_serverData*) parameter;

	//Receive message size
	int messageSize = 0;
	char *messageRcv = malloc(sizeof(messageSize));
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(messageSize));

	if ( receivedBytes > 0 ){
		messageSize = atoi(messageRcv);

		//Receive process from which the message is going to be interpreted
		enum_processes fromProcess;
		messageRcv = realloc(messageRcv, sizeof(fromProcess));
		receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(fromProcess));

		//Receive message using the size read before
		fromProcess = (enum_processes) messageRcv;
		messageRcv = realloc(messageRcv, messageSize);
		receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

		printf("bytes received: %d\n",receivedBytes);

		switch (fromProcess){
			case CONSOLA:{
				printf("Processing CONSOLA message received\n");
				t_MessageNucleo_Consola *message = malloc(sizeof(t_MessageNucleo_Consola));
				deserializeConsola_Nucleo(message, messageRcv);
				printf("se recibio el processID #%d\n", message->processID);
				free(message);
				runScript(messageRcv);
				break;
			}
			case CPU: {
				printf("Processing CPU message received\n");
				t_MessageNucleo_CPU *message;
				char *messageRcv = (char*) malloc(sizeof(t_MessageNucleo_CPU));
				char *datosEntradaSalida = malloc(sizeof(t_es));
				t_es infoES;
				memset(messageRcv, '\0', sizeof(t_MessageNucleo_CPU));
				recv(serverData->socketClient,(void*)messageRcv,sizeof(t_MessageNucleo_CPU),0);
				deserializeCPU_Nucleo(message, messageRcv);
				switch (message->operacion) {
					case 1: //Entrada Salida
						recv(serverData->socketClient, (void*) datosEntradaSalida, sizeof(t_es),MSG_WAITALL);
						deserializeIO(&infoES, &datosEntradaSalida);
						hacerEntradaSalida(serverData->socketClient, message->processID,infoES.ProgramCounter, infoES.tiempo);
						break;
					case 2: //Finaliza Proceso Bien
						finalizaProceso(serverData->socketClient, message->processID,message->operacion);
						break;
					case 3: //Finaliza Proceso Mal
						finalizaProceso(serverData->socketClient, message->processID,message->operacion);
						break;
					case 4:	//Falla otra cosa
						printf("Hubo un fallo.\n");
						break;
					case 5: //Corte por Quantum
						printf("Corto por Quantum.\n");
						atenderCorteQuantum(serverData->socketClient, message->processID);
						break;
					default:
						printf("Mensaje recibido invalido, CPU desconectado.\n");
						abort();
				} //fin del switch interno
				printf("Processing CPU message received\n");
				break;
			}
			case UMC:{
				printf("Processing UMC message received\n");
				break;
			}
			default:{
				perror("Process not allowed to connect");//TODO => Agregar logs con librerias
				printf("Invalid process '#%d' tried to connect to NUCLEO \n",(int) fromProcess);
				close(serverData->socketClient);
				break;
			}
		}

	}else if (receivedBytes == 0 ){
		//The client is down when bytes received are 0
		perror("One of the clients is down!"); //TODO => Agregar logs con librerias
		printf("Please check the client: %d is down!\n", serverData->socketClient);
	}else{
		perror("Error - No able to received");//TODO => Agregar logs con librerias
		printf("Error receiving from socket '%d', with error: %d\n",serverData->socketClient,errno);
	}

	free(messageRcv);

}

void runScript(char* codeScript){
	//Creo el PCB del proceso.
	t_PCB* PCB = malloc(sizeof(t_PCB));
	t_proceso* datosProceso = malloc(sizeof(t_proceso));

	PCB->PID = idProcesos;

//	memcpy(PCB->codeScript,codeScript,sizeof((void*)codeScript));	TODO arreglar error de codeScript
	PCB->ProgramCounter = 0;
	PCB->estado=1;
	idProcesos++;
	PCB->finalizar = 0;
	list_add(listaProcesos,(void*)PCB);
	log_info(logNucleo, "myProcess %d - Iniciado  Script: %s",PCB->PID, codeScript);

	//Agrego a la Cola de Listos
	datosProceso->ProgramCounter = PCB->ProgramCounter;
	datosProceso->PID = PCB->PID;

	pthread_mutex_lock(&cListos);
	queue_push(colaListos, (void*)datosProceso);
	pthread_mutex_unlock(&cListos);

	planificarProceso();

}

void atenderCorteQuantum(int socket,int PID){
	//Libero la CPU
	liberarCPU(socket);

	//Cambio el PC del Proceso, le sumo el quantum al PC actual.
	t_PCB* infoProceso;
	int buscar = buscarPCB(PID);
	infoProceso = (t_PCB*)list_get(listaProcesos,buscar);
	int pcnuevo;
	pcnuevo = infoProceso->ProgramCounter + configNucleo.quantum;
	actualizarPC(PID,pcnuevo);
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
	}else {
			pthread_mutex_lock(&cFinalizar);
			queue_push(colaFinalizar,(void*)datosProceso);
			pthread_mutex_unlock(&cFinalizar);
	}

	//Mando nuevamente a PLanificar
	planificarProceso();
}

void finalizaProceso(int socket, int PID, int estado) {

	int posicion = buscarPCB(PID);
	int estadoProceso;
	//Cambio el estado del proceso, y guardo en Log que finalizo correctamente
	t_PCB* datosProceso;
	datosProceso = (t_PCB*) list_get(listaProcesos, posicion);
	if (estado == 2) {
		estadoProceso = 4;
		cambiarEstadoProceso(PID, estadoProceso);
		log_info(logNucleo, "myProcess %d - Finalizo correctamente",
				datosProceso->PID);
	} else {
		if (estado == 3) {
			estadoProceso = 5;
			cambiarEstadoProceso(PID, estadoProceso);
			log_info(logNucleo,
					"myProcess %d - Finalizo incorrectamente por falta de espacio en Swap",
					datosProceso->PID);
		}
	}
	//Libero la CPU
	liberarCPU(socket);

	//Mando a revisar si hay alguno en la lista para ejecutar.
	planificarProceso();

}

void planificarProceso() {
	//Veo si hay procesos para planificar en la cola de Listos
	if (queue_is_empty(colaListos) && (queue_is_empty(colaFinalizar))) {
		return;
	}
	//Veo si hay CPU libre para asignarle
	int libre = buscarCPULibre();

	if (libre == -1) {
		printf("No hay CPU libre.\n");
		return;
	}

	//Le envio la informacion a la CPU
	t_PCB* datosPCB;
	t_MessageNucleo_CPU contextoProceso;
	t_proceso* datosProceso;
	contextoProceso.head = 0;
	contextoProceso.cantInstruc = 0;

	if (queue_is_empty(colaFinalizar)) {

		pthread_mutex_lock(&cListos);
		datosProceso = (t_proceso*) queue_peek(colaListos);
		pthread_mutex_unlock(&cListos);

		int posicion = buscarPCB(datosProceso->PID);
		if (posicion != -1) {
			datosPCB = (t_PCB*) list_get(listaProcesos, posicion);
			char* bufferEnviar = malloc(sizeof(t_MessageNucleo_CPU));

			contextoProceso.cantInstruc = configNucleo.quantum;

			contextoProceso.ProgramCounter = datosPCB->ProgramCounter;
//			strcpy(contextoProceso.codeScript, datosPCB->codeScript);  TODO arreglar error de codeScript
			contextoProceso.processID = datosPCB->PID;
			//Saco el primer elemento de la cola, porque ya lo planifique.
			pthread_mutex_lock(&cListos);
			queue_pop(colaListos);
			pthread_mutex_unlock(&cListos);

			send(libre, bufferEnviar, sizeof(t_MessageNucleo_CPU), 0);
			//Cambio Estado del Proceso
			int estado = 2;
			cambiarEstadoProceso(datosPCB->PID, estado);

		} else {
			printf("Proceso no encontrado en la lista.\n");
		}
	} else {
		pthread_mutex_lock(&cFinalizar);
		datosProceso = (t_proceso*) queue_peek(colaFinalizar);
		pthread_mutex_unlock(&cFinalizar);
		int posicion = buscarPCB(datosProceso->PID);
		if (posicion != -1) {
			datosPCB = (t_PCB*) list_get(listaProcesos, posicion);
			char* bufferEnviar = malloc(sizeof(t_MessageNucleo_CPU));

			contextoProceso.head = 1;

			contextoProceso.ProgramCounter = datosPCB->ProgramCounter;
//			strcpy(contextoProceso.codeScript, datosPCB->codeScript);   TODO arreglar error de codeScript
			contextoProceso.processID = datosPCB->PID;
			serializeNucleo_CPU(&contextoProceso, bufferEnviar, sizeof(t_MessageNucleo_CPU));
			//Saco el primer elemento de la cola, porque ya lo planifique.
			pthread_mutex_lock(&cFinalizar);
			queue_pop(colaFinalizar);
			pthread_mutex_unlock(&cFinalizar);

			send(libre, bufferEnviar, sizeof(t_MessageNucleo_CPU), 0);
			//Cambio Estado del Proceso
			int estado = 2;
			cambiarEstadoProceso(datosPCB->PID, estado);
		}
	}

}

int buscarCPULibre() {
	int cantCPU, i = 0;
	t_MessageNucleo_CPU* datosCPU;
	cantCPU = list_size(listaCPU);
	for (i = 0; i < cantCPU; i++) {
		datosCPU = (t_MessageNucleo_CPU*) list_get(listaCPU, i);
		if (datosCPU->processStatus == NEW) {
			datosCPU->processStatus = READY;
			return datosCPU->numSocket;
		}
	}
	return -1;
}

int buscarPCB(int id) {
	t_PCB* datosPCB;
	int i = 0;
	int cantPCB = list_size(listaProcesos);
	for (i = 0; i < cantPCB; i++) {
		datosPCB = list_get(listaProcesos, i);
		if (datosPCB->PID == id) {
			return i;
		}
	}
	return -1;
}

int buscarCPU(int socket) {
	t_MessageNucleo_CPU* datosCPU;
	int i = 0;
	int cantCPU = list_size(listaCPU);
	for (i = 0; i < cantCPU; i++) {
		datosCPU = list_get(listaCPU, i);
		if (datosCPU->numSocket == socket) {
			return i;
		}
	}
	return -1;
}

void hacerEntradaSalida(int socket, int PID, int ProgramCounter, int tiempo) {

	t_bloqueado* infoBloqueado = malloc(sizeof(t_bloqueado));
//Libero la CPU que ocupaba el proceso
	liberarCPU(socket);
//Cambio el estado del proceso
	int estado = 3;
	cambiarEstadoProceso(PID, estado);
//Cambio el PC del Proceso
	actualizarPC(PID, ProgramCounter);
//Agrego a la cola de Bloqueados, y seteo el semaforo
	infoBloqueado->PID = PID;
	infoBloqueado->tiempo = tiempo;

	pthread_mutex_lock(&cBloqueados);
	queue_push(colaBloqueados, (void*) infoBloqueado);
	pthread_mutex_unlock(&cBloqueados);

	sem_post(&semBloqueados);
}

void liberarCPU(int socket) {
	int liberar = buscarCPU(socket);
	if (liberar != -1) {
		t_MessageNucleo_CPU* datosCPU;
		datosCPU = (t_MessageNucleo_CPU*) list_get(listaCPU, liberar);
		datosCPU->processStatus = NEW;
		planificarProceso();
	} else {
		printf("Error al liberar CPU \n CPU no encontrada en la lista.\n");
	}
}

void cambiarEstadoProceso(int PID, int estado) {
	int cambiar = buscarPCB(PID);
	if (cambiar != -1) {
		t_PCB* datosProceso;
		datosProceso = (t_PCB*) list_get(listaProcesos, cambiar);
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
		//Agrego a la Lista de Listos el Proceso
		int buscar = buscarPCB(datosProceso->PID);
		t_PCB * informacionDeProceso = list_get(listaProcesos, buscar);
		t_proceso* infoProceso = (t_proceso*) malloc(sizeof(t_proceso));
		t_PCB* datos;
		if (buscar != -1) {
			datos = (t_PCB*) list_get(listaProcesos, buscar);
			infoProceso->PID = datos->PID;
			infoProceso->ProgramCounter = datos->ProgramCounter;
			pthread_mutex_lock(&cBloqueados);
			queue_pop(colaBloqueados);
			pthread_mutex_unlock(&cBloqueados);
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
		datosProceso = (t_PCB*) list_get(listaProcesos, cambiar);
		datosProceso->ProgramCounter = ProgramCounter;
	} else {
		printf("Error al cambiar el PC del proceso, proceso no encontrado en la lista.\n");
	}
}

void deserializeIO(t_es* datos, char** buffer) {
	int offset = 0;
	memcpy(&datos->tiempo, *buffer, sizeof(datos->tiempo));
	offset += sizeof(datos->tiempo);

	memcpy(&datos->ProgramCounter, *buffer + offset, sizeof(datos->ProgramCounter));
	offset += sizeof(datos->ProgramCounter);
}

void crearArchivoDeConfiguracion(char *configFile){
	t_config* configuration;
	configuration = config_create("/home/utnso/git/tp-2016-1c-YoNoFui/nucleo/configuracion.nucleo");
	configNucleo.puerto_prog = config_get_int_value(configuration,"PUERTO_PROG");
	configNucleo.puerto_cpu = config_get_int_value(configuration,"PUERTO_CPU");
	configNucleo.quantum = config_get_int_value(configuration,"QUANTUM");
	configNucleo.quantum_sleep = config_get_int_value(configuration,"QUANTUM_SLEEP");
	configNucleo.sem_ids = (void*) config_get_array_value(configuration,"SEM_IDS");
	configNucleo.sem_init = (void*) config_get_array_value(configuration,"SEM_INIT");
	configNucleo.io_ids = (void*) config_get_array_value(configuration,"IO_IDS");
	configNucleo.io_sleep = (void*) config_get_array_value(configuration,"IO_SLEEP");
	configNucleo.shared_vars = (void*) config_get_array_value(configuration,"SHARED_VARS");
	configNucleo.stack_size = config_get_int_value(configuration,"STACK_SIZE");
	configNucleo.frames_size = config_get_int_value(configuration,"FRAMES_SIZE");
}

