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

			//TODO disparar un thread para revisar actividad detectada
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

						//Receive message size
						int messageSize = 0;
						char *messageRcv = malloc(sizeof(messageSize));
						int receivedBytes = receiveMessage(&socketClient, messageRcv, sizeof(messageSize));

						//Receive message using the size read before
						memcpy(&messageSize, messageRcv, sizeof(int));
						//printf("messageSize received: %d\n",messageSize);
						messageRcv = realloc(messageRcv,messageSize);
						receivedBytes = receiveMessage(&socketClient, messageRcv, messageSize);

						printf("bytes received in message: %d\n",receivedBytes);

						//starting handshake with client connected
						t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
						deserializeHandShake(message, messageRcv);

						//Now it's checked that the client is not down
						if ( receivedBytes == 0 ){
							perror("One of the clients is down!"); //TODO => Agregar logs con librerias
							printf("Please check the client: %d is down!\n", socketClient);
							FD_CLR(socketClient, &readSocketSet);
							close(socketClient);
						}else{
							switch ((int) message->process){
								case SWAP:{
									printf("%s\n",message->message);
									exitCode = sendClientAcceptation(&socketClient, &readSocketSet);
									highestDescriptor = (highestDescriptor < socketClient)?socketClient: highestDescriptor;
									break;
								}
								case CPU:{
									printf("%s\n",message->message);
									exitCode = sendClientAcceptation(&socketClient, &readSocketSet);
									highestDescriptor = (highestDescriptor < socketClient)?socketClient: highestDescriptor;
									break;
								}
								case NUCLEO:{
									printf("%s\n",message->message);
									exitCode = sendClientAcceptation(&socketClient, &readSocketSet);
									highestDescriptor = (highestDescriptor < socketClient)?socketClient: highestDescriptor;
									break;
								}
								default:{
									perror("Process not allowed to connect");//TODO => Agregar logs con librerias
									printf("Invalid process '%d' tried to connect to UMC\n",(int) message->process);
									close(socketClient);
									break;
								}
							}
						}

						free(messageRcv);
						free(message->message);
						free(message);
					}// END handshake
				}

				//check activity in all the descriptors from the set
				for(socketCounter=0 ; (socketCounter<highestDescriptor+1); socketCounter++){
					//Skipping check the server activity again
					if (socketServer != socketCounter && FD_ISSET(socketCounter,&readSocketSet)){

						//Receive message size
						int messageSize = 0;
						char *messageRcv = malloc(sizeof(messageSize));
						int receivedBytes = receiveMessage(&socketClient, messageRcv, sizeof(messageSize));
						messageSize = atoi(messageRcv);

						//Receive process from which the message is going to be interpreted
						enum_processes fromProcess;
						messageRcv = realloc(messageRcv, sizeof(fromProcess));
						receivedBytes = receiveMessage(&socketClient, messageRcv, sizeof(fromProcess));

						//Receive message using the size read before
						fromProcess = (enum_processes) messageRcv;
						messageRcv = realloc(messageRcv, messageSize);
						receivedBytes = receiveMessage(&socketClient, messageRcv, messageSize);

						printf("bytes received: %d\n",receivedBytes);

						if ( receivedBytes > 0 ){

							switch (fromProcess){
								case SWAP:{
									t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
									deserializeSwap_UMC(message, messageRcv);
									printf("se recibio el processID #%d\n",message->processID);
									free(message);
									break;
								}
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
									close(socketClient);
									break;
								}
							}

						}else if (receivedBytes == 0 ){
							//The client is down when bytes received are 0
							perror("One of the clients is down!"); //TODO => Agregar logs con librerias
							printf("Please check the client: %d is down!\n", socketCounter);
							FD_CLR(socketCounter, &readSocketSet);
							close(socketCounter);
						}else{
							perror("Error - No able to received");//TODO => Agregar logs con librerias
							printf("Error receiving from socket '%d', with error: %d\n",socketCounter,errno);
						}

					free(messageRcv);
					}
				}//end checking activity
			}
		}
	}

	//cleaning set
	FD_ZERO(&readSocketSet);

	return exitCode;
}


