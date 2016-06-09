#include "CPU.h"

int main(){
	int exitCode = EXIT_SUCCESS;
	t_PCB *PCB = NULL;

	//TODO CPU 1 - Conectarse a la UMC y setear tamaño de paginas

	//TODO CPU 2 - Conectarse al Nucleo y quedar a la espera de 1 PCB
	//TODO CPU 2.1 - Al recibir PCB incrementar el valor del ProgramCounter
	//TODO CPU 2.2 - Uso del indice de codigo para solicitar a la UMC donde se encuentra la sentencia a ejecutar
	//TODO CPU 2.2.1 - Es posible recibir una excepcion desde la UMC por una solicitud enviada, en dicho caso se debe indicar por pantalla y CONCLUIR con el PROGRAMA
	//TODO CPU 2.3 - Parsear instruccion recibida y ejecutar operaciones
	//TODO CPU 2.4 - Actualizar valores del programa en la UMC
	//TODO CPU 2.5 - Notifica al Nucleo que concluyo un QUANTUM
	//TODO CPU 2.5.1 - Si es la ultima sentencia del programa avisar al Nucleo que finalizo el proceso para que elimine las estructuras usadas para este programa
	//TODO CPU 2.5.2 - Si fue captada la señal SIGUSR1 mientras se estaba ejecutando una instruccion se debe finalizar la misma y acto seguido desconectarse del Nucleo

	exitCode = connectToUMC();


	//exitCode = ejecutarPrograma(PCB);

	return EXIT_SUCCESS;
}

int connectToUMC(){
	int socketClient;

	char ip[15] = "127.0.0.1";
	int exitcode = openClientConnection(&ip, 8080, &socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == EXIT_SUCCESS){

		// ***1) Send handshake
		exitcode = sendClientHandShake(&socketClient,CPU);

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
					printf("Receiving frame size\n");
					//After receiving ACCEPTATION has to be received the "Tamanio de pagina" information
					messageRcv = malloc(sizeof(int));
					receivedBytes = receiveMessage(&socketClient, messageRcv, sizeof(messageSize));
					printf("bytes received: %d\n",receivedBytes);
					int tamanio = 0;
					memcpy(&tamanio, messageRcv, sizeof(int));
					printf("Tamanio de pagina: %d\n",tamanio);

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


