#include "UMC.h"

configFile configuration;

int main(int argc, char *argv[]){
	int exitCode = EXIT_SUCCESS ; //Normal completition
	int socketServer;
	int socketClient;
	int highestDescriptor;
	int socketCounter;
	fd_set readSocketSet;
	char *configurationFile = NULL;

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS TODO => Agregar logs con librerias

	int i;
	for( i = 0; i < argc; i++){
		if (strcmp(argv[i], "-c") == 0){
			configurationFile = argv[i+1];
			printf("Configuration File: '%s'\n",configurationFile);
		}
	}

	assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS TODO => Agregar logs con librerias

	getConfiguration(configurationFile);

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
				//printf("port: %d \n", configuration.port);
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
			case(ENTRADAS_TLB):{ //7
				fscanf(file, "%s",parameterValue);
				configuration.TLB_entries = (strcmp(parameter, EOL_DELIMITER) != 0) ? atoi(parameterValue) : 0 /*DEFAULT VALUE*/;
				break;
			}
			case(RETARDO):{ //8
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

int getEnum(char *string){
	int parameter = -1;

	//printf("string: %s \n", string);

	strcmp(string,"PUERTO") == 0 ? parameter = PUERTO : -1 ;
	strcmp(string,"IP_SWAP") == 0 ? parameter = IP_SWAP : -1 ;
	strcmp(string,"PUERTO_SWAP") == 0 ? parameter = PUERTO_SWAP : -1 ;
	strcmp(string,"MARCOS") == 0 ? parameter = MARCOS : -1 ;
	strcmp(string,"MARCO_SIZE") == 0 ? parameter = MARCO_SIZE : -1 ;
	strcmp(string,"MARCO_X_PROC") == 0 ? parameter = MARCO_X_PROC : -1 ;
	strcmp(string,"ENTRADAS_TLB") == 0 ? parameter = ENTRADAS_TLB : -1 ;
	strcmp(string,"RETARDO") == 0 ? parameter = RETARDO : -1 ;

	//printf("parameter: %d \n", parameter);

	return parameter;
}
