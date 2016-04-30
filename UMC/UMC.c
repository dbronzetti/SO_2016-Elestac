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

		//TODO disparar un thread para cada cliente nuevo
		acceptClientConnection(&socketServer, &socketClient);
		send (socketClient, "Hola Netcat!\n",14,0);// TODO 1) aca debe ir la primera parte del handshake


		char* messageRcv = malloc(1000);

		while (1){
			int receivedBytes = recv(socketClient, messageRcv, 1000, 0); // TODO 2) aca debe ir la segunda parte del handshake (con el protocolo definido)

			//Now it's checked that the client is not down
			if ( receivedBytes <= 0 ){
				perror("One of the clients is down!"); //TODO => Agregar logs con librerias
				printf("Please check the client: %d is down!\n", socketClient);
				return EXIT_FAILURE;
			}

			//'\0' marks the termination of the message received
			messageRcv[receivedBytes] = '\0';
			printf("El cliente envio: %s\n",messageRcv );
		}

		free(messageRcv);
	}

	return exitCode;
}

/* EJEMPLO DE COMO CREAR UN CLIENTE Y MANDAR MENSAJES AL SERVER
 	int socketClient;

	char ip[15] = "127.0.0.1";
	int exitcode = openClientConnection(&ip, 8080, &socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == 0){

		while(1){
			char msg[1000];
			scanf("%s",msg);

			//aca tiene que ir una validacion para ver si el server sigue arriba
			send(socketClient, msg, strlen(msg),0);
		}

	}else{
		perror("no me pude conectar al server!"); //
		printf("mi socket es: %d\n", socketClient);
	}

	return exitcode;
 */

