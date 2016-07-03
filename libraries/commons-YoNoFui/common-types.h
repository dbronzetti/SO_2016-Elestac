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
	char* codeScript;
	t_list *indiceDeCodigo;//cola o pila con registros del tipo t_registroIndiceCodigo
	t_list *indiceDeEtiquetas;//cola o pila con registros del tipo t_registroIndiceEtiqueta
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

void setPageSize (int pageSize);
int getLogicalAddress (int page);
t_puntero definirVariable(t_nombre_variable nombreVariable);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void retornar(t_valor_variable retorno);
void imprimir(t_valor_variable valor_mostrar);
void imprimirTexto(char *texto);
void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void wait(t_nombre_semaforo identificador_semaforo);
void signal(t_nombre_semaforo identificador_semaforo);
int condicionBuscarVarible(t_vars* variableBuscada,t_vars* otraVariable);
void serializarES(t_es *dispositivoEnviar, char *dispositivoSerializado);
void deserializarES(t_es* datos, char* buffer);
