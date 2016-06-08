/****************************************************
* Common Types for all the processes 
***************************************************/
#include <commons/collections/list.h>
#include <stdio.h>

typedef enum{
	DISCO = 0//TODO definir enum_dispositivos
} enum_dispositivos;

typedef struct{
	//TODO ver como implementar t_nombre_semaforo
} t_nombre_semaforo;

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
	int StackPointer;
	int cantidadDePaginas;
	int estado; //0: New, 1: Ready, 2: Exec, 3: Block, 4:5: Exit
	int finalizar;
	char path[250];
	t_list *indiceDeCodigo;//cola o pila con registros del tipo t_registroIndiceCodigo
	t_list *indiceDeEtiquetas;//cola o pila con registros del tipo t_registroIndiceEtiqueta
	t_list *indiceDeStack;//cola o pila con registros del tipo t_registroStack
}typedef t_PCB;

void setPageSize (int pageSize);
int getLogicalAddress (int page);
int definirVariable(t_vars nombreVariable);
int obtenerPosicionVariable(t_vars identificador_variable);
void *dereferenciar(t_memoryLocation direccion_variable);
void asignar(t_memoryLocation direccion_variable, t_memoryLocation valor);
t_memoryLocation *obtenerValorCompartida(t_vars variable);
t_memoryLocation asignarValorCompartida(t_vars variable, t_memoryLocation valor);
void irAlLabel(t_registroIndiceEtiqueta etiqueta);
void llamarConRetorno(t_registroIndiceEtiqueta etiqueta, t_registroStack donde_retornar);
void retornar(t_memoryLocation *retorno);
int imprimir(t_memoryLocation valor_mostrar);
int imprimirTexto(char* texto);
int entradaSalida(enum_dispositivos dispositivo, int tiempo);
void wait(t_nombre_semaforo identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);
