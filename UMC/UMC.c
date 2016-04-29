#include "UMC.h"

int main(){
	int exitCode = EXIT_SUCCESS ; //Normal completition
	configFile configuration;
	int socketServer;
	int socketClient;

	configuration.port = 8080;

	exitCode = openServerConnection(configuration.port, &socketServer);

	//If exitCode == 0 the server connection is openned and listenning
	if (exitCode == 0) {
		puts ("the server is openned");
		acceptClientConnection(&socketServer, &socketClient);
		send (socketClient, "Hola Netcat!\n",14,0);// TODO 1) aca debe ir la primera parte del handshake


		char* messageRcv = malloc(5);
		int receivedBytes = recv(socketClient, messageRcv, 4, 0); // TODO 2) aca debe ir la segunda parte del handshake (con el protocolo definido)

		//Now it's checked that the client is not down
		if ( receivedBytes < 0 ){
			perror("One of the clients is down!"); //TODO => Agregar logs con librerias
			printf("Please check the client: %d is down!", socketClient);
			return EXIT_FAILURE;
		}

		messageRcv[receivedBytes] = '\0';
		printf("El cliente envio: %s\n",messageRcv );

		free(messageRcv);
	}

	return exitCode;
}
