/*
 * consola.c
 *
 */

#include "consola.h"
char* ip_Nucleo;
int puertoNucleo;
int socketNucleo=0;

int main() {
	//	crearArchivoDeConfiguracion();
	char* codeScript;
	int exitCode = EXIT_SUCCESS;
	char inputTeclado[250];
	/*printf("antes de conectarme");
	exitCode = connectTo(NUCLEO, &socketNucleo);
	if (exitCode == EXIT_SUCCESS) {
		printf("CONSOLA connected to NUCLEO successfully\n");
	}*/

	printf("despues de conectarme");
	while (1) {
		printf(PROMPT);
		fgets(inputTeclado, sizeof(inputTeclado), stdin);
		char ** comando = string_split(inputTeclado, " ");
		switch (reconocerComando(comando[0])) {
		case 1: {
			int* tamanioArchivo=0;
			printf("Comando Reconocido.\n");
			codeScript = leerArchivoYGuardarEnCadena(tamanioArchivo);
			fgets(inputTeclado, sizeof(inputTeclado), stdin);
			//exitCode = sendMessage(&socketNucleo, codeScript,sizeof(codeScript));
			printf("%i\n",*tamanioArchivo);

			break;
		}

		case 2: {
			exit(-1);
			break;
		}
		default:
			printf("Comando invalido, inténtelo nuevamente.\n");
		}
	}
	return exitCode;
}

int reconocerComando(char* comando) {
	if (!strcmp("ejecutar\n", comando)) {
		return 1;
	}
	if (!strcmp("salir\n", comando)) {
		return 2;
	}
	return -1;
}

char* leerArchivoYGuardarEnCadena(int* tamanioDeArchivo) {
	FILE* archivo=NULL;

	int descriptorArchivo;
	printf("Ingrese archivo a ejecutar.\n");
	char nombreDelArchivo[60];
	scanf("%s", nombreDelArchivo);
	archivo = fopen(nombreDelArchivo, "r");
	descriptorArchivo=fileno(archivo);
	lseek(descriptorArchivo,0,SEEK_END);
	tamanioDeArchivo=ftell(archivo);
	char* textoDeArchivo=malloc(tamanioDeArchivo);
	lseek(descriptorArchivo,0,SEEK_SET);
	if (archivo == NULL) {
		printf("Error al abrir el archivo.\n");
	} else {
		while (!feof(archivo)) {
			fgets(textoDeArchivo, tamanioDeArchivo, archivo);
			printf("%s\n", textoDeArchivo);
		}
	}
	fclose(archivo);
	return textoDeArchivo;
}

int connectTo(enum_processes processToConnect, int *socketClient){
	int exitcode = EXIT_FAILURE;//DEFAULT VALUE
	int port = 0;
	char *ip = string_new();

	switch (processToConnect){
	case NUCLEO:{
		string_append(&ip,ip_Nucleo);
		port= puertoNucleo;
		break;
	}
	default:{
		perror("Process not identified!");//TODO => Agregar logs con librerias
		printf("Process '%s' NOT VALID to connected by CONSOLA.\n",getProcessString(processToConnect));
		break;
	}
	}
	exitcode = openClientConnection(ip, port, socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == EXIT_SUCCESS){

		// ***1) Send handshake
		exitcode = sendClientHandShake(socketClient,CONSOLA);

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

				switch (message->process) {
				case ACCEPTED: {
					printf("%s\n", message->message);
					printf("Receiving frame size\n");
					//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
					receivedBytes = receiveMessage(socketClient, &frameSize,sizeof(messageSize));

					printf("Tamanio de pagina: %d\n", frameSize);
					break;
				}
				default:{
					perror("Process couldn't connect to SERVER");//TODO => Agregar logs con librerias
					printf("Not able to connect to server %s. Please check if it's down.\n",ip);
					break;
				}
				}
			}else if (receivedBytes == 0 ){
				//The client is down when bytes received are 0
				perror("One of the clients is down!"); //TODO => Agregar logs con librerias
				printf("Please check the client: %d is down!\n", *socketClient);
			}else{
				perror("Error - No able to received");//TODO => Agregar logs con librerias
				printf("Error receiving from socket '%d', with error: %d\n",*socketClient,errno);
			}
		}

	}else{
		perror("no me pude conectar al server!"); //
		printf("mi socket es: %d\n", *socketClient);
	}

	return exitcode;
}

void crearArchivoDeConfiguracion(){
	t_config* configuration;
	configuration = config_create("/home/utnso/git/tp-2016-1c-YoNoFui/consola/configuracion.consola");
	puertoNucleo = config_get_int_value(configuration,"PUERTO_NUCLEO");
	ip_Nucleo = config_get_string_value(configuration,"IP_NUCLEO");
}

int reconocerOperacion() {
	char* tamanioSerializado=malloc(sizeof(int));
	int tamanio;
	int operacion;
	char* operacionSerializada=malloc(sizeof(int));
	int exitCode = EXIT_SUCCESS;
	exitCode = receiveMessage(&socketNucleo, operacionSerializada, sizeof(int));
	memcpy(&operacion, &operacionSerializada, sizeof(int));
	switch (operacion) {
	case 1: {
		exitCode = receiveMessage(&socketNucleo, tamanioSerializado,sizeof(int));
		memcpy(&tamanio, &tamanioSerializado, sizeof(int));
		char* textoImprimir=malloc(tamanio);
		exitCode = receiveMessage(&socketNucleo, (void*) textoImprimir,sizeof(tamanio));
		printf("Texto: %s", textoImprimir);
		break;
	}
	case 2: {
		char* valorAMostrarSerializado=malloc(sizeof(int));
		int valorAMostrar;
		exitCode = receiveMessage(&socketNucleo, valorAMostrarSerializado,sizeof(int));
		memcpy(&valorAMostrar, &valorAMostrarSerializado, sizeof(int));
		printf("Valor Recibido:%i", valorAMostrar);
		break;
	}
	case 3: {
		exit(-1);
		break;
	}
	}
	return exitCode;
}
