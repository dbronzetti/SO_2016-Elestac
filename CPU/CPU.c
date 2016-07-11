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

			/* AYUDA para deserealizar indice de etiqueta
			int offset = 0;
			while ( offset < miMetaData->etiquetas_size ){
				t_registroIndiceEtiqueta *regIndiceEtiqueta = malloc(sizeof(t_registroIndiceEtiqueta));

				int j = 0;
				for (j = 0; miMetaData->etiquetas[offset + j] != '\0'; j++);

				regIndiceEtiqueta->funcion = malloc(j);
				memset(regIndiceEtiqueta->funcion,'\0', j);

				regIndiceEtiqueta->funcion = string_substring(miMetaData->etiquetas, offset, j);
				offset += j + 1;//+1 por '\0's

				log_trace(logNucleo,"funcion: %s\n", regIndiceEtiqueta->funcion);

				memcpy(&regIndiceEtiqueta->posicionDeLaEtiqueta, miMetaData->etiquetas +offset, sizeof(regIndiceEtiqueta->posicionDeLaEtiqueta));
				offset += sizeof(regIndiceEtiqueta->posicionDeLaEtiqueta);

				log_trace(logNucleo,"posicionDeLaEtiqueta: %d\n", regIndiceEtiqueta->posicionDeLaEtiqueta);

				list_add(unBloqueControl.indiceDeEtiquetas,(void*)regIndiceEtiqueta);

			}

			log_trace(logNucleo,"list 'indiceDeEtiquetas' size: %d\n", list_size(unBloqueControl.indiceDeEtiquetas));
			*/

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

						log_error(logCPU, "Corte por quantum cumplido - Proceso %d ", PCBRecibido->PID);

						t_MessageCPU_Nucleo* corteQuantum = malloc( sizeof(t_MessageCPU_Nucleo));
						corteQuantum->operacion = 5;//operacion 5 es por quantum
						corteQuantum->processID = PCBRecibido->PID;

						int payloadSize = sizeof(corteQuantum->operacion) + sizeof(corteQuantum->processID);
						int bufferSize = sizeof(bufferSize) + payloadSize ;

						char* bufferRespuesta = malloc(bufferSize);
						serializeCPU_Nucleo(corteQuantum, bufferRespuesta, payloadSize);
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
						serializeCPU_Nucleo(respuestaFinOK, bufferRespuesta, payloadSize);
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

	exitCode = receiveMessage(&socketUMC,(void*)returnCode,sizeof(exitCode));

	if( returnCode == EXIT_FAILURE){

		log_error(logCPU, "No se pudo obtener la solicitud a ejecutar - Proceso %d - Error al finalizar", PCB->PID);
		//Envia aviso que finaliza incorrectamente el proceso a NUCLEO
		t_MessageCPU_Nucleo* respuestaFinFalla = malloc( sizeof(t_MessageCPU_Nucleo));
		respuestaFinFalla->operacion = 4;
		respuestaFinFalla->processID = PCB->PID;

		int payloadSize = sizeof(respuestaFinFalla->operacion) + sizeof(respuestaFinFalla->processID);
		int bufferSize = sizeof(bufferSize) + payloadSize ;

		char* bufferRespuesta = malloc(bufferSize);
		serializeCPU_Nucleo(respuestaFinFalla, bufferRespuesta, payloadSize);
		sendMessage(&socketNucleo, bufferRespuesta, bufferSize);

		free(bufferRespuesta);

	}else{
		analizadorLinea(codigoRecibido,funciones,funciones_kernel);
	}

	return exitCode;
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
		log_info(logCPU,"Process '%s' NOT VALID to be connected by NUCLEO.\n", getProcessString(processToConnect));
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
						log_info(logCPU, "Connected to UMC - Message: %s\n",message->message);
						printf("Receiving frame size\n");
						//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
						receivedBytes = receiveMessage(socketClient, &frameSize, sizeof(messageSize));

						printf("Tamanio de pagina: %d\n",frameSize);
						break;
					}
					case NUCLEO:{
						log_info(logCPU, "Connected to NUCLEO - Message: %s\n",message->message);
						printf("Receiving stack size (number of pages for PID stack)\n");
						//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
						receivedBytes = receiveMessage(socketClient, &stackSize, sizeof(messageSize));
						break;
					}
					default:{
						log_error(logCPU,
								"Handshake not accepted when tried to connect your '%s' with '%s'\n",
								getProcessString(processToConnect),	getProcessString(message->process));
						close(*socketClient);
						exitcode = EXIT_FAILURE;
						break;
					}
					}

					break;
				}
				default:{
					log_error(logCPU,"Process couldn't connect to SERVER - Not able to connect to server %s. Please check if it's down.\n",ip);
					close(*socketClient);
					break;
				}
				}

			}else if (receivedBytes == 0 ){
				//The client is down when bytes received are 0
				log_error(logCPU,"The client went down while receiving! - Please check the client '%d' is down!\n",*socketClient);
				close(*socketClient);
			}else{
				log_error(logCPU, "Error - No able to received - Error receiving from socket '%d', with error: %d\n",*socketClient, errno);
				close(*socketClient);
			}
		}

	}else{
		log_error(logCPU, "I'm not able to connect to the server! - My socket is: '%d'\n", *socketClient);
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

void crearArchivoDeConfiguracion(char *configFile){
	t_config* configurationFile;
	configurationFile = config_create(configFile);
	configuration.ip_Nucleo = config_get_string_value(configurationFile,"IP_NUCLEO");
	configuration.port_Nucleo = config_get_int_value(configurationFile,"PUERTO_NUCLEO");
	configuration.ip_UMC = config_get_string_value(configurationFile,"IP_UMC");
	configuration.port_UMC = config_get_int_value(configurationFile,"PUERTO_UMC");
}


/*********** TODO 			funciones de entrada salida a considerar (manejarES y leerParametro)				**********/
// Modificalas y usalas como quieras perro

void manejarES(char* instruccion,int PID, int pcActualizado, int banderaFinQuantum){
	printf("ENTRADA SALIDA...\n");

	//lee el parametro de Entrada-Salida (tiempo de bloqueo...)
	char* parametroIniciar = leerParametro(instruccion);
	int tiempoBloqueo = atoi(parametroIniciar);

	//Prepara estructuras para mandarle al NUCLEO
	t_MessageCPU_Nucleo entradaSalida;
	entradaSalida.operacion = 1;
	entradaSalida.processID = PID;
	banderaFinQuantum = 1;

	t_es* datosParaPlanifdeES = malloc(sizeof(t_es));
	datosParaPlanifdeES->ProgramCounter = pcActualizado;
	datosParaPlanifdeES->tiempo = tiempoBloqueo;
	//TODO cambiar forma a payload y buffersize
	char* bufferRespuesta = malloc(sizeof(t_MessageCPU_Nucleo));

	//TODO crear serializar CPU a NUCLEO
	//serializeCPU_Nucleo(entradaSalida, &bufferRespuesta);

	sendMessage(&socketNucleo, bufferRespuesta, sizeof(t_MessageCPU_Nucleo));

	char* bufferDatosES = malloc(sizeof(t_es));

	//TODO serializar Entrada Salida
	//serializarES(datosParaPlanifdeES, bufferDatosES);

	sendMessage(&socketNucleo, bufferDatosES, sizeof(t_es));

	log_info(logCPU, "proceso: %d en entrada-salida de tiempo: %d \n", PID,tiempoBloqueo);

	free(bufferRespuesta);
	free(bufferDatosES);
	free(datosParaPlanifdeES);

}

char* leerParametro(char* instruccion){
	//Arma array de strings de laterales del SPACE
	char** desdeSpace = string_split(instruccion, " ");

	char* parametro = string_new();
	//copia en parametro lo que hay despues del space menos ";"
	strncpy (parametro, desdeSpace[1], sizeof(desdeSpace[1])-1);
	return parametro;
}
