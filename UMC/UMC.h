#ifndef UMC_H_
#define UMC_H_

#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include "sockets.h"
#include "commons/collections/list.h"
#include "commons/string.h"
#include "common-types.h"

#define EOL_DELIMITER ";"
#define PAGE_PRESENT 1
#define PAGE_NOTPRESENT 0
#define PAGE_MODIFIED 1
#define PAGE_NOTMODIFIED 0

typedef struct {
	int port;
	char ip_swap[15]; /* 15 = because the format xxx.xxx.xxx.xxx*/
	int port_swap;
	int frames_max;
	int frames_size;
	int frames_max_proc;
	int TLB_entries; /* 0 = Disable */
	int delay;
	char algorithm_replace[20]; /* rounding up max length "Clock Modificado" */
} t_configFile;

typedef struct {
	int socketServer;
	int socketClient;
} t_serverData;

typedef struct{
	t_memoryLocation virtualAddress;
	int frameNumber;
	int PID;
	unsigned dirtyBit : 1;//field of 1 bit
	unsigned presentBit : 1;//field of 1 bit
} t_memoryAdmin; /* It will be created as a fixed list at the beginning of the process using PID as element index */

typedef enum{
	PUERTO = 0,
	IP_SWAP,
	PUERTO_SWAP,
	MARCOS,
	MARCO_SIZE,
	MARCO_X_PROC,
	ENTRADAS_TLB,
	RETARDO,
	ALGORITMO
} enum_configParameters;

/***** Global variables *****/
t_configFile configuration;
void *memBlock;
pthread_mutex_t socketMutex;
t_list *TLB;
t_list *pageTablesList;

/***** Prototype functions *****/

//UMC operations
void getConfiguration(char *configFile);
void createTLB();
void resetTLBEntries();
void consoleMessageUMC();
void createAdminStructs();
int getEnum(char *string);

//Communications
void startUMCConsole();
void startServer();
void newClients (void *parameter);
void processMessageReceived (void *parameter);
void handShake (void *parameter);

//UMC functions
void initializeProgram(int PID, int totalPagesRequired, char *programCode);
char *requestBytesFromPage(t_memoryLocation virtualAddress);
void writeBytesToPage(t_memoryLocation virtualAddress, char *buffer);
void endProgram(int PID);
void waitForResponse();

#endif /* UMC_H_ */
