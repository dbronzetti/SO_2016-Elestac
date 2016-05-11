/*
 * nucleo.c
 *
 */

#include "nucleo.h"
#include "serializacion.h"

int main(int argc, char **argv){

	int exitCode = EXIT_SUCCESS ;
	configNucleo->PUERTO_PROG = 6060;
	configNucleo->PUERTO_CPU = 6061;
	int highestDescriptor;
	int socketCounter;
	fd_set readSocketSet;

	//Creo el archivo de Log
			logNucleo = log_create("logPlanif", "TP", 0, LOG_LEVEL_TRACE);
	//Creo la lista de CPUs
			listaCPU = list_create();
	//Creo Lista Procesos
			listaProcesos = list_create();
	//Creo la Cola de Listos
			colaListos = queue_create();
	//Creo la Cola de Bloqueados
			colaBloqueados = queue_create();
	//Creo cola de Procesos a Finalizar (por Finalizar PID).
			colaFinalizar = queue_create();

	//conexión
	exitCode = openServerConnection(configNucleo->PUERTO_CPU, &socketServer);
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
			   //check activity in all the descriptors from the set
			   for(socketCounter=0 ; socketCounter<highestDescriptor+1; socketCounter++){
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
						int receivedBytes = receiveMessage(&socketCounter, messageRcv);
						messageRcv->id= numCPU;
						messageRcv->numSocket = socketClient;
						list_add(listaCPU, (void*)messageRcv);
						numCPU++;
						//Now it's checked that the client is not down
						if ( receivedBytes <= 0 ){
							perror("One of the clients is down!"); //TODO => Agregar logs con librerias
							log_info(logNucleo, "Se conecto la CPU %d", messageRcv->id);
							printf("Please check the client: %d is down!\n", socketCounter);
							FD_CLR(socketCounter, &readSocketSet);
							close(socketCounter);
						}else{
							switch ((int)messageRcv->process){
								case CONSOLA:
									exitCode = sendClientAcceptation(&socketClient, &readSocketSet);
									highestDescriptor = (highestDescriptor < socketClient)?socketClient: highestDescriptor;
									break;
								case CPU:
									exitCode = sendClientAcceptation(&socketClient, &readSocketSet);
									highestDescriptor = (highestDescriptor < socketClient)?socketClient: highestDescriptor;
									t_respuesta respuesta;
									char *respuestaCPU = (char*)malloc(sizeof(t_respuesta));
									char *datosEntradaSalida = malloc (sizeof(t_es));
									t_es infoES;
									memset(respuestaCPU, '\0', sizeof(t_respuesta));

									recv(highestDescriptor,(void*)respuestaCPU,sizeof(t_respuesta),0);
									deserializarRespuesta(&respuesta,&respuestaCPU);
									switch(respuesta.operacion){
										case 1: //Entrada Salida
											recv(socketCounter,(void*)datosEntradaSalida, sizeof(t_es),MSG_WAITALL);
											//deserializarEntradaSalida(&infoES,&datosEntradaSalida);
											//hacerEntradaSalida(socketCounter, respuesta.pid,infoES.pc,infoES.tiempo);
											break;
										case 2: //Finaliza Proceso Bien
											//finalizaProceso(socketCounter, respuesta.pid, respuesta.operacion);
											break;
										case 3: //Finaliza Proceso Mal
											//finalizaProceso(socketCounter, respuesta.pid, respuesta.operacion);
											break;
										case 4:	//Falla otra cosa
											printf ("Algo fallo.\n");
											break;
										case 5: //Corte por Quantum
											printf ("Corto por Quamtum.\n");
											//atenderCorteQuantum(socketCounter,respuesta.pid);
											break;
										default:
											printf ("Respuesta recibida invalida, CPU desconectado.\n");
											abort();
									  }//fin del switch interno
									break;
								case UMC:
									exitCode = sendClientAcceptation(&socketClient, &readSocketSet);
									highestDescriptor = (highestDescriptor < socketClient)?socketClient: highestDescriptor;
									break;
								default:
									perror("Process not allowed to connect");//TODO => Agregar logs con librerias
									printf("Invalid process '%d' tried to connect to UMC\n",(int) messageRcv->process);
									close(socketClient);
									break;
							}//fin del switch externo
						free(messageRcv);
					}// END handshake
				}//fin del else
			   }//fin del if
			}//fin del for
		}//fin del if
	  }//fin del while
	}//fin del if
	//cleaning set
	FD_ZERO(&readSocketSet);

	return exitCode;

}

