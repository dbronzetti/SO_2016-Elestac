#ifndef CPU_H_
#define CPU_H_

#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <commons/collections/list.h>
#include "common-types.h"
#include "sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

AnSISOP_funciones* funciones;
AnSISOP_kernel* funciones_kernel;

int ejecutarPrograma(t_PCB *PCB);

#endif /* CPU_H_ */
