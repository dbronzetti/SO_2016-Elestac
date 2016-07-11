/****************************************************
* Common Types for all the processes
***************************************************/
#include <commons/collections/list.h>
#include <stdio.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>

typedef enum{
	DISCO = 0//TODO definir enum_dispositivos
} enum_dispositivos;

typedef enum{
	agregar_proceso = 1,
	finalizar_proceso,
	lectura_pagina,
	escritura_pagina
} enum_operationsUMC_SWAP;

typedef enum{
	id_wait=0,
	id_signal,
}enum_semaforo;

typedef struct{
	int pag;
	int offset;
	int size;
} t_memoryLocation;

typedef struct{
	char identificador;
	t_memoryLocation *direccionValorDeVariable; //
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
	t_memoryLocation* retVar;
} t_registroStack;

struct indiceDeEtiqueta{
	char* funcion;
	int posicionDeLaEtiqueta;
}typedef t_registroIndiceEtiqueta;

struct bloqueDeControl{
	int PID;
	int ProgramCounter;
	int cantidadDePaginas;// Cantidad de paginas de codigo
	int StackPointer;
	int estado; //0: New, 1: Ready, 2: Exec, 3: Block, 4:5: Exit
	int finalizar;
	t_list *indiceDeCodigo;//cola o pila con registros del tipo t_registroIndiceCodigo
	char *indiceDeEtiquetas;
	t_list *indiceDeStack;//cola o pila con registros del tipo t_registroStack
}typedef t_PCB;

struct variableCompartidaAEnviar{
	t_valor_variable valorVariable;
	t_nombre_compartida nombreCompartida;
}typedef struct_compartida;

typedef struct datosEntradaSalida {
	int tiempo;
	int ProgramCounter;
	t_nombre_dispositivo dispositivo;
} t_es;

typedef struct {
	t_nombre_variable variable;
	t_valor_variable valor;
}t_privilegiado;

void setPageSize (int pageSize);
int getLogicalAddress (int page);
void serializarES(t_es *dispositivoEnviar, char *dispositivoSerializado);
void deserializarES(t_es* datos, char* buffer);






void serializarVars(t_vars* miRegistro, char* value);
void serializeMemoryLocation(t_memoryLocation* unaPosicion, char* value);
void serializeListaArgs(t_list* listaASerializar, char* listaSerializada);
void serializarListasVars(t_list* listaASerializar, char* listaSerializada);
void serializarStack(t_registroStack* registroStack, char* registroSerializado);
void deserializarMemoryLocation(t_memoryLocation* unaPosicion,char* posicionRecibida);
int deserializarListasVars(t_list* listaVars,char* listaSerializada);
int deserializarListasArgs(t_list* listaArgs,char* listaSerializada);
void deserializarVars(t_vars* unaVariable, char* variablesRecibidas) ;
void deserializarStack(t_registroStack* estructuraARecibir, char* registroStack);
void deserializarListaStack(t_list* listaARecibir, char* listaSerializada);

void serializarIndiceDeCodigo(t_registroIndiceCodigo* registroEnviar, char* registroSerializado);
void deserializarIndiceDeCodigo(t_registroIndiceCodigo* registroARecibir, char* registroSerializado);
void serializarListaIndiceDeCodigo(t_list* listaARecibir, char* listaSerializada);
void deserializarListaIndiceDeCodigo(t_list* listaARecibir, char* listaSerializada);
void serializarRegistroStack(t_registroStack* registroASerializar, char* registroSerializado);
void serializarListaStack(t_list* listaASerializar, char* listaSerializada);

void serializarPCB(t_MessageNucleo_CPU* value, char* buffer, int valueSize);


