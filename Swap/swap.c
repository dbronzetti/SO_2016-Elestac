#include "swap.h"

char *pathFileSwap = NULL;

int main(int argc, char *argv[]){
	char *logFile = NULL;
	char *configurationFile = NULL;

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS

	//get parameter
	int i;
	for( i = 0; i < argc; i++){
		//check config file parameter
		if (strcmp(argv[i], "-c") == 0){
			configurationFile = argv[i+1];
			printf("Configuration File: '%s'\n",configurationFile);
		}
		//check log file parameter
		if (strcmp(argv[i], "-l") == 0){
			logFile = argv[i+1];
			printf("Log File: '%s'\n",logFile);
		}
		//check Swap file parameter
		if (strcmp(argv[i], "-a") == 0){
			pathFileSwap = argv[i+1];
			printf("Swap file: '%s'\n",pathFileSwap);
		}

	}

	//ERROR if not configuration parameter was passed
		assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS

	//ERROR if not Log parameter was passed
		assert(("ERROR - NOT log file was passed as argument", logFile != NULL));//Verifies if was passed the Log file as parameter, if DONT FAILS

	//ERROR if not Log parameter was passed
		assert(("ERROR - NOT swap file was passed as argument", pathFileSwap != NULL));//Verifies if was passed the swap file as parameter, if DONT FAILS

	logSwap = log_create(logFile, "SWAP", 0, LOG_LEVEL_TRACE);
	crearArchivoDeConfiguracion(configurationFile);

	nombreSwapFull = string_new();
	string_append(&nombreSwapFull,pathFileSwap);
	string_append(&nombreSwapFull,nombre_swap);

	crearArchivoDeSwap();
	//TODO sacar lo que esta aca abajo para que no tire error en las pruebas
	/*FILE* archivoSwap;
	archivoSwap=fopen(nombre_swap,"r+");
	fseek(archivoSwap,0,SEEK_SET);
	fwrite("1",tamanioDePagina,cantidadDePaginas,archivoSwap);*/
	char* paginaAEnviar;
	bloqueSwap* bloqueInicial=malloc(sizeof(bloqueSwap));
	bloqueInicial->PID=0;
	bloqueInicial->ocupado=0;
	bloqueInicial->tamanioDelBloque=cantidadDePaginas*tamanioDePagina;
	bloqueInicial->cantDePaginas = cantidadDePaginas;
	bloqueInicial->paginaInicial = 0;
	listaSwap=list_create();
	list_add(listaSwap,(void*)bloqueInicial);
	startServer();
	return 1;
}

void processingMessages(int socketClient){
	char* paginaRecibida;
	bloqueSwap* pedidoRecibidoYDeserializado = malloc(sizeof(bloqueSwap)); //FIXING missing memory location;

	//FIXING receiving messages - IT WAS ALL WRONG
	//Receive message size
	int messageSize = 0;

	//Get Payload size
	int receivedBytes = receiveMessage(&socketClient, &messageSize, sizeof(messageSize));

	if ( receivedBytes > 0 ){

		//Receive process from which the message is going to be interpreted
		enum_processes fromProcess;
		receivedBytes = receiveMessage(&socketClient, &fromProcess, sizeof(fromProcess));
		log_info(logSwap,"Bytes received from process '%s': %d\n",getProcessString(fromProcess),receivedBytes);

		//Receive message using the size read before
		char *messageRcv = malloc(messageSize);
		int receivedBytes = receiveMessage(&socketClient, messageRcv, messageSize);
		log_info(logSwap, "message received: %s \n", messageRcv);

		//int receivedBytes = receiveMessage(&socketClient,structUmcSwap,sizeof(operacionARealizar));
		t_MessageUMC_Swap* operacionARealizar = malloc(sizeof(t_MessageUMC_Swap)); //FIXING missing memory location
		operacionARealizar->virtualAddress = malloc(sizeof(t_memoryLocation)); //FIXING missing memory location
		deserializeSwap_UMC(operacionARealizar,messageRcv);

		switch(operacionARealizar->operation){
		case agregar_proceso:{
			pedidoRecibidoYDeserializado->PID=operacionARealizar->PID;
			pedidoRecibidoYDeserializado->cantDePaginas=operacionARealizar->cantPages;
			pedidoRecibidoYDeserializado->paginaInicial = -1;
			pedidoRecibidoYDeserializado->tamanioDelBloque = operacionARealizar->cantPages * tamanioDePagina;
			pedidoRecibidoYDeserializado->ocupado = 1; //BLOQUE OCUPADO
			int valorDeError;
			int tamanio = 0;

			if(verificarEspacioDisponible() >= pedidoRecibidoYDeserializado->cantDePaginas){

				//"Recibo el tamaño de codigo del nuevo proceso"
				receiveMessage(&socketClient,&tamanio,sizeof(tamanio));
				log_info(logSwap, "message size received: %d \n", tamanio);

				//"Recibo el codigo"
				char* codeScript = malloc(tamanio); //FIXING missing memory location
				receiveMessage(&socketClient,codeScript,tamanio);
				log_info(logSwap, "message received: %s \n", codeScript);

				if(existeElBloqueNecesitado(pedidoRecibidoYDeserializado) != NULL){
					//MOVING BLOCK BECAUSE IN ELSE codeScript was not being received
					agregarProceso(pedidoRecibidoYDeserializado,codeScript);
				}else{
					compactarArchivo();
					agregarProceso(pedidoRecibidoYDeserializado,codeScript);
				}

				free(codeScript);//adding free memory
			}else{
				log_error(logSwap,"No hay espacio disponible para agregar el bloque. \n");
				//TODO aca se debe enviar un error a la UMC
			}

			break;
		}
		case finalizar_proceso:{

			//DELETING unnecessary receive

			pedidoRecibidoYDeserializado->PID=operacionARealizar->PID;
			eliminarProceso(pedidoRecibidoYDeserializado->PID);
			break;
		}

		case lectura_pagina:{

			leer_pagina lecturaNueva;
			char* paginaLeida;
			//DELETING unnecessary receive

			//deserializarBloqueSwap(lecturaNueva,mensajeRecibido);
			pedidoRecibidoYDeserializado->PID=operacionARealizar->PID;
			pedidoRecibidoYDeserializado->paginaInicial=operacionARealizar->virtualAddress->pag;
			paginaLeida=leerPagina(pedidoRecibidoYDeserializado);

			int valorDeError = sendMessage(&socketClient,paginaLeida,tamanioDePagina);
			if(valorDeError == EXIT_SUCCESS){// cambiando al valor apropiado la funcion nunca retorna -1
				log_info(logSwap,"Se enviaron correctamente los datos. \n");
			}else{
				log_error(logSwap,"No se enviaron correctamente los datos. \n");
				/* TODO esto esta mal, si no pudo hacer el send a quien va a volver a hacer otro send con un mensaje de error????
				sendMessage(&socketClient,mensajeDeError,string_length(mensajeDeError));
				*/
			}
			free(paginaLeida);//Adding free because it was never being freed the memory requested in leerPagina()
			break;
		}
		case escritura_pagina:{

			char* paginaAEscribir=malloc(tamanioDePagina);
			pedidoRecibidoYDeserializado->PID=operacionARealizar->PID;
			pedidoRecibidoYDeserializado->paginaInicial=operacionARealizar->virtualAddress->pag;
			receiveMessage(&socketClient,paginaAEscribir,operacionARealizar->virtualAddress->size);//cambio para asegurar que el receive siempre se haga bien
			escribirPagina(paginaAEscribir,pedidoRecibidoYDeserializado);

			free(paginaAEscribir);//Adding free because it was never being freed the memory requested
			break;
		}
		default: log_warning(logSwap,"La operacion recibida es invalida. \n");
		}
	}else{
		log_error(logSwap,"No se recibio correctamente los datos. \n");
	}
}

void destructorBloqueSwap(bloqueSwap* self){
	free(self);
}


int agregarProceso(bloqueSwap* unBloque,char* codeScript){
	/*<----------------------------------------------abroElArchivo----------------------------------------------------->*/
	FILE* archivoSwap;
	archivoSwap=fopen(nombreSwapFull,"r+");

	if(archivoSwap==NULL){
		log_error(logSwap,"No se abrio correctamente el archivo. \n");
	}else{
		bloqueSwap* elementoEncontrado;
		bloqueSwap* nuevoBloqueVacio = malloc(sizeof(bloqueSwap)); //FIXING missing memory location

		elementoEncontrado = buscarBloqueALlenar(unBloque);
		elementoEncontrado->tamanioDelBloque = elementoEncontrado->tamanioDelBloque - unBloque->tamanioDelBloque;
		unBloque->paginaInicial = elementoEncontrado->paginaInicial;
		nuevoBloqueVacio->PID = 0;//BY DEFAULT
		nuevoBloqueVacio->ocupado = 0;
		nuevoBloqueVacio->cantDePaginas = elementoEncontrado->cantDePaginas - unBloque->cantDePaginas;
		nuevoBloqueVacio->tamanioDelBloque = elementoEncontrado->tamanioDelBloque - unBloque->tamanioDelBloque;
		nuevoBloqueVacio->paginaInicial = elementoEncontrado->paginaInicial + unBloque->cantDePaginas;
		fseek(archivoSwap, elementoEncontrado->paginaInicial*tamanioDePagina, SEEK_SET);
		fwrite(codeScript,strlen(codeScript),1,archivoSwap);

		bool posibleBloqueAEliminar(bloqueSwap* unBloqueAEliminar){
			return unBloqueAEliminar->paginaInicial == unBloque->paginaInicial;
		}

		list_remove_and_destroy_by_condition(listaSwap,(void*)posibleBloqueAEliminar,(void*)destructorBloqueSwap);
		list_add(listaSwap,(void*)unBloque);
		list_add(listaSwap,(void*)nuevoBloqueVacio);

		bool criterioDeOrden(bloqueSwap* unBloque,bloqueSwap* otroBloque){
			return(unBloque->paginaInicial < otroBloque->paginaInicial);
		}

		list_sort(listaSwap,(void*)criterioDeOrden);
		fclose(archivoSwap);
	}
	return 0;
}



int compactarArchivo(){
	int i,acum;
	acum=0;
	int j;
	bloqueSwap* bloqueLleno;//=malloc(sizeof(bloqueSwap));
	bloqueSwap* bloqueLlenoSiguiente;//=malloc(sizeof(bloqueSwap));
	bloqueSwap* unicoBloqueLleno=malloc(sizeof(bloqueSwap));
	bloqueSwap* bloqueVacioCompacto=malloc(sizeof(bloqueSwap));
	int bloqueVacioAEliminar(bloqueSwap* unBloque){
		return (unBloque->ocupado==0);
	};
	for(i=0;listaSwap->elements_count>=i;i++){
		list_remove_and_destroy_by_condition(listaSwap,(void*)bloqueVacioAEliminar,(void*)destructorBloqueSwap);
	}
	if(listaSwap->elements_count==1){
		unicoBloqueLleno=(bloqueSwap*)list_get(listaSwap,0);
		modificarArchivo(unicoBloqueLleno->paginaInicial,unicoBloqueLleno->cantDePaginas,0);
		unicoBloqueLleno->paginaInicial=0;
		acum=bloqueLleno->cantDePaginas;

	}
	if(listaSwap->elements_count>1){
		for(i=0;i<listaSwap->elements_count;i++){
			printf("%i\n",i);
			bloqueSwap* bloqueObtenido=malloc(sizeof(bloqueSwap));
			bloqueObtenido=(bloqueSwap*)list_get(listaSwap,i);
			acum=acum+bloqueObtenido->cantDePaginas;
			printf("%i   %i   %i   %i\n",bloqueObtenido->PID,bloqueObtenido->cantDePaginas,bloqueObtenido->ocupado,bloqueObtenido->paginaInicial);
		}
		for(j=1;listaSwap->elements_count>j;j++){
			int x=0;
			bloqueLleno=(bloqueSwap*)list_get(listaSwap,x);
			printf("cantPag:%i",bloqueLleno->cantDePaginas);
			bloqueLlenoSiguiente=(bloqueSwap*)list_get(listaSwap,x+1);
			bloqueLlenoSiguiente->paginaInicial=bloqueLleno->cantDePaginas+1;
			modificarArchivo(bloqueLleno->paginaInicial,bloqueLleno->cantDePaginas,bloqueLlenoSiguiente->paginaInicial);
			x++;
		}}
	if(cantidadDePaginas-acum!=0){
		bloqueVacioCompacto->cantDePaginas=cantidadDePaginas-acum;
		bloqueVacioCompacto->ocupado=0;
		bloqueVacioCompacto->PID=0;
		bloqueVacioCompacto->paginaInicial=acum;
		list_add(listaSwap,bloqueVacioCompacto);
	}



	return 0;
}



void crearArchivoDeSwap(){
	int tamanioDePagina;
	tamanioDePagina=256;
	int cantidadDePaginas;
	cantidadDePaginas=10;

	char* cadena=string_new();
	string_append(&cadena,"dd if=/dev/zero of=");
	string_append(&cadena, nombreSwapFull);
	char* segundaParteCadena=string_new();
	string_append(&segundaParteCadena," bs=");
	string_append(&cadena,segundaParteCadena);
	string_append_with_format(&cadena,"%i",tamanioDePagina);
	char* terceraParteCadena=string_new();
	string_append(&terceraParteCadena," count=");
	string_append(&cadena,terceraParteCadena);
	string_append_with_format(&cadena,"%i",cantidadDePaginas);
	system(cadena);
}

bool condicionLeer(bloqueSwap* unBloque,bloqueSwap* otroBloque){
	return (unBloque->PID==otroBloque->PID);
}

bloqueSwap* buscarProcesoAEliminar(int PID){
	bloqueSwap* bloqueObtenido = NULL;
	int i = 0;
	while( i < listaSwap->elements_count ){

		bloqueObtenido = list_get(listaSwap,i);
		if(bloqueObtenido->PID==PID){
			break;
		}
		i++;
	}

	if (bloqueObtenido == NULL){
		log_error(logSwap,"No se encontro un bloque para eliminar. \n");
	}

	return bloqueObtenido;

}


char* leerPagina(bloqueSwap* bloqueDeSwap){
	bloqueSwap* bloqueEncontrado;
	FILE* archivoSwap;
	archivoSwap=fopen(nombreSwapFull,"r+");
	if(archivoSwap==NULL){
		log_error(logSwap,"No se abrio correctamente el archivo. \n");
	}
	char* paginaAEnviar=malloc(sizeof(tamanioDePagina));
	bloqueEncontrado=buscarProcesoAEliminar(bloqueDeSwap->PID);
	fseek(archivoSwap,bloqueEncontrado->paginaInicial*tamanioDePagina+bloqueDeSwap->paginaInicial*tamanioDePagina,SEEK_SET);
	fread(paginaAEnviar,tamanioDePagina,1,archivoSwap);
	fclose(archivoSwap);
	return paginaAEnviar;
}

void escribirPagina(char* paginaAEscribir,bloqueSwap* unBloque){
	bloqueSwap* bloqueEncontrado;
	FILE* archivoSwap;
	archivoSwap=fopen(nombreSwapFull,"r+");
	if(archivoSwap==NULL){
		log_error(logSwap,"No se abrio correctamente el archivo. \n");
	}
	bloqueEncontrado=buscarProcesoAEliminar(unBloque->PID);
	fseek(archivoSwap,bloqueEncontrado->paginaInicial*tamanioDePagina+unBloque->paginaInicial*tamanioDePagina,SEEK_SET);
	fwrite(paginaAEscribir,tamanioDePagina,1,archivoSwap);
	fclose(archivoSwap);

}


void* mapearArchivoEnMemoria(int offset,int tamanio){
	FILE* archivoSwap;
	int descriptorSwap;
	void* archivoMapeado;
	archivoSwap=fopen(nombre_swap,"a+");
	descriptorSwap=fileno(archivoSwap);
	fflush(stdin);
	fflush(stdout);
	write(descriptorSwap,"/0",tamanioDePagina*cantidadDePaginas);
	lseek(descriptorSwap,0,SEEK_SET);
	fsync(descriptorSwap);
	archivoMapeado=mmap(0,tamanio,PROT_WRITE,MAP_SHARED,descriptorSwap,offset);
	return archivoMapeado;

}

bool elementosVacios(bloqueSwap* unElemento){ //changing to bool
	return (unElemento->ocupado == 0);
}
int verificarEspacioDisponible(){
	t_list* listaFiltrada;
	int i;
	int acum = 0;
	bloqueSwap* bloqueDevuelto;
	listaFiltrada = list_filter(listaSwap,(void*)elementosVacios);
	for(i=0; i < listaFiltrada->elements_count; i++){
		bloqueDevuelto = list_get(listaFiltrada,i);
		acum += (bloqueDevuelto->cantDePaginas);
	}

	return acum;
}



bloqueSwap* existeElBloqueNecesitado(bloqueSwap* otroBloque){
	bool condicionDeCompactacion(bloqueSwap* unBloque){// esta funcion debe retornar bool
		return( unBloque->cantDePaginas >= otroBloque->cantDePaginas);
	}
	return (bloqueSwap*)list_find(listaSwap,(void*)condicionDeCompactacion);//TODO Sigue tirando segmentation fault aca
}

void crearArchivoDeConfiguracion(char* configFile){
	t_config* configuracionDeSwap;
	configuracionDeSwap=config_create(configFile);
	puertoEscucha=config_get_int_value(configuracionDeSwap,"PUERTO_ESCUCHA");
	nombre_swap=config_get_string_value(configuracionDeSwap,"NOMBRE_SWAP");
	cantidadDePaginas=config_get_int_value(configuracionDeSwap,"CANTIDAD_PAGINAS");
	tamanioDePagina=config_get_int_value(configuracionDeSwap,"TAMANIO_PAGINA");
	retardoAcceso=config_get_int_value(configuracionDeSwap,"RETARDO_ACCESO");
	retardoCompactacion=config_get_int_value(configuracionDeSwap,"RETARDO_COMPACTACION");

}

void startServer(){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	t_serverData serverData;

	exitCode = openServerConnection(puertoEscucha, &serverData.socketServer);
	log_info(logSwap,"socketServer: %d\n",serverData.socketServer);

	//If exitCode == 0 the server connection is opened and listening
	if (exitCode == 0) {
		log_info(logSwap, "The server is opened.\n");

		exitCode = listen(serverData.socketServer, SOMAXCONN);

		if (exitCode < 0 ){
			log_error(logSwap,"Failed to listen server Port.\n");
			return;
		}

		newClients((void*) &serverData);
	}

}

void newClients (void *parameter){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	int pid;

	t_serverData *serverData = (t_serverData*) parameter;

	exitCode = acceptClientConnection(&serverData->socketServer, &serverData->socketClient);

	if (exitCode == EXIT_FAILURE){
		log_warning(logSwap,"There was detected an attempt of wrong connection\n");
		close(serverData->socketClient);
	}else{
		log_info(logSwap,"The was received a connection in socket: %d.",serverData->socketClient);
		handShake(parameter);

	}// END handshakes

}


void handShake (void *parameter){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure

	t_serverData *serverData = (t_serverData*) parameter;

	//Receive message size
	int messageSize = 0;
	char *messageRcv = malloc(sizeof(messageSize));
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(messageSize));

	//Receive message using the size read before
	memcpy(&messageSize, messageRcv, sizeof(messageSize));
	//log_info(logSwap,"messageSize received: %d\n",messageSize);
	messageRcv = realloc(messageRcv,messageSize);
	receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

	//starting handshake with client connected
	t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
	deserializeHandShake(message, messageRcv);

	//Now it's checked that the client is not down
	if ( receivedBytes == 0 ){
		log_error(logSwap,"The client went down while handshaking! - Please check the client '%d' is down!\n", serverData->socketClient);
		close(serverData->socketClient);
	}else{
		switch ((int) message->process){
		case UMC:{
			log_info(logSwap,"Message from '%s': %s\n", getProcessString(message->process), message->message);

			exitCode = sendClientAcceptation(&serverData->socketClient);

			if (exitCode == EXIT_SUCCESS){

				//start receiving requests
				while(1){
					processingMessages(serverData->socketClient);//FIXING socketsSwap wrong variable
					//processingMessages(socketsSwap->socketClient); //socketsSwap was never loaded
				}

			}

			break;
		}
		default:{
			log_error(logSwap,"Process not allowed to connect - Invalid process '%s' tried to connect to UMC\n", getProcessString(message->process));
			close(serverData->socketClient);
			break;
		}
		}
	}

	free(messageRcv);
	free(message->message);
	free(message);
}

bloqueSwap* buscarBloqueALlenar(bloqueSwap* unBloque){
	bloqueSwap* bloqueObtenido = NULL;
	int i = 0;
	while( i < listaSwap->elements_count ){
		bloqueObtenido = list_get(listaSwap,i);
		if((bloqueObtenido->cantDePaginas >= unBloque->cantDePaginas) && bloqueObtenido->ocupado==0){
			break;
		}
		i++;
	}

	if (bloqueObtenido == NULL){
		log_warning(logSwap,"No se encontro un bloque para llenar. \n");
	}

	return bloqueObtenido;

}

int eliminarProceso(int PID){
	bloqueSwap* procesoAEliminar=buscarProcesoAEliminar(PID);
	procesoAEliminar->PID=0;
	procesoAEliminar->ocupado=0;
	return 1;
}



int modificarArchivo(int marcoInicial,int cantDeMarcos,int nuevoMarcoInicial){
	FILE* archivoSwap;
	char* textoRelleno=malloc(tamanioDePagina*cantDeMarcos);
	archivoSwap=fopen(nombreSwapFull,"r+");
	if(archivoSwap==NULL){
		log_error(logSwap,"no se pudo abrir el archivo. \n");
	}
	int i;
	for(i=0;i<cantDeMarcos*tamanioDePagina;i++){
		textoRelleno[i]='\0';
	}
	char* lectura=malloc(tamanioDePagina*cantDeMarcos);
	fseek(archivoSwap,marcoInicial,SEEK_SET);
	fread(lectura,tamanioDePagina,cantDeMarcos,archivoSwap);
	fseek(archivoSwap,marcoInicial,SEEK_SET);
	fwrite(textoRelleno,sizeof(textoRelleno),1,archivoSwap);
	fseek(archivoSwap,nuevoMarcoInicial,SEEK_SET);
	fwrite(lectura,tamanioDePagina,cantDeMarcos,archivoSwap);
	return 0;
}
