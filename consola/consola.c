/*
 * consola.c
 *
 */

#include "consola.h"

int main(int argc, char **argv) {
	char* codeScript;
	int exitCode = EXIT_SUCCESS;
	char inputTeclado[250];
	while (1) {
		printf(PROMPT);
		fgets(inputTeclado, sizeof(inputTeclado), stdin);
		char ** comando = string_split(inputTeclado, " ");
		switch (reconocerComando(comando[0])) {
		case 1:
			printf("Comando Reconocido.\n");
			codeScript = leerArchivoYGuardarEnCadena();
			fgets(inputTeclado, sizeof(inputTeclado), stdin);
			char ** comando = string_split(inputTeclado, " ");
			break;

		case 2:
			exit(-1);
		default:
			printf("Comando invalido, inténtelo nuevamente.\n");
		}
	}
	exitCode = conectarAlNucleo(codeScript);
	return exitCode;
}

int reconocerComando(char * comando) {
	if (!strcmp("ejecutar\n", comando)) {
		return 1;
	}
	if (!strcmp("salir\n", comando)) {
		return 2;
	}
	return -1;
}

char* leerArchivoYGuardarEnCadena() {
	char textoDeArchivo[300];
	FILE* archivo = NULL;
	printf("Ingrese archivo a ejecutar.\n");
	char nombreDelArchivo[300];
	scanf("%s", nombreDelArchivo);
	archivo = fopen(nombreDelArchivo, "r");
	if (archivo == NULL) {
		printf("Error al abrir el archivo.\n");
	} else {
		while (!feof(archivo)) {
			fgets(textoDeArchivo, 300, archivo);
			printf("%s\n", textoDeArchivo);
		}
	}
	fclose(archivo);
	return textoDeArchivo;
}
int conectarAlNucleo(char* codeScript){
	int socketClient;

	char ip[15] = "127.0.0.0";
	int exitcode = openClientConnection(ip, 4040, &socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == EXIT_SUCCESS){

		// ***1) Send handshake
		exitcode = sendClientHandShake(&socketClient,NUCLEO);

		if (exitcode == EXIT_SUCCESS){

			// ***2)Receive handshake response
			//Receive message size
			int messageSize = 0;
			char *messageRcv = malloc(sizeof(messageSize));
			int receivedBytes = receiveMessage(&socketClient, messageRcv, sizeof(messageSize));

			//Receive message using the size read before
			memcpy(&messageSize, messageRcv, sizeof(int));
			printf("messageRcv received: %s\n",messageRcv);
			printf("messageSize received: %d\n",messageSize);
			messageRcv = realloc(messageRcv,messageSize);
			receivedBytes = receiveMessage(&socketClient, messageRcv, messageSize);

			printf("bytes received: %d\n",receivedBytes);

			//starting handshake with client connected
			t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
			deserializeHandShake(message, messageRcv);

			free(messageRcv);

			switch (message->process){
				case ACCEPTED:{
					printf("%s\n",message->message);
					printf("Receiving codeScript\n");
					//After receiving ACCEPTATION has to be received the "codeScript" information
					messageRcv = malloc(sizeof(codeScript));
					receivedBytes = receiveMessage(&socketClient, messageRcv, sizeof(messageSize));
					printf("bytes received: %d\n",receivedBytes);
					memcpy(codeScript, messageRcv, sizeof((void*)codeScript));
					printf("Codigo de programa: %s\n",codeScript);

					free(messageRcv);

					break;
				}
				default:{
					perror("Process couldn't connect to SERVER");//TODO => Agregar logs con librerias
					printf("Not able to connect to server %s. Please check if it's down.\n",ip);
					break;
				}
			}
		}

		while(1){
			//aca tiene que ir una validacion para ver si el server sigue arriba
			//send(socketClient, msg, sizeof(msg),0);
		}

	}else{
		perror("no me pude conectar al server!"); //
		printf("mi socket es: %d\n", socketClient);
	}


	return exitcode;
}

void crearArchivoDeConfiguracion(){
	t_config* configuration;
	configuration = config_create("/home/utnso/git/tp-2016-1c-YoNoFui/nucleo/configuracion.consola");
	configConsola.puerto_nucleo = config_get_int_value(configuration,"PUERTO_NUCLEO");
//	configConsola.ip_nucleo = config_get_array_value(configuration,"IP_NUCLEO"); 	TODO
}

