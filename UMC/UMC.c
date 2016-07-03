#include "UMC.h"

int main(int argc, char *argv[]){
	int exitCode = EXIT_FAILURE ; //DEFAULT failure
	char *configurationFile = NULL;
	pthread_t serverThread;
	pthread_t consolaThread;

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS TODO => Agregar logs con librerias

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
	}

	//ERROR if not configuration parameter was passed
	assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS TODO => Agregar logs con librerias

	//ERROR if not configuration parameter was passed
	assert(("ERROR - NOT dump file was passed as argument", dumpFile != NULL));//Verifies if was passed the Dump file as parameter, if DONT FAILS TODO => Agregar logs con librerias

	//get configuration from file
	getConfiguration(configurationFile);

	//created administration structures for UMC process
	createAdminStructs();

	//Connecting to SWAP before starting everything else
	exitCode = connectTo(SWAP,&socketSwap);

	if(exitCode == EXIT_SUCCESS){
		printf("UMC connected to SWAP successfully\n");

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
	t_serverData serverData;

	exitCode = openServerConnection(configuration.port, &serverData.socketServer);
	printf("socketServer: %d\n",serverData.socketServer);

	//If exitCode == 0 the server connection is opened and listening
	if (exitCode == 0) {
		puts ("The server is opened.");

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
	int pid;

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
		printf("There was detected an attempt of wrong connection\n");//TODO => Agregar logs con librerias
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
		perror("The client went down while handshaking!"); //TODO => Agregar logs con librerias
		printf("Please check the client: %d is down!\n", serverData->socketClient);
		close(serverData->socketClient);
	}else{
		switch ((int) message->process){
			case CPU:{
				printf("%s\n",message->message);

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
				printf("%s\n",message->message);
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
				perror("Process not allowed to connect");//TODO => Agregar logs con librerias
				printf("Invalid process '%s' tried to connect to UMC\n",getProcessString(message->process));
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

		printf("bytes received: %d\n",receivedBytes);

		switch (fromProcess){
			case CPU:{
				printf("Processing CPU message received\n");
				pthread_mutex_lock(&activeProcessMutex);
				procesCPUMessages(messageRcv, messageSize, serverData);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			case NUCLEO:{
				printf("Processing NUCLEO message received\n");
				pthread_mutex_lock(&activeProcessMutex);
				procesNucleoMessages(messageRcv, messageSize, serverData);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			default:{
				perror("Process not allowed to connect");//TODO => Agregar logs con librerias
				printf("Invalid process '#%s' tried to connect to UMC\n",getProcessString(fromProcess));
				close(serverData->socketClient);
				break;
			}
		}

	}else if (receivedBytes == 0 ){
		//The client is down when bytes received are 0
		perror("One of the clients is down!"); //TODO => Agregar logs con librerias
		printf("Please check the client: %d is down!\n", serverData->socketClient);
		close(serverData->socketClient);
	}else{
		perror("Error - No able to received");//TODO => Agregar logs con librerias
		printf("Error receiving from socket '%d', with error: %d\n",serverData->socketClient,errno);
		close(serverData->socketClient);
	}

	free(messageRcv);

}

void procesCPUMessages(char *messageRcv, int messageSize, t_serverData* serverData){
	int exitcode = EXIT_SUCCESS;

	//Receive message using the size read before
	messageRcv = realloc(messageRcv, messageSize);
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

			break;
		}
		case escritura_pagina:{

			//Receive content size
			messageSize = 0;//reseting size to get the content size
			receivedBytes = 0;//reseting receivedBytes to get the content size

			receivedBytes = receiveMessage(&serverData->socketClient, (void*) messageSize, sizeof(messageSize));

			//Receive content using the size read before
			content = realloc(content, messageSize);
			receivedBytes = receiveMessage(&serverData->socketClient, content, messageSize);

			exitcode = writeBytesToPage(message->virtualAddress, content);

			if(content != NULL){
				//After getting the memory content WITHOUT issues, inform status of the operation
				sendMessage(&serverData->socketClient, &exitcode, sizeof(exitcode));
			}else{
				//The main memory hasn't any free frames - inform status of the operation
				sendMessage(&serverData->socketClient, &exitcode, sizeof(exitcode));
			}

			break;
		}
		default:{
			perror("Process not allowed to connect");//TODO => Agregar logs con librerias
			printf("Invalid operation for CPU messages '%d' requested to UMC \n",(int) message->operation);
			close(serverData->socketClient);
			break;
		}
	}
}

void procesNucleoMessages(char *messageRcv, int messageSize, t_serverData* serverData){
	int exitcode = EXIT_SUCCESS;

	//Receive message using the size read before
	messageRcv = realloc(messageRcv, messageSize);
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

	t_MessageNucleo_UMC *message = malloc(sizeof(t_MessageNucleo_UMC));

	//Deserialize messageRcv
	deserializeUMC_Nucleo(message, messageRcv);

	changeActiveProcess(message->PID);

	void *content = NULL;

	switch (message->operation){
		case agregar_proceso:{

			//Receive content size
			messageSize = 0;//reseting size to get the content size
			receivedBytes = 0;//reseting receivedBytes to get the content size

			receivedBytes = receiveMessage(&serverData->socketClient, (void*) messageSize, sizeof(messageSize));

			//Receive content using the size read before
			content = realloc(content, messageSize);
			receivedBytes = receiveMessage(&serverData->socketClient, content, messageSize);

			initializeProgram(message->PID, message->cantPages, content);

			break;
		}
		case finalizar_proceso:{

			endProgram(message->PID);

			break;
		}
		default:{
			perror("Process not allowed to connect");//TODO => Agregar logs con librerias
			printf("Invalid operation for CPU messages '%d' requested to UMC \n",(int) message->operation);
			close(serverData->socketClient);
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
			perror("Process not identified!");//TODO => Agregar logs con librerias
			printf("Process '%s' NOT VALID to be connected by UMC.\n",getProcessString(processToConnect));
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
							case SWAP:{
								printf("%s\n",message->message);
								printf("Conectado a NUCLEO\n");
								break;
							}
							default:{
								perror("Handshake not accepted");//TODO => Agregar logs con librerias
								printf("Handshake not accepted when tried to connect your '%s' with '%s'\n",getProcessString(processToConnect),getProcessString(message->process));
								close(*socketClient);
								exitcode = EXIT_FAILURE;
								break;
							}
						}

						break;
					}
					default:{
						perror("Process couldn't connect to SERVER");//TODO => Agregar logs con librerias
						printf("Not able to connect to server %s. Please check if it's down.\n",ip);
						close(*socketClient);
						break;
						break;
					}
				}

			}else if (receivedBytes == 0 ){
				//The client is down when bytes received are 0
				perror("One of the clients is down!"); //TODO => Agregar logs con librerias
				printf("Please check the client: %d is down!\n", *socketClient);
				close(*socketClient);
			}else{
				perror("Error - No able to received");//TODO => Agregar logs con librerias
				printf("Error receiving from socket '%d', with error: %d\n",*socketClient,errno);
				close(*socketClient);
			}
		}

	}else{
		perror("no me pude conectar al server!"); //
		printf("mi socket es: %d\n", *socketClient);
		close(*socketClient);
	}

	return exitcode;
}

void getConfiguration(char *configFile){

	FILE *file = fopen(configFile, "r");

	assert(("ERROR - Could not open the configuration file", file != 0));// ERROR - Could not open file TODO => Agregar logs con librerias

	char parameter[12]; //[12] is the max paramenter's name size
	char parameterValue[20];

	while ((fscanf(file, "%s", parameter)) != EOF){

		switch(getEnum(parameter)){
			case(PUERTO):{ //1
				fscanf(file, "%s",parameterValue);
				configuration.port = (strcmp(parameter, EOL_DELIMITER) != 0) ? atoi(parameterValue) : 0 /*DEFAULT VALUE*/;
				break;
			}
			case(IP_SWAP):{ //2
				fscanf(file, "%s",parameterValue);
				(strcmp(parameter, EOL_DELIMITER) != 0) ? memcpy(&configuration.ip_swap, parameterValue, sizeof(configuration.ip_swap)) : "" ;
				break;
			}
			case(PUERTO_SWAP):{ //3
				fscanf(file, "%s",parameterValue);
				configuration.port_swap = (strcmp(parameter, EOL_DELIMITER) != 0) ? atoi(parameterValue) : 0 /*DEFAULT VALUE*/;
				break;
			}
			case(MARCOS):{ //4
				fscanf(file, "%s",parameterValue);
				configuration.frames_max = (strcmp(parameter, EOL_DELIMITER) != 0) ? atoi(parameterValue) : 0 /*DEFAULT VALUE*/;
				break;
			}
			case(MARCO_SIZE):{ //5
				fscanf(file, "%s",parameterValue);
				configuration.frames_size = (strcmp(parameter, EOL_DELIMITER) != 0) ? atoi(parameterValue): 0 /*DEFAULT VALUE*/ ;
				break;
			}
			case(MARCO_X_PROC):{ //6
				fscanf(file, "%s",parameterValue);
				configuration.frames_max_proc = (strcmp(parameter, EOL_DELIMITER) != 0) ? atoi(parameterValue) : 0 /*DEFAULT VALUE*/;
				break;
			}
			case(ALGORITMO):{ //7
				fscanf(file, "%s",parameterValue);
				(strcmp(parameter, EOL_DELIMITER) != 0) ? memcpy(&configuration.algorithm_replace, parameterValue, sizeof(configuration.algorithm_replace)) : "" ;
				break;
			}
			case(ENTRADAS_TLB):{ //8
				fscanf(file, "%s",parameterValue);
				configuration.TLB_entries = (strcmp(parameter, EOL_DELIMITER) != 0) ? atoi(parameterValue) : -1 /*DEFAULT VALUE -1 != 0 (TLB disable)*/;
				break;
			}
			case(RETARDO):{ //9
				fscanf(file, "%s",parameterValue);
				configuration.delay = (strcmp(parameter, EOL_DELIMITER) != 0) ? atoi(parameterValue) : 0 /*DEFAULT VALUE*/;
				break;
			}
			default:{
				if (strcmp(parameter, EOL_DELIMITER) != 0){
					perror("Error - Parameter read not recognized");//TODO => Agregar logs con librerias
					printf("Error Parameter read not recognized '%s'\n",parameter);
				}
				break;
			}
		}// END switch(parameter)
	}

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

				}else if (strcmp(option, "contenido")){

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
			}else{
				printf("Sorry! There is not information to show you now =(\n");
			}

		}else if (strcmp(command,"flush") == 0 ){

			if (strcmp(option, "tlb") == 0){
				//Locking TLB access for reading
				pthread_mutex_lock(&memoryAccessMutex);
				list_clean_and_destroy_elements(TLBList, (void*) destroyElementTLB);
				resetTLBAllEntries();
				pthread_mutex_unlock(&memoryAccessMutex);

			}else if (strcmp(option, "memory")){
				//Locking memory access for reading
				pthread_mutex_lock(&memoryAccessMutex);
				list_iterate(pageTablesListxProc,(void*) iteratePageTablexProc);
				pthread_mutex_unlock(&memoryAccessMutex);
			}

			printf("The '%s' flush was completed successfully\n", option);

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

	printf("'%d' frames created and ready for use.\n", list_size(freeFramesList));

	// Checking if TLB is enable
	if (configuration.TLB_entries != 0){
		//TLB enable
		createTLB();
		TLBActivated = true;
		printf("TLB enable. Size '%d'\n", list_size(TLBList));
	}else{
		printf("TLB disable!.\n");
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
	void *memoryBlockOffset = NULL;
	char *content = string_new();

	//overwrite page content in swap (swap out)
	t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
	message->virtualAddress = (t_memoryLocation *)malloc(sizeof(t_memoryLocation));
	message->operation = agregar_proceso;
	message->PID = PID;
	message->cantPages = totalPagesRequired;
	message->virtualAddress->pag = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->offset = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->size = -1;//DEFAULT value when the operation doesn't need it

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) ;
	bufferSize = sizeof(bufferSize) + payloadSize ;

	char *buffer = malloc(bufferSize);
	//Serialize messageRcv
	serializeUMC_Swap(message, buffer, payloadSize);

	//Send information to swap - message serialized with virtualAddress information
	sendMessage(&socketSwap, buffer, bufferSize);

	//After sending information have to be sent the program code

	//1) Send program size to swap
	string_append(&programCode,"\0");//ALWAYS put \0 for finishing the string
	int programCodeLen = strlen(programCode) + 1;//+1 because of '\0'
	sendMessage(&socketSwap, (void*) programCodeLen, sizeof(programCodeLen));

	//2) Send program to swap
	sendMessage(&socketSwap, content, programCodeLen);

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
	void *memoryBlockOffset = NULL;
	char *content = string_new();

	//overwrite page content in swap (swap out)
	t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
	message->virtualAddress = (t_memoryLocation *)malloc(sizeof(t_memoryLocation));
	message->operation = finalizar_proceso;
	message->PID = PID;
	message->cantPages = -1; //DEFAULT value when the operation doesn't need it
	message->virtualAddress->pag = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->offset = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->size = -1;//DEFAULT value when the operation doesn't need it

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) ;
	bufferSize = sizeof(bufferSize) + payloadSize ;

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

	//adding frame again to free frames list
	int *frameNro = malloc(sizeof(int));
	*frameNro = memoryElement->frameNumber;
	list_add(freeFramesList, (void*)frameNro);

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

		payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) ;
		bufferSize = sizeof(bufferSize) + payloadSize ;

		char *buffer = malloc(bufferSize);
		//Serialize messageRcv
		serializeUMC_Swap(message, buffer, payloadSize);

		//Send message serialized with virtualAddress information
		send(socketSwap, (void*) buffer, bufferSize,0);

		//Send memory content to overwrite with the virtualAddress->size - On the other side is going to be waiting it with that size sent previously
		memoryBlockOffset = &memBlock + (memoryElement->frameNumber * configuration.frames_size) + memoryElement->virtualAddress->offset;
		content = realloc(content, memoryElement->virtualAddress->size);
		memcpy(content, memoryBlockOffset, memoryElement->virtualAddress->size);
		send(socketSwap, (void*) content, memoryElement->virtualAddress->size,0);

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
	memcpy(message->virtualAddress, virtualAddress,sizeof(virtualAddress));

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) ;
	bufferSize = sizeof(bufferSize) + payloadSize ;

	//Serialize messageRcv
	serializeUMC_Swap(message, buffer, payloadSize);

	//Send message serialized with virtualAddress information
	send(socketSwap, (void*) buffer, bufferSize,0);

	//Receive memory content from SWAP with the virtualAddress->size - On the other side is going to be sending it with that size requested previously
	int receivedBytes = receiveMessage(&socketSwap, messageRcv, virtualAddress->size);
	memoryContent = malloc(virtualAddress->size);
	memcpy(memoryContent, messageRcv, virtualAddress->size);

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

			pageTableList = pageTablexProc->ptrPageTable;

			//Locking memory access for reading
			pthread_mutex_lock(&memoryAccessMutex);
			pageNeeded = (t_memoryAdmin*) list_find(pageTableList,(void*) is_PagePresent);
			pthread_mutex_unlock(&memoryAccessMutex);
			break;
		}
		default:{
			perror("Error - Device Location not recognized for searching");//TODO => Agregar logs con librerias
			printf("Error Device Location not recognized for searching: '%d'\n",deviceLocation);
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

				//TODO Request space to Swap

			}else{//No empty entry is present in TLB

				//Execute LRU algorithm
				executeLRUAlgorithm(memoryElement, virtualAddress);
			}
		}
	}else{//PAGE Fault in Main Memory

		if (list_size(freeFramesList) > 0){

			//The main memory still has free frames available
			printf("The main memory still has free frames to assign to process '%d'", activePID);

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
				printf("The process '%d' still has free frames to assigned", activePID);

				void *memoryBlockOffset = NULL;
				memoryBlockOffset = &memBlock + (memoryElement->frameNumber * configuration.frames_size) + virtualAddress->offset;

				pthread_mutex_lock(&memoryAccessMutex);//Locking mutex for writing memory
				memcpy(memoryBlockOffset, memoryContent , virtualAddress->size);
				pthread_mutex_unlock(&memoryAccessMutex);//unlocking mutex for writing memory

			}else{
				//The active process hasn't free frames available, replacement algorithm has to be executed
				printf("The process '%d' hasn't more free frames available", activePID);

				//  ejecuto algoritmo reemplazo segun deviceLocation ( TLB=> LRU, MAIN_MEMORY=> Clock/mejorado)

				if(TLBActivated){//TLB is enable
					//Execute LRU algorithm
					executeLRUAlgorithm(memoryElement, virtualAddress);
				}else{
					//TODO execute main memory algorithm
				}

			}
		}else{
			//The main memory still hasn't any free frames
			printf("The main memory is Full!");
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

	//check page status before replacing
	checkPageModification(LRUCandidate);

	//Load new program structure into candidate TLB entry
	LRUCandidate->PID = newElement->PID;
	LRUCandidate->dirtyBit = newElement->dirtyBit;
	LRUCandidate->presentBit = newElement->presentBit;
	LRUCandidate->virtualAddress = virtualAddress;
	LRUCandidate->frameNumber = newElement->frameNumber;

	//updated TLB by LRU algorithm
	updateTLBPositionsbyLRU(LRUCandidate);
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
	activePID = PID;
}
