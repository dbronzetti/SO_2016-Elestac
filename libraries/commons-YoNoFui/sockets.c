#include "sockets.h"

int openServerConnection(int newSocketServerPort, int *socketServer){
	int exitcode = EXIT_SUCCESS; //Normal completition

	struct sockaddr_in newSocketInfo;

	assert(("ERROR - Server port is the DEFAULT (=0)", newSocketServerPort != 0)); // FAILS if the Server port is equal to default value (0) TODO => Agregar logs con librerias

	newSocketInfo.sin_family = AF_INET; //AF_INET = IPv4
	newSocketInfo.sin_addr.s_addr = INADDR_ANY; //
	newSocketInfo.sin_port = htons(newSocketServerPort);

	*socketServer = socket(AF_INET, SOCK_STREAM, 0);

	int socketActivated = 1;

	//This line is to notify to the SO that the socket created is going to be reused
	setsockopt(*socketServer, SOL_SOCKET, SO_REUSEADDR, &socketActivated, sizeof(socketActivated));

	if (bind(*socketServer, (void*) &newSocketInfo, sizeof(newSocketInfo)) != 0){
		perror("Failed bind in openServerConnection()\n"); //TODO => Agregar logs con librerias
		printf("Please check whether another process is using port: %d \n",newSocketServerPort);

		//Free socket created
		close(*socketServer);

		exitcode = EXIT_FAILURE;
		return exitcode;
	}

	listen(*socketServer, SOMAXCONN); //SOMAXCONN = Constant with the maximum connections allowed by the SO

	return exitcode;
}

int openClientConnection(char *IPServer, int PortServer, int *socketClient){
	int exitcode = EXIT_SUCCESS; //Normal completition

	struct sockaddr_in serverSocketInfo;

	serverSocketInfo.sin_family = AF_INET; //AF_INET = IPv4
	serverSocketInfo.sin_addr.s_addr = inet_addr(IPServer); //
	serverSocketInfo.sin_port = htons(PortServer);

	*socketClient = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(*socketClient, (void*) &serverSocketInfo, sizeof(serverSocketInfo)) != 0){
		perror("Failed connect to server in OpenClientConnection()"); //TODO => Agregar logs con librerias
		printf("Please check whether the server '%s' is up or the correct port is: %d \n",IPServer, PortServer);

		//Free socket created
		close(*socketClient);
		exitcode = EXIT_FAILURE;
		return exitcode;
	}

	return exitcode;
}

int acceptClientConnection(int *socketServer, int *socketClient){
	int exitcode = EXIT_SUCCESS; //Normal completition
	struct sockaddr_in clientConnection;
	unsigned int addressSize = sizeof(clientConnection); //The addressSize has to be initialized with the size of sockaddr_in before passing it to accept function

	*socketClient = accept(*socketServer, (void*) &clientConnection, &addressSize);

	if (*socketClient != -1){
		printf("The was received a connection in: %d.\n", *socketClient);
	}else{
		perror("Failed to get a new connection"); //TODO => Agregar logs con librerias
		exitcode = EXIT_FAILURE;
	}

	return exitcode;
}

int sendClientHandShake(int *socketClient, enum_processes process){
	int exitcode = EXIT_SUCCESS; //Normal completition
	int bufferSize = 0;
	int messageLen = 0;
	int payloadSize = 0;
	char *processString;

	t_MessageGenericHandshake *messageACK = malloc(sizeof(t_MessageGenericHandshake));
	messageACK->process = process;

	switch (process){
		case CONSOLA:{
			processString = "CONSOLA";
			break;
		}
		case NUCLEO:{
			processString = "NUCLEO";
			break;
		}
		case UMC:{
			processString = "UMC";
			break;
		}
		case SWAP:{
			processString = "SWAP";
			break;
		}
		case CPU:{
			processString = "CPU";
			break;
		}
		default:{
			perror("Process not recognized");//TODO => Agregar logs con librerias
			printf("Invalid process '%d' tried to send a message\n",(int) process);
			return exitcode = EXIT_FAILURE;
		}
	}

	messageACK->message = malloc(sizeof(char[60]));
	sprintf(messageACK->message, "The process '%s' is trying to connect you!\0", processString);
	messageLen = strlen(messageACK->message);

	payloadSize = sizeof(messageACK->process) + sizeof(messageLen) + messageLen + 1; // process + length message + message + 1 (+1 because of '\0')
	bufferSize = sizeof(bufferSize) + payloadSize ;//+1 because of '\0'
	char *buffer = malloc(bufferSize);
	serializeHandShake(messageACK, buffer, bufferSize);

	send(*socketClient, (void*) buffer, bufferSize,0);

	free(buffer);
	free(messageACK->message);
	free(messageACK);

	return exitcode;
}

int sendClientAcceptation(int *socketClient){
	int exitcode = EXIT_SUCCESS; //Normal completition
	int bufferSize = 0;
	int messageLen = 0;
	int payloadSize = 0;

	t_MessageGenericHandshake *messageACK = malloc(sizeof(t_MessageGenericHandshake));
	messageACK->process = ACCEPTED;
	messageACK->message = "The server has accepted your connection!\0"; //ALWAYS put \0 for finishing the string
	messageLen = strlen(messageACK->message);

	payloadSize = sizeof(messageACK->process) + sizeof(messageLen) + messageLen + 1; // process + length message + message + 1 (+1 because of '\0')
	bufferSize = sizeof(bufferSize) + payloadSize ;//+1 because of '\0'
	char *buffer = malloc(bufferSize);
	serializeHandShake(messageACK, buffer, bufferSize);

	exitcode = send(*socketClient, (void*) buffer, bufferSize,0) == -1 ? EXIT_FAILURE : EXIT_SUCCESS ;

	free(buffer);
	free(messageACK);
	return exitcode;
}

int sendMessage (int *socketClient, char *buffer, int bufferSize){
	int exitcode = EXIT_SUCCESS; //Normal completition

	exitcode = send(*socketClient, (void*) buffer, bufferSize,0) == -1 ? EXIT_FAILURE : EXIT_SUCCESS ;

	return exitcode;
}

int receiveMessage(int *socketClient, char *messageRcv, int bufferSize){

	int receivedBytes = recv(*socketClient, (void*) messageRcv, bufferSize, 0);

	return receivedBytes;
}

void serializeHandShake(t_MessageGenericHandshake *value, char *buffer, int valueSize){
    int offset = 0;

    //0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

    //1)process
    memcpy(buffer + offset, &value->process, sizeof(value->process));
    offset += sizeof(value->process);

    //2)message length
    int messageLen = strlen(value->message) + 1;//+1 because of '\0'
	memcpy(buffer + offset, &messageLen, sizeof(messageLen));
	offset += sizeof(messageLen);

    //3)message
    memcpy(buffer + offset, value->message, messageLen);

}

void deserializeHandShake(t_MessageGenericHandshake *value, char *bufferReceived){
    int offset = 0;
    int messageLen = 0;

    //1)process
    memcpy(&value->process, bufferReceived, sizeof(value->process));
    offset += sizeof(value->process);

    //2)message length
	memcpy(&messageLen, bufferReceived + offset, sizeof(messageLen));
	offset += sizeof(messageLen);

    //3)message
	value->message = malloc(messageLen);
    memcpy(value->message, bufferReceived + offset, messageLen);

}

void serializeUMC_Swap(t_MessageUMC_Swap *value, char *buffer, int valueSize){
	int offset = 0;
    enum_processes process = UMC;

    //0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//1)From process
	memcpy(buffer, &process, sizeof(process));
	offset += sizeof(process);

    //2)pageNro
    memcpy(buffer, &value->pageNro, sizeof(value->pageNro));
    offset += sizeof(value->pageNro);

    //3)processID
    memcpy(buffer + offset, &value->processID, sizeof(value->processID));
    offset += sizeof(value->processID);

    //4)processStatus
    memcpy(buffer + offset, &value->processStatus, sizeof(value->processStatus));
	offset += sizeof(value->processStatus);

	//5)totalPages
	memcpy(buffer + offset, &value->totalPages, sizeof(value->totalPages));
	offset += sizeof(value->totalPages);

}

void deserializeSwap_UMC(t_MessageUMC_Swap *value, char *bufferReceived){
    int offset = 0;

    //2)pageNro
    memcpy(&value->pageNro, bufferReceived, sizeof(value->pageNro));
    offset += sizeof(value->pageNro);

    //3)processID
    memcpy(&value->processID, bufferReceived + offset, sizeof(value->processID));
    offset += sizeof(value->processID);

    //4)processStatus
    memcpy(&value->processStatus, bufferReceived + offset, sizeof(value->processStatus));
	offset += sizeof(value->processStatus);

	//5)totalPages
	memcpy(&value->totalPages, bufferReceived + offset, sizeof(value->totalPages));
	offset += sizeof(value->totalPages);

}

void serializeNucleo_CPU(t_MessageNucleo_CPU *value, char *buffer, int valueSize) {
	int offset = 0;
	enum_processes process = NUCLEO;

	//0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//1)From process
	memcpy(buffer, &process, sizeof(process));
	offset += sizeof(process);

	//2)head
	memcpy(buffer + offset, &value->head, sizeof(value->head));
	offset += sizeof(value->head);

	//3)processID
	memcpy(buffer + offset, &value->processID, sizeof(value->processID));
	offset += sizeof(value->processID);

	//4)path
	memcpy(buffer + offset, &value->path, 250);
	offset += 250;

	//5)pc
	memcpy(buffer + offset, &value->pc, sizeof(value->pc));
	offset += sizeof(value->pc);

	//6)cantInstruc
	memcpy(buffer + offset, &value->cantInstruc, sizeof(value->cantInstruc));

}

void deserializeCPU_Nucleo(t_MessageNucleo_CPU *value, char * bufferReceived) {
	int offset = 0;

	memcpy(&value->operacion, bufferReceived, sizeof(value->operacion));
	offset += sizeof(value->operacion);

	memcpy(&value->processID, bufferReceived + offset, sizeof(value->processID));
	offset += sizeof(value->processID);
}


/* EJEMPLO DE COMO CREAR UN CLIENTE Y MANDAR MENSAJES AL SERVER
int socketClient;

	char ip[15] = "127.0.0.1";
	int exitcode = openClientConnection(&ip, 8080, &socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == 0){

		// ***1) Send handshake
		sendClientHandShake(&socketClient,SWAP);

		// ***2)Receive handshake response
		//Receive message size
		int messageSize = 0;
		char *messageRcv = malloc(sizeof(messageSize));
		int receivedBytes = receiveMessage(&socketClient, messageRcv, sizeof(messageSize));

		//Receive message using the size read before
		memcpy(&messageSize, messageRcv, sizeof(int));
		printf("messageRcv received: %s\n",messageRcv);
		printf("messageSize received: %d\n",messageSize);
		messageRcv = realloc(messageRcv,messageSize);
		receivedBytes = receiveMessage(&socketClient, messageRcv, messageSize);

		printf("bytes received: %d\n",receivedBytes);

		//starting handshake with client connected
		t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
		deserializeHandShake(message, messageRcv);

		switch (message->process){
			case ACCEPTED:{
				printf("%s\n",message->message);
				break;
			}
			default:{
				perror("Process couldn't connect to SERVER");//TODO => Agregar logs con librerias
				printf("Not able to connect to server %s. Please check if it's down.\n",ip);
				break;
			}
		}

		while(1){
			//aca tiene que ir una validacion para ver si el server sigue arriba
			//send(socketClient, msg, sizeof(msg),0);
		}

	}else{
		perror("no me pude conectar al server!"); //
		printf("mi socket es: %d\n", socketClient);
	}

	return exitcode;

 */


