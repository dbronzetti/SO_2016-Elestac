#include <sys/mman.h>

struct bloqueDeMemoria{
	int PID;
	int cantDePaginas;
	int ocupado;
	int tamanioDelBloque;
	int paginaInicial;

}typedef bloqueSwap;

int main(){
	bloqueSwap* unBloque=malloc(20);
	char* bloqueSerializado=malloc(20);
	bloqueSwap* bloqueDeserializado=malloc(20);
	unBloque->PID=2;
	unBloque->cantDePaginas=3;
	unBloque->ocupado=0;
	unBloque->paginaInicial=1;
	unBloque->tamanioDelBloque=250;
	printf("Por Serializar");
	serializarBloqueSwap(unBloque,bloqueSerializado);
	printf("Ya serialize");
	deserializarBloqueSwap(bloqueSerializado,bloqueDeserializado);
	printf("Ya Deserialize");
	printf("%i",bloqueDeserializado->PID);
	printf("%i",bloqueDeserializado->cantDePaginas);
	printf("%i",bloqueDeserializado->ocupado);
	printf("%i",bloqueDeserializado->paginaInicial);
	printf("%i",bloqueDeserializado->tamanioDelBloque);


}



int serializarBloqueSwap(bloqueSwap* unBloque,char* bloqueSerializado){

	bloqueSerializado=malloc(sizeof(bloqueSwap));
	memcpy(bloqueSerializado,unBloque->PID,4);
	memcpy(bloqueSerializado+4,unBloque->cantDePaginas,4);
	memcpy(bloqueSerializado+8,unBloque->ocupado,4);
	memcpy(bloqueSerializado+12,unBloque->paginaInicial,4);
	memcpy(bloqueSerializado+16,unBloque->tamanioDelBloque,4);
	return 0;
}

int deserializarBloqueSwap(char* unBloque,bloqueSwap* bloqueRecibido){

	memcpy(bloqueRecibido->PID,unBloque,4);
	memcpy(bloqueRecibido->cantDePaginas,unBloque+4,4);
	memcpy(bloqueRecibido->ocupado,unBloque+8,4);
	memcpy(bloqueRecibido->paginaInicial,unBloque+12,4);
	memcpy(bloqueRecibido->tamanioDelBloque,unBloque+16,4);
	return 0;
}
