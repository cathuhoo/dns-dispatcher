
#include "common.h"
#include "dispatcher.h"

#include "external.h"


typedef struct __argdis_{
    int  fd;
    struct sockaddr * addr;
} ARGS_DISP;

static void * dispatcher_thread_handler( void * args);

static void * dispatcher_thread_handler( void * args)
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
            debug("Got task from %s, message:%s\n",strClient, buffer ); 
        #endif
   }  

   return NULL;
}


pthread_t dispatcher(int idx )
{
    debug("Dispatcher Begin\n");

    pthread_t tid;

    ARGS_DISP * pArgs =NULL;

    pArgs =  malloc(sizeof( ARGS_DISP));
    if( pArgs ==NULL)
    {
        error_report("Memory error for dispatcher\n");
        return 0;
    } 
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
    snprintf(path_name, MAX_WORD, "%s_%d.sock", "/tmp/dnsd_unix_socket_", idx);
    debug("unix socket:%s\n", path_name);

    pArgs->fd = CreateServerSocket(AF_LOCAL, SOCK_STREAM, path_name, 0, (struct sockaddr_in *)pArgs->addr); 
    
    if( pArgs->fd <0)
    {
        fprintf(stderr, "Error: Cannot create unix server socket for dispatcher %d \n", idx);
        if(pArgs != NULL) 
            free(pArgs); 
        return 0; 
    }

    disp_addr[idx].sockfd= pArgs->fd;
    strcpy(disp_addr[idx].path_name, path_name);
    
    if ( 0 != pthread_create(&tid, NULL, dispatcher_thread_handler, (void*)pArgs ))
    {
        error_report("Cannot create thread for dispatcher\n");
        if(pArgs != NULL) 
            free(pArgs); 
        return 0;
    }
    return  tid;
}
