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

	//Initializing mutex
	pthread_mutex_init(&memoryAccessMutex, NULL);
	pthread_mutex_init(&activeProcessMutex, NULL);

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
				pthread_mutex_lock(&activeProcessMutex);
				//TODO deserealize messageRcv
				//TODO get PID from message and process
				int PID;//TODO cambiar por el de la estructura que deserealice
				changeActiveProcess(PID);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			case NUCLEO:{
				printf("Processing NUCLEO message received\n");
				pthread_mutex_lock(&activeProcessMutex);
				//TODO deserealize messageRcv
				//TODO get PID from message and process
				int PID;//TODO cambiar por el de la estructura que deserealice
				changeActiveProcess(PID);
				pthread_mutex_unlock(&activeProcessMutex);
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

	//TODO inform new program to swap and check if it could write it.

}

void endProgram(int PID){

	changeActiveProcess(PID);

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

	//TODO Notify to Swap for freeing space - Se puede tirar un hilo para que lo haga mientras la UMC continua y se puede desbloquear para seguir su procesamiento de mensajes.

}

void *requestBytesFromPage(t_memoryLocation virtualAddress){
	void *memoryContent;
	t_memoryAdmin *memoryElement;
	void *memoryBlockOffset = NULL;

	getElementFrameNro(&virtualAddress, READ, memoryElement);

	if (memoryElement != NULL){ //PAGE HIT and NO errors from getFrame
		//delay memory access
		waitForResponse();

		memoryBlockOffset = &memBlock + (memoryElement->frameNumber * configuration.frames_size) + virtualAddress.offset;

		pthread_mutex_lock(&memoryAccessMutex);//Checking mutex before reading
		memcpy(memoryContent, memoryBlockOffset , virtualAddress.size);
		pthread_mutex_unlock(&memoryAccessMutex);
	}

	return memoryContent;
}

void writeBytesToPage(t_memoryLocation virtualAddress, void *buffer){
	t_memoryAdmin *memoryElement;
	void *memoryBlockOffset = NULL;

	getElementFrameNro(&virtualAddress, WRITE, memoryElement);

	if (memoryElement != NULL){ //PAGE HIT and NO errors from getFrame
		//delay memory access
		waitForResponse();

		memoryBlockOffset = &memBlock + (memoryElement->frameNumber * configuration.frames_size) + virtualAddress.offset;

		pthread_mutex_lock(&memoryAccessMutex);//Locking mutex for writing memory
		memcpy(memoryBlockOffset, buffer , virtualAddress.size);
		pthread_mutex_unlock(&memoryAccessMutex);//unlocking mutex for writing memory
	}

}

/******************* Auxiliary functions *******************/

void iteratePageTablexProc(t_pageTablesxProc *pageTablexProc){
	list_iterate(pageTablexProc->ptrPageTable, (void*) markElementModified);
}

void markElementModified(t_memoryAdmin *pageTableElement){
	pageTableElement->dirtyBit = PAGE_MODIFIED;
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
		//TODO overwrite page content in swap (swap out)
	}
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
			void *memoryContent = requestPageToSwap(virtualAddress);

			//request memory for new element
			t_memoryAdmin *memoryElement = malloc(sizeof(t_memoryAdmin));

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
					//TODO execute main menory algorithm
				}

			}
		}else{
			//The main memory still hasn't any free frames
			printf("The main memory is Full!");
			//TODO inform this to upstream process
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

void *requestPageToSwap(t_memoryLocation *virtualAddress){
	void *memoryContent = NULL;

	//TODO request page content to swap

	return memoryContent;
}

void waitForResponse(){
	sleep(configuration.delay);
}

void changeActiveProcess(int PID){
	activePID = PID;
}
