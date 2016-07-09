#include "CPU.h"

int main(int argc, char *argv[]){
	int exitCode = EXIT_SUCCESS;
	char *configurationFile = NULL;
	char* messageRcv = NULL;
	char *logFile = NULL;
	t_PCB *PCBRecibido = NULL;

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS TODO => Agregar logs con librerias

	//get parameter
	int i;
	for( i = 0; i < argc; i++){
		if (strcmp(argv[i], "-c") == 0){
			configurationFile = argv[i+1];
			printf("Configuration File: '%s'\n",configurationFile);
		}
		//check log file parameter
		if (strcmp(argv[i], "-l") == 0){
			logFile = argv[i+1];
			printf("Log File: '%s'\n",logFile);
		}
	}

	//ERROR if not configuration parameter was passed
	assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS TODO => Agregar logs con librerias

	//ERROR if not log parameter was passed
	assert(("ERROR - NOT log file was passed as argument", logFile != NULL));//Verifies if was passed the log file as parameter, if DONT FAILS

	//get configuration from file
	crearArchivoDeConfiguracion(configurationFile);

	//Creo Archivo de Log
	logCPU = log_create(logFile,"CPU",0,LOG_LEVEL_TRACE);

	//TODO CPU 2.5.2 - Si fue captada la señal SIGUSR1 mientras se estaba ejecutando una instruccion se debe finalizar la misma y acto seguido desconectarse del Nucleo

	exitCode = connectTo(UMC,&socketUMC);
	if(exitCode == EXIT_SUCCESS){
		printf("CPU connected to UMC successfully\n");
	}

	exitCode = connectTo(NUCLEO,&socketNucleo);

	while (exitCode == EXIT_SUCCESS){

		if(exitCode == EXIT_SUCCESS){
			printf("CPU connected to NUCLEO successfully\n");
			waitRequestFromNucleo(&socketNucleo, messageRcv);
		}

		//exitCode=receiveMessage(&socketNucleo,PCBrecibido,sizeof(t_PCB));

		if(messageRcv !=  NULL){
			printf("Construyendo PCB\n");
			//TODO deserealizar y construir PCB

			printf("El PCB fue recibido correctamente\n");
			//contadorDeProgramas=+1;

			//deseializarPCB(pcbRecibido,PCBrecibido);

			int j;
			//TODO CAMBIAR 'QUANTUM' y  'QUANTUM_SLEEP' por lo que se recibio
			for(j=0;j < QUANTUM;j++){

				exitCode = ejecutarPrograma(PCBRecibido);

				if (exitCode == EXIT_SUCCESS){

					sleep(QUANTUM_SLEEP);

					if(j == QUANTUM){

						log_error(logCPU, "Corte por quamtum cumplido - Proceso %d - Error al finalizar", PCBRecibido->PID);
						//Envia aviso que finaliza incorrectamente el proceso a NUCLEO
						t_MessageCPU_Nucleo* respuestaFinFalla = malloc( sizeof(t_MessageCPU_Nucleo));
						respuestaFinFalla->operacion = 5;// TODO ver como recibe el NUCLEO corte quatum
						respuestaFinFalla->processID = PCBRecibido->PID;

						int payloadSize = sizeof(respuestaFinFalla->operacion) + sizeof(respuestaFinFalla->processID);
						int bufferSize = sizeof(bufferSize) + payloadSize ;

						char* bufferRespuesta = malloc(bufferSize);
						//serializeNucleo_CPU(respuestaFinFalla, bufferRespuesta, sizeof(respuestaFinFalla)); TODO crear nuevo serializaror CPU_NUCLEO para este tipo de mensaje
						sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

						free(bufferRespuesta);

					}

					if(j == PCBRecibido->indiceDeCodigo->elements_count){

						log_info(logCPU, "Proceso %d - Finalizado correctamente", PCBRecibido->PID);
						//Envia aviso que finaliza correctamente el proceso a NUCLEO
						t_MessageCPU_Nucleo* respuestaFinOK = malloc(sizeof(t_MessageCPU_Nucleo));
						respuestaFinOK->operacion = 2;
						respuestaFinOK->processID = PCBRecibido->PID;

						int payloadSize = sizeof(respuestaFinOK->operacion) + sizeof(respuestaFinOK->processID);
						int bufferSize = sizeof(bufferSize) + payloadSize ;

						char* bufferRespuesta = malloc(bufferSize);
						//serializeNucleo_CPU(respuestaFinFalla, bufferRespuesta, sizeof(respuestaFinFalla)); TODO crear nuevo serializaror CPU_NUCLEO para este tipo de mensaje
						sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

						free(bufferRespuesta);

						j = QUANTUM;
					}

				}

			}

		}

	}

	return EXIT_SUCCESS;
}

int connectTo(enum_processes processToConnect, int *socketClient){
	int exitcode = EXIT_FAILURE;//DEFAULT VALUE
	int port = 0;
	char *ip = string_new();

	switch (processToConnect){
	case UMC:{
		string_append(&ip,configuration.ip_UMC);
		port= configuration.port_UMC;
		break;
	}
	case NUCLEO:{
		string_append(&ip,configuration.ip_Nucleo);
		port= configuration.port_Nucleo;
		break;
	}
	default:{
		perror("Process not identified!");//TODO => Agregar logs con librerias
		printf("Process '%s' NOT VALID to be connected by CPU.\n",getProcessString(processToConnect));
		break;
	}
	}

	exitcode = openClientConnection(ip, port, socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == EXIT_SUCCESS){

		// ***1) Send handshake
		exitcode = sendClientHandShake(socketClient,CPU);

		if (exitcode == EXIT_SUCCESS){

			// ***2)Receive handshake response
			//Receive message size
			int messageSize = 0;
			char *messageRcv = malloc(sizeof(messageSize));
			int receivedBytes = receiveMessage(socketClient, messageRcv, sizeof(messageSize));

			if ( receivedBytes > 0 ){
				//Receive message using the size read before
				memcpy(&messageSize, messageRcv, sizeof(int));
				messageRcv = realloc(messageRcv,messageSize);
				receivedBytes = receiveMessage(socketClient, messageRcv, messageSize);

				//starting handshake with client connected
				t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
				deserializeHandShake(message, messageRcv);

				free(messageRcv);

				switch (message->process){
				case ACCEPTED:{
					switch(processToConnect){
					case UMC:{
						printf("%s\n",message->message);
						printf("Receiving frame size\n");
						//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
						receivedBytes = receiveMessage(socketClient, &frameSize, sizeof(messageSize));

						printf("Tamanio de pagina: %d\n",frameSize);
						break;
					}
					case NUCLEO:{
						printf("%s\n",message->message);
						printf("Receiving stack size (number of pages for PID stack)\n");
						//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
						receivedBytes = receiveMessage(socketClient, &stackSize, sizeof(messageSize));
						printf("Conectado a NUCLEO\n");
						break;
					}
					default:{
						perror("Handshake not accepted");//TODO => Agregar logs con librerias
						printf("Handshake not accepted when tried to connect your '%s' with '%s'\n",getProcessString(processToConnect),getProcessString(message->process));
						close(*socketClient);
						exitcode = EXIT_FAILURE;
						break;
					}
					}

					break;
				}
				default:{
					perror("Process couldn't connect to SERVER");//TODO => Agregar logs con librerias
					printf("Not able to connect to server %s. Please check if it's down.\n",ip);
					close(*socketClient);
					break;
				}
				}

			}else if (receivedBytes == 0 ){
				//The client is down when bytes received are 0
				perror("One of the clients is down!"); //TODO => Agregar logs con librerias
				printf("Please check the client: %d is down!\n", *socketClient);
				close(*socketClient);
			}else{
				perror("Error - No able to received");//TODO => Agregar logs con librerias
				printf("Error receiving from socket '%d', with error: %d\n",*socketClient,errno);
				close(*socketClient);
			}
		}

	}else{
		perror("no me pude conectar al server!"); //
		printf("mi socket es: %d\n", *socketClient);
		close(*socketClient);
	}

	return exitcode;
}

void sendRequestToUMC(){

}

void waitRequestFromNucleo(int *socketClient, char * messageRcv){
	//Receive message size
	int messageSize = 0;
	messageRcv = malloc(sizeof(messageSize));
	int receivedBytes = receiveMessage(socketClient, messageRcv, sizeof(messageSize));

	if ( receivedBytes > 0 ){
		//Receive message using the size read before
		memcpy(&messageSize, messageRcv, sizeof(int));
		messageRcv = realloc(messageRcv,messageSize);
		receivedBytes = receiveMessage(socketClient, messageRcv, messageSize);
	}else{
		messageRcv = NULL;
	}

}

int ejecutarPrograma(t_PCB* PCB){
	int exitCode = EXIT_SUCCESS;
	t_registroIndiceCodigo* instruccionActual;
	t_memoryLocation* posicionEnMemoria = malloc(sizeof(t_memoryLocation));
	char* direccionAEnviar = malloc(sizeof(t_memoryLocation));

	instruccionActual = (t_registroIndiceCodigo*) list_get(PCB->indiceDeCodigo,PCB->ProgramCounter);
	char* codigoRecibido = malloc(instruccionActual->longitudInstruccionEnBytes);

	log_info(logCPU,"Contexto de ejecucion recibido - Process ID : %d - PC : %d", PCB->PID, PCB->ProgramCounter);

	posicionEnMemoria->pag=instruccionActual->inicioDeInstruccion/frameSize;
	posicionEnMemoria->offset=instruccionActual->inicioDeInstruccion%frameSize;
	posicionEnMemoria->size=instruccionActual->longitudInstruccionEnBytes;

	//serializarPosicionDeMemoria(direccionAEnviar,posicionEnMemoria,tamanio);
	sendMessage(&socketUMC,direccionAEnviar,sizeof(t_memoryLocation));

	int returnCode = EXIT_SUCCESS;//DEFAULT

	exitCode = receiveMessage(&socketUMC,returnCode,sizeof(exitCode));

	if( returnCode == EXIT_FAILURE){

		log_error(logCPU, "No se pudo obtener la solicitud a ejecutar - Proceso %d - Error al finalizar", PCB->PID);
		//Envia aviso que finaliza incorrectamente el proceso a NUCLEO
		t_MessageCPU_Nucleo* respuestaFinFalla = malloc( sizeof(t_MessageCPU_Nucleo));
		respuestaFinFalla->operacion = 4;
		respuestaFinFalla->processID = PCB->PID;

		int payloadSize = sizeof(respuestaFinFalla->operacion) + sizeof(respuestaFinFalla->processID);
		int bufferSize = sizeof(bufferSize) + payloadSize ;

		char* bufferRespuesta = malloc(bufferSize);
		//serializeNucleo_CPU(respuestaFinFalla, bufferRespuesta, sizeof(respuestaFinFalla)); TODO crear nuevo serializaror CPU_NUCLEO para este tipo de mensaje
		sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

		free(bufferRespuesta);

	}else{
		analizadorLinea(codigoRecibido,funciones,funciones_kernel);
	}

	return exitCode;
}

void crearArchivoDeConfiguracion(char *configFile){
	t_config* configurationFile;
	configurationFile = config_create(configFile);
	configuration.ip_Nucleo = config_get_string_value(configurationFile,"IP_NUCLEO");
	configuration.port_Nucleo = config_get_int_value(configurationFile,"PUERTO_NUCLEO");
	configuration.ip_UMC = config_get_string_value(configurationFile,"IP_UMC");
	configuration.port_UMC = config_get_int_value(configurationFile,"PUERTO_UMC");
}
