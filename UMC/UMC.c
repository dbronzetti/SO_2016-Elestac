#include "UMC.h"

int main(){
	int exitCode = EXIT_SUCCESS ; //Normal completition
	configFile configuration;
	int socketServer;
	int socketClient;
	int highestDescriptor;
	int socketCounter;
	fd_set readSocketSet;

	configuration.port = 8080;

	exitCode = openServerConnection(configuration.port, &socketServer);
	printf("socketServer: %d\n",socketServer);

	//Clear socket set
	FD_ZERO(&readSocketSet);

	//If exitCode == 0 the server connection is openned and listenning
	if (exitCode == 0) {
		puts ("the server is openned");

		//
		highestDescriptor = socketServer;
		//Add socket server to the set
		FD_SET(socketServer,&readSocketSet);

		while (1){

			do{
				exitCode = select(highestDescriptor + 1, &readSocketSet, NULL, NULL, NULL);
			} while (exitCode == -1 && errno == EINTR);

			if (exitCode > EXIT_SUCCESS){
				//Checking new connections in server socket
				if (FD_ISSET(socketServer, &readSocketSet)){
					//The server has detected activity, a new connection has been received
					//TODO disparar un thread para acceptar cada cliente nuevo (debido a que el accept es bloqueante) y para hacer el handshake
					exitCode = acceptClientConnection(&socketServer, &socketClient);

					if (exitCode == EXIT_FAILURE){
						printf("There was detected an attempt of wrong connection\n");//TODO => Agregar logs con librerias
					}else{
						//TODO posiblemente aca haya que disparar otro thread para que haga el recv y continue recibiendo conexiones al mismo tiempo
						//starting handshake with client connected
						t_MessagePackage *messageRcv = malloc(sizeof(t_MessagePackage));
						int receivedBytes = receiveMessage(&socketClient, messageRcv);

						//Now it's checked that the client is not down
						if ( receivedBytes <= 0 ){
							perror("One of the clients is down!"); //TODO => Agregar logs con librerias
							printf("Please check the client: %d is down!\n", socketClient);
							FD_CLR(socketClient, &readSocketSet);
							close(socketClient);
						}else{
							switch ((int)messageRcv->process){
								case SWAP:
									exitCode = sendClientAcceptation(&socketClient, &readSocketSet);
									highestDescriptor = (highestDescriptor < socketClient)?socketClient: highestDescriptor;
									break;
								case CPU:
									exitCode = sendClientAcceptation(&socketClient, &readSocketSet);
									highestDescriptor = (highestDescriptor < socketClient)?socketClient: highestDescriptor;
									break;
								default:
									perror("Process not allowed to connect");//TODO => Agregar logs con librerias
									printf("Invalid process '%d' tried to connect to UMC\n",(int) messageRcv->process);
									close(socketClient);
									break;
							}
						}
						free(messageRcv);
					}// END handshake
				}

				//check activity in all the descriptors from the set
//				for(socketCounter=0 ; socketCounter<highestDescriptor+1; socketCounter++){
//					if (FD_ISSET(socketCounter,&readSocketSet)){
//						t_MessagePackage *messageRcv = malloc(sizeof(t_MessagePackage));
//						int receivedBytes = receiveMessage(&socketCounter, messageRcv);
//
//						//Now it's checked that the client is not down
//						if ( receivedBytes <= 0 ){
//							perror("One of the clients is down!"); //TODO => Agregar logs con librerias
//							printf("Please check the client: %d is down!\n", socketCounter);
//							FD_CLR(socketCounter, &readSocketSet);
//							close(socketCounter);
//						}else{
//							//TODO Debo implementar las acciones con el mensaje recibido
//							printf("El cliente envio: %s\n",(char*) messageRcv->message );
//						}
//
//					free(messageRcv);
//					}
//				}//end checking activity
			}
		}
	}

	//cleaning set
	FD_ZERO(&readSocketSet);

	return exitCode;
}


/* EJEMPLO DE COMO CREAR UN CLIENTE Y MANDAR MENSAJES AL SERVER
 	int socketClient;

	char ip[15] = "127.0.0.1";
	int exitcode = openClientConnection(&ip, 8080, &socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == 0){

		char *package = "Envie una prueba por paquete";
		t_MessagePackage *msg = malloc(sizeof(t_MessagePackage));
		msg->process = '1';
		msg->message = &package;

		while(1){
			//aca tiene que ir una validacion para ver si el server sigue arriba
			send(socketClient, msg, sizeof(msg),0);
		}

	}else{
		perror("no me pude conectar al server!"); //
		printf("mi socket es: %d\n", socketClient);
	}

	return exitcode;
 */

