#include <stdio.h>
#include <stdlib.h>
#include "commons/collections/list.h"

#include <arpa/inet.h>
#include <sys/socket.h>

typedef struct {
	int port;
	char ip_swap[15]; /* 15 = because the format xxx.xxx.xxx.xxx*/
	int port_swap;
	int frames_max;
	int frames_size;
	int frames_max_proc;
	int TLB_entries; /* 0 = Disable */
	int delay;
} configFile;
