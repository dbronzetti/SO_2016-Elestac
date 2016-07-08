#include "CPU.h"
#include "common-types.h"


int tamanioDePagina;
int socketCPU;
int contadorDeProgramas=0;
int QUANTUM_SLEEP;
int QUANTUM;
int main(int argc, char *argv[]){
	int exitCode = EXIT_SUCCESS;
	char *configurationFile = NULL;
	t_PCB *PCB = NULL;

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS TODO => Agregar logs con librerias

	//get parameter
	int i;
	for( i = 0; i < argc; i++){
		if (strcmp(argv[i], "-c") == 0){
			configurationFile = argv[i+1];
			printf("Configuration File: '%s'\n",configurationFile);
		}
	}

	//ERROR if not configuration parameter was passed
	assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS TODO => Agregar logs con librerias

	//get configuration from file
	crearArchivoDeConfiguracion(configurationFile);


	//TODO CPU 2.5.2 - Si fue captada la señal SIGUSR1 mientras se estaba ejecutando una instruccion se debe finalizar la misma y acto seguido desconectarse del Nucleo

	exitCode = connectTo(UMC,&socketUMC);
	if(exitCode == EXIT_SUCCESS){
		printf("CPU connected to UMC successfully\n");
	}
	char* tamanioDePaginaRecibido;
	receiveMessage(&socketUMC,tamanioDePaginaRecibido,sizeof(int));
	memcpy(&tamanioDePagina,tamanioDePaginaRecibido,sizeof(int));
	exitCode = connectTo(NUCLEO,&socketNucleo);
	if(exitCode == EXIT_SUCCESS){
		printf("CPU connected to NUCLEO successfully\n");
		waitRequestFromNucleo(&socketNucleo);
	}
	char* PCBrecibido;
	t_PCB* pcbRecibdo;
	exitCode=receiveMessage(&socketNucleo,PCBrecibido,sizeof(t_PCB));
	if(exitCode!=-1){
		printf("El PCB fue recibido correctamente");
		contadorDeProgramas=+1;
	}
	//deseializarPCB(pcbRecibido,PCBrecibido);
	int j;
	for(j=0;j<QUANTUM;j++){
		ejecutarPrograma(pcbRecibdo);
		sleep(QUANTUM_SLEEP);
		if(j==QUANTUM){
			char* mensajeDeQuantum="Concluyo El Quantum Asignado";
			sendMessage(&socketNucleo,mensajeDeQuantum,strlen(mensajeDeQuantum));
		}
		if(j==pcbRecibdo->indiceDeCodigo->elements_count){
			char* mensajeFinalizacion="Finalizo el proceso que se estaba ejecutando";
			sendMessage(&socketNucleo,mensajeFinalizacion,strlen(mensajeFinalizacion));
			j=QUANTUM;
		}

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


int ejecutarPrograma(t_PCB* PCB){
	int exitCode = EXIT_SUCCESS;
	t_registroIndiceCodigo* instruccionActual;
	t_memoryLocation* posicionEnMemoria=malloc(sizeof(t_memoryLocation));
	char* direccionAEnviar=malloc(sizeof(t_memoryLocation));
	instruccionActual=list_get(PCB->indiceDeCodigo,PCB->ProgramCounter);
	char* codigoRecibido=malloc(instruccionActual->longitudInstruccionEnBytes);

	posicionEnMemoria->pag=instruccionActual->inicioDeInstruccion/tamanioDePagina;
	posicionEnMemoria->offset=instruccionActual->longitudInstruccionEnBytes;
	posicionEnMemoria->size=instruccionActual->longitudInstruccionEnBytes;
	//serializarPosicionDeMemoria();
	sendMessage(&socketUMC,direccionAEnviar,sizeof(t_memoryLocation));
	if(receiveMessage(&socketCPU,codigoRecibido,instruccionActual->longitudInstruccionEnBytes)==-1){
		char* mensajeDeError="No se pudo obtener la solicitud a ejecutar";
		sendMessage(&socketNucleo,mensajeDeError,strlen(mensajeDeError));
		printf("No se pudo obtener la solicitud a ejecutar");
		exit(EXIT_FAILURE);
	};
	analizadorLinea(codigoRecibido,funciones,funciones_kernel);
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
