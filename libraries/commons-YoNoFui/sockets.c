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
		printf("The was received a connection in socket: %d.\n", *socketClient);
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

	processString = getProcessString(process);

	messageACK->message = string_new();
	string_append_with_format(&messageACK->message,"The process '%s' is trying to connect you!\0",processString);
	messageLen = strlen(messageACK->message);

	payloadSize = sizeof(messageACK->process) + sizeof(messageLen) + messageLen + 1; // process + length message + message + 1 (+1 because of '\0')
	bufferSize = sizeof(bufferSize) + payloadSize ;//+1 because of '\0'

	char *buffer = malloc(bufferSize);
	serializeHandShake(messageACK, buffer, payloadSize);//has to be sent the PAYLOAD size!!

	exitcode = send(*socketClient, (void*) buffer, bufferSize,0) == -1 ? EXIT_FAILURE : EXIT_SUCCESS ;

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
	messageACK->message = string_new();
	string_append(&messageACK->message,"The server has accepted your connection!\0");//ALWAYS put \0 for finishing the string
	messageLen = strlen(messageACK->message);

	payloadSize = sizeof(messageACK->process) + sizeof(messageLen) + messageLen + 1; // process + length message + message + 1 (+1 because of '\0')
	bufferSize = sizeof(bufferSize) + payloadSize ;//+1 because of '\0'

	char *buffer = malloc(bufferSize);
	serializeHandShake(messageACK, buffer, payloadSize);//has to be sent the PAYLOAD size!!

	exitcode = send(*socketClient, (void*) buffer, bufferSize,0) == -1 ? EXIT_FAILURE : EXIT_SUCCESS ;

	free(buffer);
	free(messageACK->message);
	free(messageACK);
	return exitcode;
}

int sendMessage (int *socketClient, void *buffer, int bufferSize){
	int exitcode = EXIT_SUCCESS; //Normal completition

	exitcode = send(*socketClient, buffer, bufferSize,0) == -1 ? EXIT_FAILURE : EXIT_SUCCESS ;

	return exitcode;
}

int receiveMessage(int *socketClient, void *messageRcv, int bufferSize){

	int receivedBytes = recv(*socketClient, messageRcv, bufferSize, 0);

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

void serializeCPU_UMC(t_MessageCPU_UMC *value, char *buffer, int valueSize){
	int offset = 0;
	enum_processes process = CPU;

	//0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//1)From process
	memcpy(buffer, &process, sizeof(process));
	offset += sizeof(process);

	//2)operation
	memcpy(buffer + offset, &value->operation, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)processID
	memcpy(buffer + offset, &value->PID, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)pageNro
	memcpy(buffer, &value->virtualAddress->pag, sizeof(value->virtualAddress->pag));
	offset += sizeof(value->virtualAddress->pag);

	//5)offset
	memcpy(buffer, &value->virtualAddress->offset, sizeof(value->virtualAddress->offset));
	offset += sizeof(value->virtualAddress->offset);

	//6)size
	memcpy(buffer, &value->virtualAddress->size, sizeof(value->virtualAddress->size));
	offset += sizeof(value->virtualAddress->size);

}

void deserializeUMC_CPU(t_MessageCPU_UMC *value, char *bufferReceived){
	int offset = 0;

	//2)operation
	memcpy(&value->operation, bufferReceived, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)processID
	memcpy(&value->PID, bufferReceived, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)pageNro
	memcpy(&value->virtualAddress->pag, bufferReceived + offset, sizeof(value->virtualAddress->pag));
	offset += sizeof(value->virtualAddress->pag);

	//5)offset
	memcpy(&value->virtualAddress->offset, bufferReceived + offset, sizeof(value->virtualAddress->offset));
	offset += sizeof(value->virtualAddress->offset);

	//6)size
	memcpy(&value->virtualAddress->size, bufferReceived + offset, sizeof(value->virtualAddress->size));
	offset += sizeof(value->virtualAddress->size);
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

	//2)operation
	memcpy(buffer + offset, &value->operation, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)PID
	memcpy(buffer + offset, &value->PID, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)pageNro
	memcpy(buffer, &value->virtualAddress->pag, sizeof(value->virtualAddress->pag));
	offset += sizeof(value->virtualAddress->pag);

	//5)offset
	memcpy(buffer, &value->virtualAddress->offset, sizeof(value->virtualAddress->offset));
	offset += sizeof(value->virtualAddress->offset);

	//6)size
	memcpy(buffer, &value->virtualAddress->size, sizeof(value->virtualAddress->size));
	offset += sizeof(value->virtualAddress->size);

	//7)page quantity
	memcpy(buffer, &value->cantPages, sizeof(value->cantPages));
	offset += sizeof(value->cantPages);
}

void deserializeSwap_UMC(t_MessageUMC_Swap *value, char *bufferReceived){
	int offset = 0;

	//2)operation
	memcpy(&value->operation, bufferReceived, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)PID
	memcpy(&value->PID, bufferReceived, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)pageNro
	memcpy(&value->virtualAddress->pag, bufferReceived + offset, sizeof(value->virtualAddress->pag));
	offset += sizeof(value->virtualAddress->pag);

	//5)offset
	memcpy(&value->virtualAddress->offset, bufferReceived + offset, sizeof(value->virtualAddress->offset));
	offset += sizeof(value->virtualAddress->offset);

	//6)size
	memcpy(&value->virtualAddress->size, bufferReceived + offset, sizeof(value->virtualAddress->size));
	offset += sizeof(value->virtualAddress->size);

	//7)page quantity
	memcpy(&value->cantPages, bufferReceived + offset, sizeof(value->cantPages));
	offset += sizeof(value->cantPages);
}


void serializeNucleo_UMC(t_MessageNucleo_UMC *value, char *buffer, int valueSize){
	int offset = 0;
	enum_processes process = NUCLEO;

	//0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//1)From process
	memcpy(buffer, &process, sizeof(process));
	offset += sizeof(process);

	//2)operation
	memcpy(buffer + offset, &value->operation, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)PID
	memcpy(buffer + offset, &value->PID, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)page quantity
	memcpy(buffer, &value->cantPages, sizeof(value->cantPages));
	offset += sizeof(value->cantPages);
}

void deserializeUMC_Nucleo(t_MessageNucleo_UMC *value, char *bufferReceived){
	int offset = 0;

	//2)operation
	memcpy(&value->operation, bufferReceived, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)PID
	memcpy(&value->PID, bufferReceived, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)page quantity
	memcpy(&value->cantPages, bufferReceived + offset, sizeof(value->cantPages));
	offset += sizeof(value->cantPages);
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

    //4)processStatus
    memcpy(buffer + offset, &value->processStatus, sizeof(value->processStatus));
	offset += sizeof(value->processStatus);

	//5)codeScript
	memcpy(buffer + offset, &value->codeScript, (strlen(value->codeScript)));
	offset += strlen(value->codeScript);

	//6)pc
	memcpy(buffer + offset, &value->ProgramCounter, sizeof(value->ProgramCounter));
	offset += sizeof(value->ProgramCounter);

	//7)cantInstruc
	memcpy(buffer + offset, &value->cantInstruc, sizeof(value->cantInstruc));
	offset += sizeof(value->cantInstruc);

	//8)operacion
	memcpy(buffer + offset, &value->operacion, sizeof(value->operacion));
	offset += sizeof(value->operacion);

	//9)numSocket
	memcpy(buffer + offset, &value->numSocket, sizeof(value->numSocket));

}

void deserializeCPU_Nucleo(t_MessageNucleo_CPU *value, char * bufferReceived) {
	int offset = 0;

	//2)head
	memcpy(&value->head, bufferReceived, sizeof(value->head));
	offset += sizeof(value->head);

	//3)processID
	memcpy(&value->processID, bufferReceived + offset, sizeof(value->processID));
	offset += sizeof(value->processID);

	//4)processStatus
	memcpy(&value->processStatus, bufferReceived + offset, sizeof(value->processStatus));
	offset += sizeof(value->processStatus);

	//5)codeScript
	memcpy(&value->codeScript, bufferReceived + offset,(strlen(value->codeScript)));
	offset += strlen(value->codeScript);

	//6)ProgramCounter
	memcpy(&value->ProgramCounter, bufferReceived + offset, sizeof(value->ProgramCounter));
	offset += sizeof(value->ProgramCounter);

	//7)cantInstruc
	memcpy(&value->cantInstruc, bufferReceived + offset, sizeof(value->cantInstruc));
	offset += sizeof(value->cantInstruc);

	//8)operacion
	memcpy(&value->operacion, bufferReceived + offset, sizeof(value->operacion));
	offset += sizeof(value->operacion);

	//9)numSocket
	memcpy(&value->numSocket, bufferReceived + offset, sizeof(value->numSocket));

}

void deserializeConsola_Nucleo(t_MessageNucleo_Consola *value, char *bufferReceived){
	int offset = 0;

	//1)codeScript
	memcpy(&value->codeScript, bufferReceived + offset,(strlen(value->codeScript)));
	offset += strlen(value->codeScript);

	//2)processID
	memcpy(&value->processID, bufferReceived + offset, sizeof(value->processID));
	offset += sizeof(value->processID);

	//3)processStatus
	memcpy(&value->processStatus, bufferReceived + offset, sizeof(value->processStatus));

}

char *getProcessString (enum_processes process){

	char *processString;
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
			processString = NULL;
		}
	}
	return processString;
}

/* EJEMPLO DE COMO CREAR UN CLIENTE Y MANDAR MENSAJES AL SERVER
int socketClient;

	char *ip= "127.0.0.1";
	int exitcode = openClientConnection(ip, 8080, &socketClient);

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


