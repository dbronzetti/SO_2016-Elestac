#include "CPU.h"

int main(int argc, char *argv[]){
	int exitCode = EXIT_SUCCESS;
	char *configurationFile = NULL;
	char *logFile = NULL;
	t_PCB *PCB = NULL;

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

	//CPU 1 - Conectarse a la UMC y setear tamaño de paginas

	//TODO CPU 2 - Conectarse al Nucleo y quedar a la espera de 1 PCB
	//TODO CPU 2.1 - Al recibir PCB incrementar el valor del ProgramCounter
	//TODO CPU 2.2 - Uso del indice de codigo para solicitar a la UMC donde se encuentra la sentencia a ejecutar
	//TODO CPU 2.2.1 - Es posible recibir una excepcion desde la UMC por una solicitud enviada, en dicho caso se debe indicar por pantalla y CONCLUIR con el PROGRAMA
	//TODO CPU 2.3 - Parsear instruccion recibida y ejecutar operaciones
	//TODO CPU 2.4 - Actualizar valores del programa en la UMC
	//TODO CPU 2.5 - Notifica al Nucleo que concluyo un QUANTUM
	//TODO CPU 2.5.1 - Si es la ultima sentencia del programa avisar al Nucleo que finalizo el proceso para que elimine las estructuras usadas para este programa
	//TODO CPU 2.5.2 - Si fue captada la señal SIGUSR1 mientras se estaba ejecutando una instruccion se debe finalizar la misma y acto seguido desconectarse del Nucleo

	exitCode = connectTo(UMC,&socketUMC);
	if(exitCode == EXIT_SUCCESS){
		printf("CPU connected to UMC successfully\n");
	}

	exitCode = connectTo(NUCLEO,&socketNucleo);
	if(exitCode == EXIT_SUCCESS){
		printf("CPU connected to NUCLEO successfully\n");
		waitRequestFromNucleo(&socketNucleo);
	}

	//exitCode = ejecutarPrograma(PCB);

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
								configuration.pageSize = frameSize;

								printf("Tamanio de pagina: %d\n",frameSize);
								break;
							}
							case NUCLEO:{
								printf("%s\n",message->message);

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

void waitRequestFromNucleo(int *socketClient){
	//Receive message size
	int messageSize = 0;
	char *messageRcv = malloc(sizeof(messageSize));
	int receivedBytes = receiveMessage(socketClient, messageRcv, sizeof(messageSize));

	if ( receivedBytes > 0 ){
		//Receive message using the size read before
		memcpy(&messageSize, messageRcv, sizeof(int));
		messageRcv = realloc(messageRcv,messageSize);
		receivedBytes = receiveMessage(socketClient, messageRcv, messageSize);
	}

}


int ejecutarPrograma(t_PCB *PCB){
	int exitCode = EXIT_SUCCESS;

	//char *instruccion_a_ejecutar = malloc (nextInstructionPointer->offset);

	//TODO aca se debe obtener la siguente instruccion a ejecutar.
	//memcpy(instruccion_a_ejecutar, nextInstructionPointer->start , nextInstructionPointer->offset); //El warning es porque en ves de una direccion de memoria se le pasa un VALOR de direccion de memoria

	//////TODO EJECUTAR  instruccion_a_ejecutar
	//analizadorLinea(instruccion_a_ejecutar,funciones,funciones_kernel);

	//free (instruccion_a_ejecutar);

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


/*
//TODO incluir parametros del mensaje recibido de ser necesario
void ejecutarCPU(){

	 void *content = NULL;

	//Receive content size
		int messageSize = 0;//reseting size to get the content size
		int receivedBytes = 0;//reseting receivedBytes to get the content size

		receivedBytes = receiveMessage(&socketNucleo, (void*) messageSize, sizeof(messageSize));

	//Receive content using the size read before
		content = realloc(content, messageSize);
		receivedBytes = receiveMessage(&socketNucleo, content, messageSize);

	//TODO Recibir el tamaño del contexto

	//Recibo contexto
	 	t_MessageNucleo_CPU contexto;
	 	int banderaFinQuantum=0;
		char* message = (char*)malloc(sizeof(t_MessageNucleo_CPU));

		int exitCode = EXIT_FAILURE; // por default
		while (exitCode != EXIT_SUCCESS){
			//Recibo contexto de NUCLEO y deserializo
			exitCode =receiveMessage(&socketNucleo, (void*)message, sizeof(t_MessageNucleo_CPU));
			printf ("Contexto de ejecucion recibido.\n");
			deserializeCPU_Nucleo(&contexto, message);
			//pcActualizado para tener el pc actual de las instrucciones a ejecutar
		int pcActualizado=contexto.ProgramCounter;

		log_info(logCPU,
				"Contexto de ejecucion recibido - Process ID : %d - PC : %d",
				contexto.processID, contexto.ProgramCounter);

			switch(contexto.head){					// HEAD = 1 SIN desalojo HEAD = 2 CON desalojo
			case 1:// Ejecutar Funcion sin Desalojo
					ejecutarSinDesalojo(contexto.ProgramCounter,contexto.processID,pcActualizado);
					break;
			case 2: //Ejecutar Funcion Con Desalojo
					ejecutarConDesalojo(contexto.ProgramCounter ,contexto.processID,contexto.cantInstruc,pcActualizado);
					break;
			case 3: //Finalizar PID
					finalizar(contexto.processID, &banderaFinQuantum);
					break;
			default:
					printf("Contexto de Ejecucion no valido\n");
			}

		}

		free(message);

	}

void ejecutarSinDesalojo(int pc,int pid, int pcActualizado){

	pcActualizado++ ;

	// TODO procesarAnalizadorLinea
}

void ejecutarConDesalojo(int pc,int pid, int cantInstr, int pcActualizado){

	int banderaFinQuantum=0;
	pcActualizado++;

	if(pcActualizado-pc==cantInstr && banderaFinQuantum==0){
		//Envia aviso que fallo la iniciacion del proceso a NUCLEO

		t_MessageNucleo_CPU* respuestaFQuantum = malloc(sizeof(t_MessageNucleo_CPU));
		respuestaFQuantum->operacion=5;
		respuestaFQuantum->processID=pid;

		char* bufferRespuesta = malloc(sizeof(t_MessageNucleo_CPU));
		serializeNucleo_CPU(respuestaFQuantum, bufferRespuesta, sizeof(respuestaFQuantum));
		send(socketNucleo,bufferRespuesta, sizeof(t_MessageNucleo_CPU),0);

		free(bufferRespuesta);
	}

}

*/

/*

typedef struct {
	int operacion;
	int pid;
	int pagina;
	int tamanio;
} t_datosPedido;


void finalizar(int pid, int* banderaFinQuantum) {

	printf("FINALIZAR...\n");
	*banderaFinQuantum = 1;
	t_datosPedido pedido;

	pedido.operacion = 4;
	pedido.pagina = 0;
	pedido.pid = pid;
	pedido.tamanio = 0;

	char* buffer = malloc(sizeof(t_datosPedido));
	//serializarPedido(pedido, &buffer);
	//usleep(retardo);
	send(socketUMC, buffer, sizeof(t_datosPedido), 0);

	char* respuestaS = malloc(1);	// Maneja el status de los receive.
	recv(socketUMC, (void*)respuestaS, 1, 0);
	char respuesta = *respuestaS;

	printf("Respuesta: %c\n.", respuesta);
	if (respuesta == 'B') {
		log_info(logCPU, "Proceso %d - Finalizado correctamente", pid);

		//Envia aviso que finaliza correctamente el proceso a NUCLEO
		t_MessageNucleo_CPU* respuestaFinOK = malloc(
				sizeof(t_MessageNucleo_CPU));
		respuestaFinOK->operacion = 2;
		respuestaFinOK->processID = pid;

		char* bufferRespuesta = malloc(sizeof(t_MessageNucleo_CPU));
		serializeNucleo_CPU(respuestaFinOK, bufferRespuesta,
				sizeof(respuestaFinOK));
		send(socketNucleo, bufferRespuesta, sizeof(t_MessageNucleo_CPU), 0);

		free(bufferRespuesta);

	} else {
		if (respuesta == 'M') {
			log_error(logCPU, "Proceso %d - Error al finalizar", pid);

			//Envia aviso que finaliza incorrectamente el proceso a NUCLEO
			t_MessageNucleo_CPU* respuestaFinFalla = malloc( sizeof(t_MessageNucleo_CPU));
			respuestaFinFalla->operacion = 4;
			respuestaFinFalla->processID = pid;

			char* bufferRespuesta = malloc(sizeof(t_MessageNucleo_CPU));
			serializeNucleo_CPU(respuestaFinFalla, bufferRespuesta, sizeof(respuestaFinFalla));

			send(socketNucleo, bufferRespuesta, sizeof(t_MessageNucleo_CPU), 0);

			free(bufferRespuesta);

		} else {
			printf("Respuesta Invalida.\n");
		}
	}

	free(buffer);

}

char recibirRespuesta(){
	char* respuesta = malloc(1);	// Maneja el status de los receive.

	recv(socketUMC, (void*)respuesta, 1, 0);

	return *respuesta;   //B = BIEN M = MAL
}

*/

