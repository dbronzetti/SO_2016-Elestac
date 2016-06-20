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
		if (strcmp(argv[i], "-c") == 0){
			configurationFile = argv[i+1];
			printf("Configuration File: '%s'\n",configurationFile);
		}
	}

	//ERROR if not configuration parameter was passed
	assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS TODO => Agregar logs con librerias

	//get configuration from file
	getConfiguration(configurationFile);

	//created administration structures for UMC process
	createAdminStructs();

	pthread_mutex_init(&socketMutex, NULL);

	//Create thread for server start
	pthread_create(&serverThread, NULL, (void*) startServer, NULL);

	//Create thread for UMC console
	pthread_create(&consolaThread, NULL, (void*) startUMCConsole, NULL);

	pthread_join(serverThread, NULL);
	pthread_join(consolaThread, NULL);

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

	//TODO disparar un thread para acceptar cada cliente nuevo (debido a que el accept es bloqueante) y para hacer el handshake
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
			case CPU:{
				printf("Processing CPU message received\n");
				break;
			}
			case NUCLEO:{
				printf("Processing NUCLEO message received\n");
				break;
			}
			default:{
				perror("Process not allowed to connect");//TODO => Agregar logs con librerias
				printf("Invalid process '#%d' tried to connect to UMC\n",(int) fromProcess);
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
				configuration.delay = (strcmp(parameter, EOL_DELIMITER) != 0) ? atoi(parameterValue) : 0 /*DEFAULT VALUE*/;
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
			configuration.delay = atoi(option);
			printf("The delay UMC was successfully changed to: %d\n", configuration.delay);

		}else if (strcmp(command,"dump") == 0 ){
			printf("\nCommand entered: '%s %s'\n", command,option);
			printf("== [VALUE]\n");
			printf("all\t\t:: Todos los procesos\n");
			printf("<processName>\t:: Nombre del proceso deseado\n\nPlease enter a value with the above format: ");
			scanf("%s", value);

			printf("A copy of this dump was saved in: \n");

		}else if (strcmp(command,"flush") == 0 ){

			if (strcmp(option, "tlb") == 0){
				list_clean(TLBList);
				resetTLBEntries();

			}else if (strcmp(option, "memory")){

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
	resetTLBEntries();
}

void resetTLBEntries(){
	int i;
	for(i=0; i < configuration.TLB_entries; i++){
		t_memoryAdmin *defaultTLBElement;
		defaultTLBElement->PID = -1; //DEFAULT PID value in TLB
		list_add(TLBList, (void*) defaultTLBElement);
	}
}

void createAdminStructs(){
	//Creating memory block
	memBlock = calloc(configuration.frames_max, configuration.frames_size);

	// Checking if TLB is enable
	if (configuration.TLB_entries != 0){
		//TLB enable
		createTLB();
		TLBActivated = true;
		printf("TLB enable. Size '%d'\n", list_size(TLBList));
	}else{
		printf("TLB disable!.\n");
	}
}

void initializeProgram(int PID, int totalPagesRequired, char *programCode){
	t_memoryAdmin *pageTable;

	pageTable->PID = PID;

}

void endProgram(int PID){

}

void *requestBytesFromPage(t_memoryLocation virtualAddress){
	void *memoryContent;
	int *frameNro;
	void *memoryBlockOffset = NULL;
	enum_memoryStructure memoryStructure;


	if(TLBActivated){//TLB is enable
		memoryStructure = TLB;
		frameNro = searchFramebyPage(memoryStructure, READ,virtualAddress);
	}else{//TLB is disable
		memoryStructure = MAIN_MEMORY;
		frameNro = searchFramebyPage(memoryStructure, READ, virtualAddress);
	}

	if (frameNro == NULL){//PAGE FAUL
		/*
		memoryContent = requestPageToSwap(virtualAddress);
		updateMemoryStructure(memoryStructure, virtualAddress);
		*/
	}

	//delay memory access
	waitForResponse();

	memoryBlockOffset = &memBlock + (*frameNro * configuration.frames_size) + virtualAddress.offset;
	memcpy(memoryContent, memoryBlockOffset , virtualAddress.size);

	return memoryContent;
}

void writeBytesToPage(t_memoryLocation virtualAddress, void *buffer){
	int *frameNro;
	void *memoryBlockOffset = NULL;
	enum_memoryStructure memoryStructure;

	if(TLBActivated){//TLB is enable
		memoryStructure = TLB;
		frameNro = searchFramebyPage(memoryStructure, WRITE, virtualAddress);
	}else{//TLB is disable
		memoryStructure = MAIN_MEMORY;
		frameNro = searchFramebyPage(memoryStructure, WRITE, virtualAddress);
	}

	if (frameNro == NULL){//PAGE FAULT
		/*
			memoryContent = requestPageToSwap(virtualAddress);
			updateMemoryStructure(memoryStructure, virtualAddress);
		 */
	}

	//delay memory access
	waitForResponse();

	memoryBlockOffset = &memBlock + (*frameNro * configuration.frames_size) + virtualAddress.offset;
	memcpy(memoryBlockOffset, buffer , virtualAddress.size);

}

int *searchFramebyPage(enum_memoryStructure deviceLocation, enum_memoryOperations operation, t_memoryLocation virtualAddress){
	int *frame = NULL; // DEFAULT VALUE
	t_memoryAdmin* pageNeeded = NULL;

	//** Find function **//
	bool isPagePresent(t_memoryAdmin* listElement){
		return (listElement->virtualAddress.pag == virtualAddress.pag);
	}

	switch(deviceLocation){
		case(TLB):{
			pageNeeded = (t_memoryAdmin*) list_find(TLBList,(void*) isPagePresent);
			break;
		}
		case(MAIN_MEMORY):{
			pageNeeded = (t_memoryAdmin*) list_find(pageTablesList,(void*) isPagePresent);
			break;
		}
		default:{
			perror("Error - Device Location not recognized for searching");//TODO => Agregar logs con librerias
			printf("Error Device Location not recognized for searching: '%d'\n",deviceLocation);
		}
	}

	if (pageNeeded != NULL){
		//Page found
		*frame = pageNeeded->frameNumber;

		switch(operation){
			case(READ):{
				//After getting the frame needed for reading, mark memory element as present (overwrite it no matter if it was marked as present before)
				pageNeeded->presentBit = PAGE_PRESENT;
				break;
			}
			case (WRITE):{
				//After getting the frame needed for writing, mark memory element as modified
				pageNeeded->dirtyBit = PAGE_MODIFIED;
				break;
			}
		}
	}

	return frame;
}

void updateMemoryStructure(enum_memoryStructure memoryStructure, t_memoryLocation virtualAddress){

	/*
	 * 1) ver marcos disponibles
	 * 1.1) si hay disponibles => cargo pagina en marco
	 * 1.2) si no hay disponibles => ejecuto algoritmo reemplazo segun deviceLocation ( TLB=> LRU, MAIN_MEMORY=> Clock/mejorado)
	 *
	 * 2) Algoritmo de reemplazo segun deviceLocation
	 * 2.1) Si pagina a reemplazar == MODIFIED  =>
	 * 2.1.1) escribo en swap el contenido de memoria principal
	 * 2.1.2) reemplazo por pagina a ser cargada
	 */
}

void waitForResponse(){
	sleep(configuration.delay);
}
