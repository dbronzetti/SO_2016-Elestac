#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include <stdio.h>


struct indiceDeCodigo{
	int inicioDeInstruccion;
	int desplazamientoEnBytes;
}typedef indDeCodigo;

struct campoVariables{
	char identificador;
	int valorVariable;
}typedef bloqueVariables;

struct posicionDeRetorno{
	int pag;
	int offset;
	int size;
}typedef retornoVariable;

struct indiceDeStack{
	int posicion;
	t_list* listaArgumentos;
	t_list* listaVariables;
	int retPos;
	retornoVariable retVar;
}typedef indDeStack;

struct indiceDeEtiqueta{
	char* funcion;
	int posicionDeLaEtiqueta;
}typedef indDeEtiqueta;

struct bloqueDeControl{
	int PID;
	int ProgramCounter;
	int cantidadDePaginas;
	indDeCodigo indiceDeCodigo[50];//TODO porque es un array de 50 posiciones exactamente?
	indDeEtiqueta indiceDeEtiquetas;
	indDeStack indiceDeStack;
}typedef PCB;

AnSISOP_funciones* funciones;
AnSISOP_kernel* funciones_kernel;


int main(){
	char* rutaPrograma="/home/utnso/Escritorio/miPrograma";
	t_metadata_program* miMetaData = malloc(sizeof(t_metadata_program));
	miMetaData = metadata_desde_literal(rutaPrograma);

	printf("%d",miMetaData->instrucciones_serializado->start);

	return 0;
}

int ejecutarPrograma(char* unPrograma,PCB bloqueControl){
	int i;
	t_metadata_program* miMetaData = metadata_desde_literal(unPrograma);

	//char *instruccion_a_ejecutar = malloc (nextInstructionPointer->offset);

	//TODO aca se debe obtener la siguente instruccion a ejecutar.
	//memcpy(instruccion_a_ejecutar, nextInstructionPointer->start , nextInstructionPointer->offset); //El warning es porque en ves de una direccion de memoria se le pasa un VALOR de direccion de memoria

	//////TODO EJECUTAR  instruccion_a_ejecutar
	//analizadorLinea(instruccion_a_ejecutar,funciones,funciones_kernel);

	//free (instruccion_a_ejecutar);

	return EXIT_SUCCESS;
}

int armarIndiceDeEtiquetas(PCB unBloqueControl,t_metadata_program* miMetaData){
	int i;
	t_puntero_instruccion devolucionEtiqueta;

	for( i=0; i < miMetaData->cantidad_de_etiquetas; i++ ){
		devolucionEtiqueta = metadata_buscar_etiqueta(miMetaData->etiquetas[i],miMetaData->etiquetas,miMetaData->etiquetas_size);//TODO se tiene que agregar una validacion porque la funcion devuelve un error si no se encontro la etiqueta.
		//TODO esto esta mal Funcion no es lo mismo que etiqueta... ver como identificar etiquetas
		unBloqueControl.indiceDeEtiquetas.funcion = miMetaData->etiquetas;
		unBloqueControl.indiceDeEtiquetas.posicionDeLaEtiqueta = devolucionEtiqueta;
	}
	return 0;
}

int armarIndiceDeCodigo(PCB unBloqueControl,t_metadata_program* miMetaData){
	int i;

	int programOffset  = 0;
	t_intructions * nextInstructionPointer =  NULL;

	//First instruction
	nextInstructionPointer->start = miMetaData->instrucciones_serializado->start;
	nextInstructionPointer->offset = miMetaData->instrucciones_serializado->offset;

	for (i= miMetaData->instruccion_inicio; i < miMetaData->instrucciones_size ; i++){

		unBloqueControl.indiceDeCodigo[i].inicioDeInstruccion = nextInstructionPointer->start;
		unBloqueControl.indiceDeCodigo[i].desplazamientoEnBytes = nextInstructionPointer->offset;

		programOffset += nextInstructionPointer->start + nextInstructionPointer->offset ;

		memcpy(nextInstructionPointer, programOffset, sizeof(t_intructions));//El warning es porque en ves de una direccion de memoria se le pasa un VALOR de direccion de memoria

	}

	return 0;
}

int definirVariable(char nombreVariable,PCB miPrograma,int posicion){
	bloqueVariables *nuevaVariable;

	nuevaVariable->identificador = nombreVariable;
	miPrograma.indiceDeStack.posicion = 0;

	list_add(miPrograma.indiceDeStack.listaVariables, (void*) &nuevaVariable);

	miPrograma.indiceDeStack.retPos=posicion;

	return 0;
}
