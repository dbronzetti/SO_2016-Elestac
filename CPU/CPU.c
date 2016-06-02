#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include <stdio.h>


struct indiceDeCodigo{
	int numeroDeInstruccion;
	int ubicacionEnBytes;
}typedef indDeCodigo;

struct indiceDeStack{
	int posicion;
	t_list* listaArgumentos;
	t_list* listaVariables;
	int retPos;
	int retVar;
}typedef indDeStack;

struct indiceDeEtiqueta{
	char* funcion;
	int posicionDeLaEtiqueta;
}typedef indDeEtiqueta;

struct bloqueDeControl{
	int PID;
	int ProgramCounter;
	int cantidadDePaginas;
	indDeCodigo indiceDeCodigo[50];
	indDeEtiqueta indiceDeEtiquetas;
	indDeStack indiceDeStack;
}typedef PCB;





int main(){
	char* rutaPrograma="/home/utnso/Escritorio/miPrograma";
	t_metadata_program* miMetaData=metadata_desde_literal(rutaPrograma);
	t_puntero_instruccion miInicio=miMetaData->instrucciones_serializado[0].start;
	t_puntero_instruccion offsetInicio=miMetaData->instrucciones_serializado[0].offset;
	printf("%s",miMetaData->instrucciones_serializado[0].start);
	return 0;
}

int ejecutarPrograma(char* unPrograma){
	t_metadata_program* miMetaData=metadata_desde_literal(unPrograma);
	t_puntero_instruccion miInicio=miMetaData->instrucciones_serializado[0].start;
	t_puntero_instruccion offsetInicio=miMetaData->instrucciones_serializado[0].offset;

	return 0;
}

