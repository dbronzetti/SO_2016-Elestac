#include "sockets.h"


int openServerConnection(int newSocketServerPort, int *socketServer){
	int exitcode = EXIT_SUCCESS; //Normal completition

	struct sockaddr_in newSocketInfo;

	newSocketInfo.sin_family = AF_INET; //AF_INET = IPv4
	newSocketInfo.sin_addr.s_addr = INADDR_ANY; //
	newSocketInfo.sin_port = htons(newSocketServerPort);

	*socketServer = socket(AF_INET, SOCK_STREAM, 0);

	int socketActivated = 1;
	setsockopt(*socketServer, SOL_SOCKET, SO_REUSEADDR, &socketActivated, sizeof(socketActivated)); //This line is to notify to the SO that the socket created is going to be reused

	if (bind(*socketServer, (void*) &newSocketInfo, sizeof(newSocketInfo)) != 0){
		perror("Failed bind in openServerConnection()\n"); //TODO => Agregar logs con librerias
		printf("Please check whether another process is using port: %d \n",newSocketServerPort);
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
		perror("Failed connect to server in OpenClientConnection()\n"); //TODO => Agregar logs con librerias
		printf("Please check whether the server '%s' is up or the correct port is: %d \n",IPServer, PortServer);
		exitcode = EXIT_FAILURE;
		return exitcode;
	}

	return exitcode;
}

int acceptClientConnection(int *socketServer, int *socketClient){
	int exitcode = EXIT_SUCCESS; //Normal completition
	struct sockaddr_in clientConnection;
	unsigned int addressSize;

	*socketClient = accept(*socketServer, (void*) &clientConnection, &addressSize);

	printf("The was received a connection in: %d.\n", *socketClient);

	return exitcode;
}



