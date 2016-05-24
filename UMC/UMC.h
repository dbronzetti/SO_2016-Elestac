#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include "sockets.h"
#include "commons/collections/list.h"

#define EOL_DELIMITER ";"

typedef struct {
	int port;
	char ip_swap[15]; /* 15 = because the format xxx.xxx.xxx.xxx*/
	int port_swap;
	int frames_max;
	int frames_size;
	int frames_max_proc;
	int TLB_entries; /* 0 = Disable */
	int delay;
} t_configFile;

typedef struct {
	int socketServer;
	int socketClient;
} t_serverData;

typedef enum{
	PUERTO = 0,
	IP_SWAP,
	PUERTO_SWAP,
	MARCOS,
	MARCO_SIZE,
	MARCO_X_PROC,
	ENTRADAS_TLB,
	RETARDO
} enum_configParameters;

void getConfiguration(char *configFile);
int getEnum(char *string);
void startUMCConsole();
void startServer();
void newClients (void *parameter);
void processMessageReceived (void *parameter);
void handShake (void *parameter);
