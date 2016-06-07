/****************************************************
* Common Types for all the processes 
***************************************************/
#include <commons/collections/list.h>

typedef struct{
	int pag;
	int offset;
	int size;
} t_memoryLocation;

typedef struct{
	char *identificador;
	t_memoryLocation direccionValorDeVariable; //
} t_vars;

struct indiceDeCodigo{
	int inicioDeInstruccion;//Se carga con un contador cada vez que se deserealiza una instruccion desde "t_metadata_program.instrucciones_serializado"
	int longitudInstruccionEnBytes; // strlen de la instruccion
}typedef t_registroIndiceCodigo;

typedef struct{
	int pos; // Position in stack
	t_list *args; // List of address memory position for function arguments with format t_memoryLocation
	t_list *vars; // List of vars with format t_vars
	int retPos; // si hay una funcion se deberia cargar con el valor "t_registroIndiceCodigo.inicioDeInstruccion" de esa instruccion
	t_memoryLocation retVar;
} t_registroStack;

struct indiceDeEtiqueta{
	char* funcion;
	int posicionDeLaEtiqueta;
}typedef t_registroIndiceEtiqueta;

struct bloqueDeControl{
	int PID;
	int ProgramCounter;
	int cantidadDePaginas;
	t_list *indiceDeCodigo;//cola o pila con registros del tipo t_registroIndiceCodigo
	t_list *indiceDeEtiquetas;//cola o pila con registros del tipo t_registroIndiceEtiqueta
	t_list *indiceDeStack;//cola o pila con registros del tipo t_registroStack
}typedef t_PCB;
