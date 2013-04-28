#ifndef  __DISPATCHER_H__
#define  __DISPATCHER_H__

#include <pthread.h>

typedef struct _sock_info{
    int sockfd;
    char * path_name;
    struct sockaddr_un *cliAddr;
} Disp_info;

pthread_t dispatcher( int i ) ; //char * unix_sock_name);

#endif
