
#include "common.h"
#include "dispatcher.h"

#include "external.h"


void * dispatcher_thread_handler( void * args)
{
    debug("dispatcher thread is running..\n");

    int sockfd=  *((int *)args);
    free(args);

    pthread_t tid_me = pthread_self();
    //pthread_detach (pthread_self());

    for (;;)
    {
        sleep(TIME_SLEEP);
        debug("dispatcher thread[%lu] is running, listen on un_socket:%d..\n", (unsigned long)tid_me, sockfd);
        
    }  
}


pthread_t dispatcher( char * unix_sock_name)
{
    debug("Dispatcher Begin\n");
    debug("on unix socket:%s\n", unix_sock_name);
    
    pthread_t tid;
    char un_name[MAX_WORD];
    int *sockfd= malloc(sizeof(int));

    struct sockaddr_un servaddr, cliaddr;

    unlink(unix_sock_name);

    *sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    memset(&servaddr, 0, sizeof(struct sockaddr_un));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, unix_sock_name);
    bind(*sockfd, (struct socakaddr *) &servaddr, sizeof(servaddr));

    
    if ( 0 != pthread_create(&tid, NULL, &dispatcher_thread_handler, (void*)sockfd ))
    {
        error_report("Cannot create thread for dispatcher\n");
        return 0;
    }
    return  tid;
}
