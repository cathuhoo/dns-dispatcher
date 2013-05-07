#ifndef  __DISPATCHER_H__
#define  __DISPATCHER_H__

//#include <pthread.h>
#include "common.h"

typedef struct _sock_info{
    int sockfd;// the recv_send thread create  and use this file discriptor  to communicate the dispatcher
    struct sockaddr  server_addr;
    int port;

    char  path_name[MAX_WORD];
    struct sockaddr_un *cliAddr;
} Disp_info;

pthread_t dispatcher( int i ) ; //char * unix_sock_name);

//static void * dispatcher_thread_handler( void * args);

#include "external.h"

#endif
