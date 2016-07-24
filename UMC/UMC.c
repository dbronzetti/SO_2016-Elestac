#include "UMC.h"

int main(int argc, char *argv[]){
	int exitCode = EXIT_FAILURE ; //DEFAULT failure
	char *configurationFile = NULL;
	char *logFile = NULL;
	pthread_t serverThread;
	pthread_t consolaThread;

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS

	//get parameter
	int i;
	for( i = 0; i < argc; i++){
		//check Configuration file parameter
		if (strcmp(argv[i], "-c") == 0){
			configurationFile = argv[i+1];
			printf("Configuration File: '%s'\n",configurationFile);
		}
		//check Dump file parameter
		if (strcmp(argv[i], "-d") == 0){
			dumpFile = argv[i+1];
			printf("Dump File: '%s'\n",dumpFile);
		}
		//check log file parameter
		if (strcmp(argv[i], "-l") == 0){
			logFile = argv[i+1];
			printf("Log File: '%s'\n",logFile);
		}
	}

	//ERROR if not configuration parameter was passed
	assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS

	//ERROR if not dump parameter was passed
	assert(("ERROR - NOT dump file was passed as argument", dumpFile != NULL));//Verifies if was passed the Dump file as parameter, if DONT FAILS

	//ERROR if not log parameter was passed
	assert(("ERROR - NOT log file was passed as argument", logFile != NULL));//Verifies if was passed the Dump file as parameter, if DONT FAILS

	//Creating log file
	UMCLog = log_create(logFile, "UMC", 0, LOG_LEVEL_TRACE);

	//get configuration from file
	getConfiguration(configurationFile);

	//created administration structures for UMC process
	createAdminStructs();

	//Connecting to SWAP before starting everything else
	exitCode = connectTo(SWAP,&socketSwap);

	if(exitCode == EXIT_SUCCESS){
		log_info(UMCLog, "UMC connected to SWAP successfully\n");

		//Initializing mutex
		pthread_mutex_init(&memoryAccessMutex, NULL);
		pthread_mutex_init(&activeProcessMutex, NULL);

		//Create thread for UMC console
		pthread_create(&consolaThread, NULL, (void*) startUMCConsole, NULL);

		//Create thread for server start
		pthread_create(&serverThread, NULL, (void*) startServer, NULL);

		pthread_join(consolaThread, NULL);
		pthread_join(serverThread, NULL);
	}

	return exitCode;
}

void startServer(){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	int socketServer;

	exitCode = openServerConnection(configuration.port, &socketServer);
	log_info(UMCLog, "SocketServer: %d\n", socketServer);

	//If exitCode == 0 the server connection is opened and listening
	if (exitCode == 0) {
		log_info(UMCLog, "The server is opened.\n");

		exitCode = listen(socketServer, SOMAXCONN);

		if (exitCode < 0 ){
			log_error(UMCLog,"Failed to listen server Port.\n");
			return;
		}

		while (1){
			newClients((void*) &socketServer);
		}
	}

}

void newClients (void *parameter){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	int pid;

	t_serverData *serverData = malloc(sizeof(t_serverData));
	memcpy(&serverData->socketServer, parameter, sizeof(serverData->socketServer));

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
		log_warning(UMCLog,"There was detected an attempt of wrong connection\n");
		close(serverData->socketClient);
		free(serverData);
	}else{

		//Create thread attribute detached
		pthread_attr_t handShakeThreadAttr;
		pthread_attr_init(&handShakeThreadAttr);
		pthread_attr_setdetachstate(&handShakeThreadAttr, PTHREAD_CREATE_DETACHED);

		//Create thread for checking new connections in server socket
		pthread_t handShakeThread;
		pthread_create(&handShakeThread, &handShakeThreadAttr, (void*) handShake, serverData);

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
		log_error(UMCLog,"The client went down while handshaking! - Please check the client '%d' is down!\n", serverData->socketClient);
		close(serverData->socketClient);
		free(serverData);
	}else{
		switch ((int) message->process){
			case CPU:{
				log_info(UMCLog,"Message from '%s': %s\n", getProcessString(message->process), message->message);

				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){

					//After sending ACCEPTATION has to be sent the "Tamanio de pagina" information
					exitCode = sendMessage(&serverData->socketClient, &configuration.frames_size , sizeof(configuration.frames_size));

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
			case NUCLEO:{
				log_info(UMCLog,"Message from '%s': %s\n", getProcessString(message->process), message->message);

				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){

					//After sending ACCEPTATION has to be sent the "Tamanio de pagina" information
					exitCode = sendMessage(&serverData->socketClient, &configuration.frames_size , sizeof(configuration.frames_size));

					//processMessageReceived(parameter);

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
				log_error(UMCLog,"Process not allowed to connect - Invalid process '%s' tried to connect to UMC\n", getProcessString(message->process));
				close(serverData->socketClient);
				free(serverData);
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

	while(1){
		//Receive message size
		int messageSize = 0;

		//Get Payload size
		int receivedBytes = receiveMessage(&serverData->socketClient, &messageSize, sizeof(messageSize));
		log_info(UMCLog, "message size received: %d \n", messageSize);

		if ( receivedBytes > 0 ){

			printf("socket cliente: %d\n", serverData->socketClient);

			//Receive process from which the message is going to be interpreted
			enum_processes fromProcess;
			receivedBytes = receiveMessage(&serverData->socketClient, &fromProcess, sizeof(fromProcess));

			log_info(UMCLog,"Bytes received from process '%s': %d\n",getProcessString(fromProcess),receivedBytes);

			switch (fromProcess){
			case CPU:{
				log_info(UMCLog, "Processing CPU message received\n");
				pthread_mutex_lock(&activeProcessMutex);
				procesCPUMessages(messageSize, serverData);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			case NUCLEO:{
				log_info(UMCLog, "Processing NUCLEO message received\n");
				pthread_mutex_lock(&activeProcessMutex);
				procesNucleoMessages(messageSize, serverData);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			default:{
				log_error(UMCLog,"Process not allowed to connect - Invalid process '%s' send a message to UMC\n", getProcessString(fromProcess));
				close(serverData->socketClient);
				free(serverData);
				break;
			}
			}

		}else if (receivedBytes == 0 ){
			//The client is down when bytes received are 0
			log_error(UMCLog,"The client went down while receiving! - Please check the client '%d' is down!\n", serverData->socketClient);
			close(serverData->socketClient);
			free(serverData);
			break;
		}else{
			log_error(UMCLog, "Error - No able to received - Error receiving from socket '%d', with error: %d\n",serverData->socketClient,errno);
			close(serverData->socketClient);
			free(serverData);
			break;
		}

	}

}

void procesCPUMessages(int messageSize, t_serverData* serverData){
	int exitcode = EXIT_SUCCESS;

	//Receive message using the size read before
	char *messageRcv = malloc(messageSize);
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

	t_MessageCPU_UMC *message = malloc(sizeof(t_MessageCPU_UMC));
	message->virtualAddress = (t_memoryLocation *)malloc(sizeof(t_memoryLocation));

	//Deserialize messageRcv
	deserializeUMC_CPU(message, messageRcv);

	changeActiveProcess(message->PID);

	void *content = NULL;

	switch (message->operation){
		case lectura_pagina:{

			content = malloc(message->virtualAddress->size);

			content = requestBytesFromPage(message->virtualAddress);

			if(content != NULL){
				//After getting the memory content WITHOUT issues, inform status of the operation
				sendMessage(&serverData->socketClient, &exitcode, sizeof(exitcode));
				//has to be sent it to upstream - on the other side the process already know the size to received
				sendMessage(&serverData->socketClient, content, message->virtualAddress->size);
			}else{
				//The main memory hasn't any free frames - inform status of the operation
				sendMessage(&serverData->socketClient, &exitcode, sizeof(exitcode));
			}

			free(content);

			break;
		}
		case escritura_pagina:{

			//Receive content size
			messageSize = 0;//reseting size to get the content size
			receivedBytes = 0;//reseting receivedBytes to get the content size

			receivedBytes = receiveMessage(&serverData->socketClient, &messageSize, sizeof(messageSize));

			//Receive content using the size read before
			content = malloc(messageSize);
			receivedBytes = receiveMessage(&serverData->socketClient, content, messageSize);

			exitcode = writeBytesToPage(message->virtualAddress, content);

			if(content != NULL){
				//After getting the memory content WITHOUT issues, inform status of the operation
				sendMessage(&serverData->socketClient, &exitcode, sizeof(exitcode));
			}else{
				//The main memory hasn't any free frames - inform status of the operation
				sendMessage(&serverData->socketClient, &exitcode, sizeof(exitcode));
			}

			free(content);

			break;
		}
		default:{
			log_error(UMCLog,"Process not allowed to connect - Invalid operation for CPU messages '%d' requested to UMC \n",(int) message->operation);
			break;
		}
	}
}

void procesNucleoMessages(int messageSize, t_serverData* serverData){
	int exitcode = EXIT_SUCCESS;

	//Receive message using the size read before
	char *messageRcv = malloc(messageSize);
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

	t_MessageNucleo_UMC *message = malloc(sizeof(t_MessageNucleo_UMC));

	//Deserialize messageRcv
	deserializeUMC_Nucleo(message, messageRcv);

	changeActiveProcess(message->PID);

	switch (message->operation){
		case agregar_proceso:{

			//Receive content size
			messageSize = 0;//reseting size to get the content size
			receivedBytes = 0;//reseting receivedBytes to get the content size

			receivedBytes = receiveMessage(&serverData->socketClient, &messageSize, sizeof(messageSize));
			log_info(UMCLog, "message size received: %d \n", messageSize);

			//Receive content using the size read before
			void *content = malloc(messageSize);
			receivedBytes = receiveMessage(&serverData->socketClient, content, messageSize);
			log_info(UMCLog, "message received: %s \n", content);

			initializeProgram(message->PID, message->cantPages, content);

			free(content);
			break;
		}
		case finalizar_proceso:{

			endProgram(message->PID);

			break;
		}
		default:{
			log_error(UMCLog, "Process not allowed to connect - Invalid operation for CPU messages '%d' requested to UMC \n",(int) message->operation);
			close(serverData->socketClient);
			free(serverData);
			break;
		}
	}
}

int connectTo(enum_processes processToConnect, int *socketClient){
	int exitcode = EXIT_FAILURE;//DEFAULT VALUE
	int port = 0;
	char *ip = string_new();

	switch (processToConnect){
		case SWAP:{
			 string_append(&ip,configuration.ip_swap);
			 port= configuration.port_swap;
			break;
		}
		default:{
			log_info(UMCLog,"Process '%s' NOT VALID to be connected by UMC.\n", getProcessString(processToConnect));
			break;
		}
	}

	exitcode = openClientConnection(ip, port, socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == EXIT_SUCCESS){

		// ***1) Send handshake
		exitcode = sendClientHandShake(socketClient,UMC);

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
							case SWAP:{
								log_info(UMCLog, "Connected to SWAP - Message: %s\n",message->message);
								break;
							}
							default:{
								log_error(UMCLog, "Handshake not accepted when tried to connect your '%s' with '%s'\n",getProcessString(processToConnect),getProcessString(message->process));
								close(*socketClient);
								exitcode = EXIT_FAILURE;
								break;
							}
						}

						break;
					}
					default:{
						log_error(UMCLog, "Process couldn't connect to SERVER - Not able to connect to server %s. Please check if it's down.\n",ip);
						close(*socketClient);
						break;
						break;
					}
				}

			}else if (receivedBytes == 0 ){
				//The client is down when bytes received are 0
				log_error(UMCLog,"The client went down while receiving! - Please check the client '%d' is down!\n", *socketClient);
				close(*socketClient);
			}else{
				log_error(UMCLog, "Error - No able to received - Error receiving from socket '%d', with error: %d\n",*socketClient,errno);
				close(*socketClient);
			}
		}

	}else{
		log_error(UMCLog, "I'm not able to connect to the server! - My socket is: '%d'\n", *socketClient);
		close(*socketClient);
	}

	return exitcode;
}

void getConfiguration(char *configFile){

	t_config* configurationFile;
	configurationFile = config_create(configFile);
	configuration.port = config_get_int_value(configurationFile,"PUERTO");
	configuration.ip_swap = config_get_string_value(configurationFile,"IP_SWAP");
	configuration.port_swap = config_get_int_value(configurationFile,"PUERTO_SWAP");
	configuration.frames_max = config_get_int_value(configurationFile,"MARCOS");
	configuration.frames_size = config_get_int_value(configurationFile,"MARCO_SIZE");
	configuration.frames_max_proc = config_get_int_value(configurationFile,"MARCO_X_PROC");
	configuration.algorithm_replace = config_get_string_value(configurationFile,"ALGORITMO");
	configuration.TLB_entries = config_get_int_value(configurationFile,"ENTRADAS_TLB");
	configuration.delay = config_get_int_value(configurationFile,"RETARDO");

}

void startUMCConsole(){
	char command[50];
	char option[50];
	char value[50];

	consoleMessageUMC();

	while (1){
		scanf("%s %s", command, option);

		//cleaning screen
		system("clear");

		int i;
		//lower case options
		for(i = 0; option[i] != '\0'; i++){
			option[i] = tolower(option[i]);
		}

		if (strcmp(command,"retardo") == 0 ){
			pthread_mutex_lock(&delayMutex);
			configuration.delay = atoi(option);
			pthread_mutex_unlock(&delayMutex);
			printf("The delay UMC was successfully changed to: %d\n", configuration.delay);
			log_info(UMCLog, "The delay UMC was successfully changed to: %d\n", configuration.delay);

		}else if (strcmp(command,"dump") == 0 ){
			int neededPID = 0; //DEFAULT value

			//Auxiliary function - Look for page table by PID
			bool is_PIDPageTable(t_pageTablesxProc* listElement){
				return (listElement->PID == neededPID);
			}

			printf("\nCommand entered: '%s %s'\n", command,option);
			printf("== [VALUE]\n");
			printf("all\t\t:: Todos los procesos\n");
			printf("<processPID>\t:: PID del proceso deseado\n\nPlease enter a value with the above format: ");
			scanf("%s", value);

			if(list_size(pageTablesListxProc) != 0){

				if (strcmp(value, "all") == 0){
					neededPID = -1; //ALL PIDs in memory
				}else{
					neededPID = atoi(value); //given PID in memory
				}

				if (strcmp(option, "estructuras") == 0){

					dumpf = fopen(dumpFile, "w+");

					if(neededPID == -1){
						pthread_mutex_lock(&memoryAccessMutex);
						list_iterate(pageTablesListxProc,(void*) dumpPageTablexProc);
						pthread_mutex_unlock(&memoryAccessMutex);
					}else{
						//Look for table page by neededPID
						pthread_mutex_lock(&memoryAccessMutex);
						t_pageTablesxProc *pageTablexProc = (t_pageTablesxProc*) list_find(pageTablesListxProc,(void*) is_PIDPageTable);
						list_iterate(pageTablexProc->ptrPageTable, (void*) showPageTableRows);
						pthread_mutex_unlock(&memoryAccessMutex);
					}

				}else if (strcmp(option, "contenido") == 0){

					if(neededPID == -1){
						pthread_mutex_lock(&memoryAccessMutex);
						list_iterate(pageTablesListxProc,(void*) dumpMemoryxProc);
						pthread_mutex_unlock(&memoryAccessMutex);
					}else{
						//Look for table page by neededPID
						pthread_mutex_lock(&memoryAccessMutex);
						t_pageTablesxProc *pageTablexProc = (t_pageTablesxProc*) list_find(pageTablesListxProc,(void*) is_PIDPageTable);
						list_iterate(pageTablexProc->ptrPageTable, (void*) showMemoryRows);
						pthread_mutex_unlock(&memoryAccessMutex);
					}

				}
				//Closing file
				fclose(dumpf);
				printf("A copy of this dump was saved in: '%s'.\n", dumpFile);
				log_info(UMCLog, "A copy of this dump was saved in: '%s'.\n", dumpFile);
			}else{
				printf("Sorry! There is not information to show you now =(\n");
				log_info(UMCLog, "COMMAND: '%s %s %s' - Sorry! There is not information to show you now =(\n", command, option, value);
			}

		}else if (strcmp(command,"flush") == 0 ){

			if (strcmp(option, "tlb") == 0){

				if(list_all_satisfy(TLBList, (void*)isThereEmptyEntry) == false){
					//Locking TLB access for reading
					pthread_mutex_lock(&memoryAccessMutex);
					list_clean_and_destroy_elements(TLBList, (void*) destroyElementTLB);
					resetTLBAllEntries();
					pthread_mutex_unlock(&memoryAccessMutex);

					printf("The '%s' flush was completed successfully\n", option);
					log_info(UMCLog, "The '%s' flush was completed successfully\n", option);

				}else{
					printf("Sorry! There is not information in TLB for flushing =(\n");
					log_info(UMCLog, "COMMAND: '%s %s' - Sorry! There is not information in TLB for flushing =(\n", command, option);
				}

			}else if (strcmp(option, "memory") == 0){

				if(list_size(pageTablesListxProc) != 0){
					//Locking memory access for reading
					pthread_mutex_lock(&memoryAccessMutex);
					list_iterate(pageTablesListxProc,(void*) iteratePageTablexProc);
					pthread_mutex_unlock(&memoryAccessMutex);

					printf("The '%s' flush was completed successfully\n", option);
					log_info(UMCLog, "The '%s' flush was completed successfully\n", option);

				}else{
					printf("Sorry! There is not information in MEMORY for flushing =(\n");
					log_info(UMCLog, "COMMAND: '%s %s' - Sorry! There is not information in MEMORY for flushing =(\n", command, option);
				}
			}



		}else{

			printf("\nCommand entered NOT recognized: '%s %s'\n", command,option);
			printf("Please take a look to the correct commands!\n");
		}

		consoleMessageUMC();

	}

	//free(command);
	//free(value);

}

int getEnum(char *string){
	int parameter = -1;

	strcmp(string,"PUERTO") == 0 ? parameter = PUERTO : -1 ;
	strcmp(string,"IP_SWAP") == 0 ? parameter = IP_SWAP : -1 ;
	strcmp(string,"PUERTO_SWAP") == 0 ? parameter = PUERTO_SWAP : -1 ;
	strcmp(string,"MARCOS") == 0 ? parameter = MARCOS : -1 ;
	strcmp(string,"MARCO_SIZE") == 0 ? parameter = MARCO_SIZE : -1 ;
	strcmp(string,"MARCO_X_PROC") == 0 ? parameter = MARCO_X_PROC : -1 ;
	strcmp(string,"ENTRADAS_TLB") == 0 ? parameter = ENTRADAS_TLB : -1 ;
	strcmp(string,"RETARDO") == 0 ? parameter = RETARDO : -1 ;

	return parameter;
}

void consoleMessageUMC(){
	printf("\n***********************************\n");
	printf("* UMC Console ready for your use! *");
	printf("\n***********************************\n\n");
	printf("COMMANDS USAGE:\n\n");

	printf("===> COMMAND:\tretardo [OPTIONS]\n");
	printf("== [OPTIONS]\n");
	printf("<numericValue>\t::Cantidad de milisegundos que el sistema debe esperar antes de responder una solicitud\n\n");

	printf("===> COMMAND:\tdump [OPTIONS]\n");
	printf("== [OPTIONS]\n");
	printf("estructuras\t:: Tablas de paginas de todos los procesos o de un proceso en particular\n");
	printf("contenido\t:: Datos almacenados en la memoria de todos los procesos o de un proceso en particular\n\n");

	printf("===> COMMAND:\tflush [OPTIONS]\n");
	printf("== [OPTIONS]\n");
	printf("tlb\t\t:: Limpia completamente el contenido de la TLB\n");
	printf("memory\t\t:: Marca todas las paginas del proceso como modificadas\n\n");
	printf("UMC console >> $ ");
}

void createTLB(){
	TLBList = list_create();
	resetTLBAllEntries();
}

void resetTLBAllEntries(){
	int i;
	for(i=0; i < configuration.TLB_entries; i++){
		t_memoryAdmin *defaultTLBElement = malloc(sizeof(t_memoryAdmin));
		defaultTLBElement->virtualAddress = malloc(sizeof(t_memoryLocation));
		defaultTLBElement->PID = -1; //DEFAULT PID value in TLB
		defaultTLBElement->virtualAddress->pag = -1;
		defaultTLBElement->frameNumber = i;
		list_add(TLBList, (void*) defaultTLBElement);
	}
}

void createAdminStructs(){
	//Creating memory block
	memBlock = calloc(configuration.frames_max, configuration.frames_size);

	//Creating list for checking free frames available
	freeFramesList = list_create();
	int i;
	for(i=0; i < configuration.frames_max; i++){
		int *frameNro = malloc(sizeof(int));
		*frameNro = i;
		list_add(freeFramesList, (void*)frameNro);
	}

	log_info(UMCLog, "'%d' frames created and ready for use.\n", list_size(freeFramesList));

	// Checking if TLB is enable
	if (configuration.TLB_entries != 0){
		//TLB enable
		createTLB();
		TLBActivated = true;
		log_info(UMCLog, "TLB enable. Size '%d'\n", list_size(TLBList));
	}else{
		log_info(UMCLog, "TLB disable!.\n");
	}

	//Creating page table list for Main Memory administration
	pageTablesListxProc = list_create();
}

/******************* Primary functions *******************/

void initializeProgram(int PID, int totalPagesRequired, char *programCode){

	//Creating new table page by PID - NOT needed to load anything in TLB
	t_pageTablesxProc *newPageTable = malloc(sizeof(t_pageTablesxProc));
	newPageTable->PID = PID;
	newPageTable->assignedFrames = 0;
	newPageTable->ptrPageTable = list_create();

	// inform new program to swap and check if it could write it.
	int bufferSize = 0;
	int payloadSize = 0;

	//overwrite page content in swap (swap out)
	t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
	message->virtualAddress = malloc(sizeof(t_memoryLocation));
	message->operation = agregar_proceso;
	message->PID = PID;
	message->cantPages = totalPagesRequired;
	message->virtualAddress->pag = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->offset = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->size = -1;//DEFAULT value when the operation doesn't need it

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) + sizeof(message->cantPages);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char *buffer = malloc(bufferSize);
	//Serialize messageRcv
	serializeUMC_Swap(message, buffer, payloadSize);

	//Send information to swap - message serialized with virtualAddress information
	sendMessage(&socketSwap, buffer, bufferSize);

	//After sending information have to be sent the program code
	//TODO received possible error from SWAP if is OK then send code
	//1) Send program size to swap
	int programCodeLen = strlen(programCode);//+1 not needed it's being sent from Nucleo
	sendMessage(&socketSwap, &programCodeLen, sizeof(programCodeLen));

	//2) Send program to swap
	//string_append(&programCode,"\0");//ALWAYS put \0 for finishing the string
	sendMessage(&socketSwap, programCode, programCodeLen);

	free(buffer);
	free(message);


}

void endProgram(int PID){

	if(TLBActivated){//TLB is enable
		pthread_mutex_lock(&memoryAccessMutex);
		//Reseting to default entries from active PID in TLB
		list_iterate(TLBList, (void*)resetTLBbyActivePID);
		pthread_mutex_unlock(&memoryAccessMutex);
	}

	//Deleting elements from main memory
	pthread_mutex_lock(&memoryAccessMutex);
	list_remove_and_destroy_by_condition(pageTablesListxProc, (void*) find_PIDEntry_PageTable, (void*) PageTable_Element_destroy);
	pthread_mutex_unlock(&memoryAccessMutex);

	// Notify to Swap for freeing space
	// inform new program to swap and check if it could write it.
	int bufferSize = 0;
	int payloadSize = 0;

	//overwrite page content in swap (swap out)
	t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
	message->virtualAddress = (t_memoryLocation *)malloc(sizeof(t_memoryLocation));
	message->operation = finalizar_proceso;
	message->PID = PID;
	message->cantPages = -1; //DEFAULT value when the operation doesn't need it
	message->virtualAddress->pag = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->offset = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->size = -1;//DEFAULT value when the operation doesn't need it

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) + sizeof(message->cantPages);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize;

	char *buffer = malloc(bufferSize);
	//Serialize messageRcv
	serializeUMC_Swap(message, buffer, payloadSize);

	//Send information to swap - message serialized with virtualAddress information
	sendMessage(&socketSwap, buffer, bufferSize);

}

void *requestBytesFromPage(t_memoryLocation *virtualAddress){
	void *memoryContent;
	t_memoryAdmin *memoryElement;
	void *memoryBlockOffset = NULL;

	getElementFrameNro(virtualAddress, READ, memoryElement);

	if (memoryElement != NULL){ //PAGE HIT and NO errors from getFrame
		//delay memory access
		waitForResponse();

		memoryBlockOffset = &memBlock + (memoryElement->frameNumber * configuration.frames_size) + virtualAddress->offset;

		pthread_mutex_lock(&memoryAccessMutex);//Checking mutex before reading
		memcpy(memoryContent, memoryBlockOffset , virtualAddress->size);
		pthread_mutex_unlock(&memoryAccessMutex);
	}else{
		//The main memory hasn't any free frames - inform this to upstream process
		memoryContent = NULL;
	}

	return memoryContent;
}

int writeBytesToPage(t_memoryLocation *virtualAddress, void *buffer){
	int exitCode = EXIT_SUCCESS;
	t_memoryAdmin *memoryElement;
	void *memoryBlockOffset = NULL;

	getElementFrameNro(virtualAddress, WRITE, memoryElement);

	if (memoryElement != NULL){ //PAGE HIT and NO errors from getFrame
		//delay memory access
		waitForResponse();

		memoryBlockOffset = &memBlock + (memoryElement->frameNumber * configuration.frames_size) + virtualAddress->offset;

		pthread_mutex_lock(&memoryAccessMutex);//Locking mutex for writing memory
		memcpy(memoryBlockOffset, buffer , virtualAddress->size);
		pthread_mutex_unlock(&memoryAccessMutex);//unlocking mutex for writing memory
	}else{
		//The main memory hasn't any free frames
		//inform this to upstream process
		exitCode = EXIT_FAILURE;
	}

	return exitCode;
}

/******************* Auxiliary functions *******************/

void iteratePageTablexProc(t_pageTablesxProc *pageTablexProc){
	list_iterate(pageTablexProc->ptrPageTable, (void*) markElementModified);
}

//** Mark memory Element as MODIFIED**//
void markElementModified(t_memoryAdmin *pageTableElement){
	pageTableElement->dirtyBit = PAGE_MODIFIED;
}

//** Mark memory Element as NOTPRESENT**//
void markElementNOTPResent(t_memoryAdmin *pageTableElement){
	pageTableElement->presentBit = PAGE_NOTPRESENT;
}

//** Dumper Page Table Element **//
void dumpPageTablexProc(t_pageTablesxProc *pageTablexProc){
	printf("Informacion PID: '%d'.\n", pageTablexProc->PID);
	fprintf(dumpf,"Informacion PID: '%d'.\n", pageTablexProc->PID);

	list_iterate(pageTablexProc->ptrPageTable, (void*) showPageTableRows);
}

//** Show Page Table Element information **//
void showPageTableRows(t_memoryAdmin *pageTableElement){
	char *status = string_new();
	char *presence = string_new();

	if(pageTableElement->dirtyBit == PAGE_MODIFIED){
		string_append(&status,"MODIFIED");
	}else{
		string_append(&status,"NOT_MODIFIED");
	}

	if(pageTableElement->presentBit == PAGE_PRESENT){
		string_append(&presence,"PAGE_PRESENT");
	}else{
		string_append(&presence,"PAGE_NOT_PRESENT");
	}

	printf("\t En Frame '%d'\t--> Pagina '%d'\t--> status: '%s' and '%s'.\n", pageTableElement->frameNumber,pageTableElement->virtualAddress->pag, presence, status);
	fprintf(dumpf,"\t En Frame '%d'\t--> Pagina '%d'\t--> status: '%s' and '%s'.\n", pageTableElement->frameNumber,pageTableElement->virtualAddress->pag, presence, status);

	free(status);
	free(presence);

}

//** Dumper Memory content **//
void dumpMemoryxProc(t_pageTablesxProc *pageTablexProc){
	printf("Contenido de memoria de PID: '%d'.\n", pageTablexProc->PID);
	fprintf(dumpf,"Contenido de memoria de PID: '%d'.\n", pageTablexProc->PID);

	list_iterate(pageTablexProc->ptrPageTable, (void*) showMemoryRows);
}

//** Show Memory content **//
void showMemoryRows(t_memoryAdmin *pageTableElement){
	char *content = string_new();

	void *memoryBlockOffset = NULL;
	memoryBlockOffset = &memBlock + (pageTableElement->frameNumber * configuration.frames_size) + pageTableElement->virtualAddress->offset;

	content = realloc(content, pageTableElement->virtualAddress->size);

	memcpy(content, memoryBlockOffset, pageTableElement->virtualAddress->size);

	printf("\t Contenido en Pagina:'%d'\n\t\tContenido: '%s'.\n", pageTableElement->virtualAddress->pag, content);
	fprintf(dumpf,"\t Contenido en Pagina:'%d'\n\t\tContenido: '%s'.\n", pageTableElement->virtualAddress->pag, content);

	free(content);
}

void deleteContentFromMemory(t_memoryAdmin *memoryElement){
	//check page status before deleting
	checkPageModification(memoryElement);

	//Deleting element from Main Memory
	int memOffset = memoryElement->frameNumber * configuration.frames_size + memoryElement->virtualAddress->offset;
	memset(memBlock + memOffset,'\0', memoryElement->virtualAddress->size);

	log_info(UMCLog, "From PID '%d' - Content in page '#%d' DELETED from memory\n",memoryElement->PID, memoryElement->virtualAddress->pag);

	//adding frame again to free frames list
	int *frameNro = malloc(sizeof(int));
	*frameNro = memoryElement->frameNumber;
	list_add(freeFramesList, (void*)frameNro);

	log_info(UMCLog, "NEW free frame '%d' available\n", memoryElement->frameNumber);

}

void checkPageModification(t_memoryAdmin *memoryElement){

	if (memoryElement->dirtyBit == PAGE_MODIFIED){
		int bufferSize = 0;
		int payloadSize = 0;
		void *memoryBlockOffset = NULL;
		char *content = string_new();

		//overwrite page content in swap (swap out)
		t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
		message->virtualAddress = (t_memoryLocation *)malloc(sizeof(t_memoryLocation));
		message->operation = escritura_pagina;
		message->PID = memoryElement->PID;
		message->cantPages = -1; //DEFAULT value when the operation doesnt need it
		memcpy(message->virtualAddress, memoryElement->virtualAddress,sizeof(memoryElement->virtualAddress));

		payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) + sizeof(message->cantPages);
		bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

		char *buffer = malloc(bufferSize);
		//Serialize messageRcv
		serializeUMC_Swap(message, buffer, payloadSize);

		//Send message serialized with virtualAddress information
		sendMessage(&socketSwap, buffer, bufferSize);

		//Send memory content to overwrite with the virtualAddress->size - On the other side is going to be waiting it with that size sent previously
		memoryBlockOffset = &memBlock + (memoryElement->frameNumber * configuration.frames_size) + memoryElement->virtualAddress->offset;
		content = realloc(content, memoryElement->virtualAddress->size);
		memcpy(content, memoryBlockOffset, memoryElement->virtualAddress->size);
		sendMessage(&socketSwap, content, memoryElement->virtualAddress->size); //OJO asegurarse que el CPU siempre envie pagesize

		log_info(UMCLog, "From PID '%d' - Content in page '#%d' swapped OUT\n",memoryElement->PID, memoryElement->virtualAddress->pag);

		free(message->virtualAddress);
		free(message);
		free(buffer);
		free(content);
	}
}

void *requestPageToSwap(t_memoryLocation *virtualAddress, int PID){
	void *memoryContent = NULL;
	int bufferSize = 0;
	int payloadSize = 0;
	char *buffer = malloc(bufferSize);
	char *messageRcv = malloc(sizeof(virtualAddress->size));

	// request page content to swap
	t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
	message->virtualAddress = (t_memoryLocation *)malloc(sizeof(t_memoryLocation));
	message->operation = lectura_pagina;
	message->PID = PID;
	message->cantPages = -1; //DEFAULT value when the operation doesnt need it
	memcpy(message->virtualAddress, virtualAddress, sizeof(virtualAddress));

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) + sizeof(message->cantPages);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	//Serialize messageRcv
	serializeUMC_Swap(message, buffer, payloadSize);

	//Send message serialized with virtualAddress information
	sendMessage(&socketSwap, buffer, bufferSize);

	//Receive memory content from SWAP with the virtualAddress->size - On the other side is going to be sending it with that size requested previously
	int receivedBytes = receiveMessage(&socketSwap, messageRcv, virtualAddress->size);
	memoryContent = malloc(virtualAddress->size);
	memcpy(memoryContent, messageRcv, virtualAddress->size);

	log_info(UMCLog, "From PID '%d' - Content in page '#%d' swapped IN\n",PID, virtualAddress->pag);

	free(message->virtualAddress);
	free(message);
	free(buffer);
	free(messageRcv);

	return memoryContent;
}

//** TLB and PageTable Element Destroyer **//
void destroyElementTLB(t_memoryAdmin *elementTLB){
	//Erasing memory content
	deleteContentFromMemory(elementTLB);

	//Deleting all elements from structure
	free(elementTLB->virtualAddress);
	free(elementTLB);
}

//** Page Table Element Destroyer **//
void PageTable_Element_destroy(t_pageTablesxProc *self){
	list_destroy_and_destroy_elements(self->ptrPageTable, (void*) destroyElementTLB);
	free(self);
}

//** Find function by process**//
bool is_PIDPageTablePresent(t_pageTablesxProc* listElement){
	return (listElement->PID == activePID);
}

//** Find function by PID**//
bool find_PIDEntry_PageTable(t_pageTablesxProc* listElement){
	return (listElement->PID == activePID);
}

//** Find function by Page**//
bool isThereEmptyEntry(t_memoryAdmin* listElement){
	return (listElement->virtualAddress->pag == -1);
}

//** Find Page with presence bit disable**//
bool isPageNOTPresent(t_memoryAdmin* listElement){
	return (listElement->presentBit == PAGE_NOTPRESENT);
}

//** Find Page with presence bit disable and not modified**//
bool isPageNOTPresentNOTModified(t_memoryAdmin* listElement){
	return ((listElement->presentBit == PAGE_NOTPRESENT) & (listElement->presentBit == PAGE_NOTMODIFIED));
}

//** Find Page with presence bit disable and  modified**//
bool isPageNOTPresentModified(t_memoryAdmin* listElement){
	bool exitCode;

	exitCode = ((listElement->presentBit == PAGE_NOTPRESENT) & (listElement->presentBit == PAGE_MODIFIED));

	if (exitCode == false){
		listElement->presentBit = PAGE_NOTPRESENT;
	}

	return exitCode;
}


t_memoryAdmin *getLRUCandidate(){
	t_memoryAdmin *elementCandidate = NULL;

	pthread_mutex_lock(&memoryAccessMutex);
	//Get memory element from last index position
	elementCandidate = (t_memoryAdmin*) list_remove(TLBList, list_size(TLBList) - 1);
	pthread_mutex_unlock(&memoryAccessMutex);

	return elementCandidate;
}

void updateTLBPositionsbyLRU(t_memoryAdmin *elementCandidate){
	pthread_mutex_lock(&memoryAccessMutex);
	//add element to first position for keeping Least Recently used access updated
	list_add_in_index(TLBList, 0, (void*)elementCandidate);
	pthread_mutex_unlock(&memoryAccessMutex);
}

void resetTLBbyActivePID(t_memoryAdmin *listElement){

	if(listElement->PID == activePID){
		//adding frame again to free frames list
		int *frameNro = malloc(sizeof(int));
		*frameNro = listElement->frameNumber;
		list_add(freeFramesList, (void*)frameNro);

		//Additionally to restore PID default value, restoring all values to default just in case..
		listElement->PID = -1;
		listElement->dirtyBit = PAGE_NOTMODIFIED;
		listElement->presentBit = PAGE_NOTPRESENT;
		listElement->frameNumber = -1;
		free(listElement->virtualAddress);
		listElement->virtualAddress = malloc(sizeof(listElement->virtualAddress));
	}
}

void getElementFrameNro(t_memoryLocation *virtualAddress, enum_memoryOperations operation, t_memoryAdmin *memoryElement){
	enum_memoryStructure memoryStructure;

	if(TLBActivated){//TLB is enable
		memoryStructure = TLB;
		memoryElement = searchFramebyPage(memoryStructure, operation, virtualAddress);
	}

	if (memoryElement == NULL){//TLB is disable or PAGE FAULT in TLB

		memoryStructure = MAIN_MEMORY;
		memoryElement = searchFramebyPage(memoryStructure, operation, virtualAddress);

		t_pageTablesxProc *pageTablexProc = NULL;

		//Look for table page by active Process - (at least the list is going to have an empty entry due to its creation)
		pthread_mutex_lock(&memoryAccessMutex);
		pageTablexProc = (t_pageTablesxProc*) list_find(pageTablesListxProc,(void*) is_PIDPageTablePresent);
		pthread_mutex_unlock(&memoryAccessMutex);

		updateMemoryStructure(pageTablexProc, virtualAddress, memoryElement);

	}
}

t_memoryAdmin *searchFramebyPage(enum_memoryStructure deviceLocation, enum_memoryOperations operation, t_memoryLocation *virtualAddress){
	t_pageTablesxProc *pageTablexProc = NULL;
	t_list *pageTableList = NULL;//lista con registros del tipo t_memoryAdmin
	t_memoryAdmin* pageNeeded = NULL;

	//** Find function by page**//
	bool is_PagePresent(t_memoryAdmin* listElement){
		return (listElement->virtualAddress->pag == virtualAddress->pag);
	}

	switch(deviceLocation){
		case(TLB):{
			//Locking TLB access for reading
			pthread_mutex_lock(&memoryAccessMutex);
			pageNeeded = (t_memoryAdmin*) list_find(TLBList,(void*) is_PagePresent);
			pthread_mutex_unlock(&memoryAccessMutex);
			break;
		}
		case(MAIN_MEMORY):{
			//Look for table page by active Process
			pthread_mutex_lock(&memoryAccessMutex);
			pageTablexProc = (t_pageTablesxProc*) list_find(pageTablesListxProc,(void*) is_PIDPageTablePresent);
			pthread_mutex_unlock(&memoryAccessMutex);

			//By default in program initialization ptrPageTable is going to be NULL
			pageTableList = pageTablexProc->ptrPageTable;

			//Locking memory access for reading
			pthread_mutex_lock(&memoryAccessMutex);
			pageNeeded = (t_memoryAdmin*) list_find(pageTableList,(void*) is_PagePresent);
			pthread_mutex_unlock(&memoryAccessMutex);
			break;
		}
		default:{
			log_error(UMCLog, "Error Device Location not recognized for searching: '%d'\n",deviceLocation);
		}
	}

	if (pageNeeded != NULL){
		//Page found
		switch(operation){
			case(READ):{
				//After getting the frame needed for reading, mark memory element as present (overwrite it no matter if it was marked as present before)
				pageNeeded->presentBit = PAGE_PRESENT;
				break;
			}
			case (WRITE):{
				//After getting the frame needed for writing, mark memory element as modified and as present as well
				pageNeeded->presentBit = PAGE_PRESENT;
				pageNeeded->dirtyBit = PAGE_MODIFIED;
				break;
			}
		}
	}

	return pageNeeded;
}

void updateMemoryStructure(t_pageTablesxProc *pageTablexProc, t_memoryLocation *virtualAddress, t_memoryAdmin *memoryElement){
	/*
	 * 1)busco frame libre(ver algoritmo de ubicacion - puede ser cualquiera es irrelevante para paginacion pura)
	 * 2)agrego info en TLB (si corresponde)
	 * 3)agrego info en estructura de Main Memory
	 * 4)pageTablexProc->assignedFrames++;
	 */

	if (memoryElement != NULL){//PAGE HIT in Main Memory

		log_info(UMCLog, "PID '%d': PAGE HIT in Main Memory - Page #%d\n", pageTablexProc->PID, virtualAddress->pag);

		if(TLBActivated){//TLB is enable
			t_memoryAdmin *TLBElem = NULL;

			pthread_mutex_lock(&memoryAccessMutex);
			TLBElem = (t_memoryAdmin*) list_find(TLBList, (void*) isThereEmptyEntry);
			pthread_mutex_unlock(&memoryAccessMutex);

			if (TLBElem != NULL){
				//Load new program structure in empty TLB entry
				TLBElem->PID = memoryElement->PID;
				TLBElem->dirtyBit = memoryElement->dirtyBit;
				TLBElem->presentBit = memoryElement->presentBit;
				TLBElem->virtualAddress = virtualAddress;
				TLBElem->frameNumber = memoryElement->frameNumber;

			}else{//No empty entry is present in TLB

				//Execute LRU algorithm
				executeLRUAlgorithm(memoryElement, virtualAddress);
			}
		}
	}else{//PAGE Fault in Main Memory

		log_info(UMCLog, "PID '%d': PAGE Fault in Main Memory - Page #%d\n", pageTablexProc->PID, virtualAddress->pag);

		if (list_size(freeFramesList) > 0){

			//The main memory still has free frames available
			log_info(UMCLog, "The main memory still has free frames to assign to process '%d'\n", activePID);

			//Request memory content to Swap
			void *memoryContent = requestPageToSwap(virtualAddress, pageTablexProc->PID);

			//request memory for new element
			memoryElement = malloc(sizeof(t_memoryAdmin));

			//By DEFAULT always take the first free frame
			pthread_mutex_lock(&memoryAccessMutex);
			memoryElement->frameNumber = (int) list_remove(freeFramesList,0);
			pthread_mutex_unlock(&memoryAccessMutex);
			memoryElement->PID = pageTablexProc->PID;
			memoryElement->presentBit = PAGE_PRESENT;
			memoryElement->dirtyBit = PAGE_NOTMODIFIED;
			memoryElement->virtualAddress = virtualAddress;

			//Adding new Page table entry by process
			pthread_mutex_lock(&memoryAccessMutex);
			list_add(pageTablexProc->ptrPageTable,(void *)memoryElement);
			pthread_mutex_unlock(&memoryAccessMutex);

			//check if process has not reached the max of frames by process
			if (pageTablexProc->assignedFrames < configuration.frames_max_proc){

				//The active process has free frames available
				log_info(UMCLog, "The process '%d' still has free frames to assigned\n", activePID);

				void *memoryBlockOffset = NULL;
				memoryBlockOffset = &memBlock + (memoryElement->frameNumber * configuration.frames_size) + virtualAddress->offset;

				pthread_mutex_lock(&memoryAccessMutex);//Locking mutex for writing memory
				memcpy(memoryBlockOffset, memoryContent , virtualAddress->size);
				pthread_mutex_unlock(&memoryAccessMutex);//unlocking mutex for writing memory

			}else{
				//The active process hasn't free frames available, replacement algorithm has to be executed
				log_info(UMCLog, "The process '%d' hasn't more free frames available \n", activePID);

				//  ejecuto algoritmo reemplazo segun deviceLocation ( TLB=> LRU, MAIN_MEMORY=> Clock/mejorado)

				if(TLBActivated){//TLB is enable
					//Execute LRU algorithm
					executeLRUAlgorithm(memoryElement, virtualAddress);
				}else{
					//execute main memory algorithm
					executeMainMemoryAlgorithm(pageTablexProc, memoryElement, memoryContent);
				}

			}
		}else{
			//The main memory still hasn't any free frames
			log_warning(UMCLog, "The main memory is FULL!\n");
			//inform this to upstream process
			memoryElement = NULL;
			//exit function immediately
			return;
		}

	}

	//increasing frames assigned to active process
	pageTablexProc->assignedFrames++;

}

void executeLRUAlgorithm(t_memoryAdmin *newElement, t_memoryLocation *virtualAddress){
	t_memoryAdmin *LRUCandidate = NULL;

	//Replacement algorithm LRU
	LRUCandidate = getLRUCandidate();
	log_info(UMCLog, "PID: '%d' - New page needed '%d' -> LRU candidate page #%d\n", newElement->PID, newElement->virtualAddress->pag, LRUCandidate->virtualAddress->pag);
	//check page status before replacing
	checkPageModification(LRUCandidate);

	//Load new program structure into candidate TLB entry
	LRUCandidate->PID = newElement->PID;
	LRUCandidate->dirtyBit = newElement->dirtyBit;
	LRUCandidate->presentBit = newElement->presentBit;
	LRUCandidate->virtualAddress = virtualAddress;

	//updated TLB by LRU algorithm
	updateTLBPositionsbyLRU(LRUCandidate);
}

void executeMainMemoryAlgorithm(t_pageTablesxProc *pageTablexProc, t_memoryAdmin *newElement, void *memoryContent){
	t_memoryAdmin *clockCandidate = NULL;

	bool find_ClockCandidateToRemove(t_memoryAdmin* listElement){
		return (listElement->virtualAddress->pag == clockCandidate->virtualAddress->pag);
	}

	bool find_freeFrameToRemove (int freeFrame){
		return (freeFrame == clockCandidate->frameNumber);
	}

	pthread_mutex_lock(&memoryAccessMutex);

	if (string_equals_ignore_case(configuration.algorithm_replace,"CLOCK")){
		//*** Algorithm CLOCK ***//

		//First seek
		clockCandidate = (t_memoryAdmin*) list_find(pageTablexProc->ptrPageTable, (void*) isPageNOTPresent);

		if(clockCandidate == NULL){
			//Second seek for candidate after first pass - THIS ENSURES A CANDIDATE
			clockCandidate = (t_memoryAdmin*) list_find(pageTablexProc->ptrPageTable, (void*) isPageNOTPresent);
		}

		log_info(UMCLog, "PID: '%d' - New page needed '%d' -> CLOCK candidate page #%d\n", pageTablexProc->PID, newElement->virtualAddress->pag, clockCandidate->virtualAddress->pag);

		//Candidate found
		newElement->frameNumber = clockCandidate->frameNumber;
		newElement->presentBit = PAGE_PRESENT;

	}else{
		//*** Algorithm ENHANCED CLOCK***//

		while (clockCandidate == NULL){
			//First seek
			clockCandidate = (t_memoryAdmin*) list_find(pageTablexProc->ptrPageTable, (void*) isPageNOTPresentNOTModified);

			if(clockCandidate == NULL){
				//Second seek for candidate after first pass
				clockCandidate = (t_memoryAdmin*) list_find(pageTablexProc->ptrPageTable, (void*) isPageNOTPresentModified);
			}
		}

		log_info(UMCLog, "PID: '%d' - New page needed '%d' -> ENCAHNCED CLOCK candidate page #%d\n", pageTablexProc->PID, newElement->virtualAddress->pag, clockCandidate->virtualAddress->pag);

		//Candidate found
		newElement->frameNumber = clockCandidate->frameNumber;
		newElement->presentBit = PAGE_PRESENT;

	}

	//Removing candidate from page table list
	list_remove_and_destroy_by_condition(pageTablexProc->ptrPageTable, (void*) find_ClockCandidateToRemove, (void*) destroyElementTLB);

	//Removing the frame added to free frames list when the candidate was destroyed
	list_remove_and_destroy_by_condition(freeFramesList, (void*) find_freeFrameToRemove, (void*) free);

	//Add new element to the end of the list
	list_add_in_index(pageTablexProc->ptrPageTable, list_size(pageTablexProc->ptrPageTable) - 1, newElement);

	void *memoryBlockOffset = NULL;
	memoryBlockOffset = &memBlock + (newElement->frameNumber * configuration.frames_size) + newElement->virtualAddress->offset;

	//writing memory
	memcpy(memoryBlockOffset, memoryContent , newElement->virtualAddress->size);

	pthread_mutex_unlock(&memoryAccessMutex);
}

void waitForResponse(){
	pthread_mutex_lock(&delayMutex);
	sleep(configuration.delay);
	pthread_mutex_unlock(&delayMutex);
}

void changeActiveProcess(int PID){

	//Ante un cambio de proceso, se realizará una limpieza (flush) en las entradas que correspondan.
	if(TLBActivated){//TLB is enable
		pthread_mutex_lock(&memoryAccessMutex);
		//Reseting to default entries from previous active PID
		list_iterate(TLBList, (void*)resetTLBbyActivePID);
		pthread_mutex_unlock(&memoryAccessMutex);
	}

	//after flushing entries from old process change active process to the one needed
	log_info(UMCLog, "Changing to active PID: '#%d'\n", PID);
	activePID = PID;
}
