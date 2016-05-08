/*
 * serializacion.h
 *
 */

#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#include <string.h>

typedef struct enviarCPU{
int head;
int pid;
char path[250];
int pc;
int cantInstruc;
}t_contexto;

typedef struct datosRespuesta{
	int operacion;
	int pid;
}t_respuesta;

typedef struct datosEntradaSalida{
	int tiempo;
	int pc;
}t_es;

void serializarCPU(t_contexto, char**);
void deserializarRespuesta(t_respuesta*, char**);
void deserializarEntradaSalida(t_es*,char**);
void deserializarArray(float [], int , char **);


#endif /* SERIALIZACION_H_ */
