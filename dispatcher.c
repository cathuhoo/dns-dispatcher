
#include "common.h"
#include "dispatcher.h"

#include "external.h"


typedef struct __argdis_{
    int  fd;
    struct sockaddr * addr;
} ARGS_DISP;

void * dispatcher_thread_handler( void * args)
{
    debug("dispatcher thread is running..\n");

    ARGS_DISP * pArgs= args;
    int sockfd= pArgs->fd;
    struct sockaddr * server_addr =  pArgs->addr;
    
    free(args);

    pthread_t tid_me = pthread_self();
    //pthread_detach (pthread_self());

    for (;;)
    {
        debug("dispatcher thread[%lu] is running, listen on un_socket:%d..\n", (unsigned long)tid_me, sockfd);
        int num;
        char buffer[MAX_WORD];
        socklen_t len;
        struct sockaddr_un client_addr;

        num = recvfrom(sockfd, buffer, MAX_WORD, 0, (SA *) &client_addr, &len);  
        #ifdef DEBUG
            char strAddr[MAX_WORD];
            char * strClient = sock_ntop((SA *)& client_addr, len, strAddr, sizeof(strAddr));
            debug("Got task from %s, message:%s\n",strAddr, buffer ); 
        #endif
        
    }  
}


pthread_t dispatcher(int idx )
{
    debug("Dispatcher Begin\n");

    pthread_t tid;

    ARGS_DISP * pArgs = malloc(sizeof( ARGS_DISP));
    pArgs->addr = malloc(sizeof(struct sockaddr));

/*
    struct sockaddr_un servaddr, cliaddr;

    unlink(unix_sock_name);

    *sockfd = socket(AF_LOCAL, SOCK_DGRAM, 0);
    memset(&servaddr, 0, sizeof(struct sockaddr_un));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, unix_sock_name);
    bind(*sockfd, (struct socakaddr *) &servaddr, sizeof(servaddr));
*/
    char path_name[MAX_WORD];
    sprintf(path_name, "%s_%d", "/tmp/dns_dispatcher_unix_socket_", idx);
    pArgs->fd = CreateServerSocket(AF_LOCAL, SOCK_STREAM, path_name, 0, (struct sockaddr_in *)pArgs->addr); 
    
    if( pArgs->fd <0)
    {
        fprintf(stderr, "Error: Cannot create unix server socket for dispatcher %d \n", idx);
        free(pArgs); 
        return 0; 
    }

    disp_addr[idx].sockfd= pArgs->fd;
    strcpy(disp_addr[idx].path_name, path_name);
    
    if ( 0 != pthread_create(&tid, NULL, &dispatcher_thread_handler, (void*)pArgs ))
    {
        error_report("Cannot create thread for dispatcher\n");
        return 0;
    }
    return  tid;
}
