#include <sys/select.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#include "common.h"

#include "query.h"
#include "list.h"
#include "policy.h"
#include "config.h"

#include "listener.h"

#include "external.h"


#define TIMEOUT 5
/*
#define  error_report(fmt, ...)  \
        { \
         fprintf(stderr, fmt, ##__VA_ARGS__); fprintf(stderr, "%s(%d)\n",__FILE__, __LINE__); \
        }
*/


/* 
 * Open and listen to the following sockets:
 *  1. one udp server socket to accept client queries via udp
 *  2. one tcp server socket to accept client queries via tcp
 *  3. multiple udp client sockets to all the upstream resolvers
 *  
 *  When a DNS query comes from a client, this process will:
 *  1. add the query to the query list;
 *  2. select and notify a dispatcher thread from the unix socket
 *
 *  When a DNS reply comes from an upstream resolver, this thread will:  
 *  1. attach the reply to right item in the query list, if any;
 *  2. notify the sender thread 
 *
 */

typedef struct _args{
     Configuration * config;
     ResolverList * resolvers;
     Policy * policy;
     QueryList * queries;
} ARGS;

/*
extern pthread_t tid_listener;
extern pthread_t *tid_dispatchers;
extern pthread_t tid_sender;
*/

pthread_t listener()//List * resolvers, Configuration *config, QueryList * queries)
{

    debug("Now in listener\n");
    pthread_t tid;
    ARGS * args;

    args = (ARGS *) malloc(sizeof(ARGS));

    args->config =&config;
    args->resolvers = &resolvers;
    args->policy = &policy;
    args->queries = & queries;

    if ( 0 != pthread_create(&tid, NULL, &listen_thread_handler, (void *) args))
    {
        error_report("Cannot create thread for listener\n");
        return 0;
    }
    return tid;
}
int udp_query_process()
{
    debug("udp_query_process\n");

    return 0;
} 
void * listen_thread_handler(void * arg)
{

    debug("Now in listen_thread_handler\n");
    fd_set read_fds;
    int *resolverSockFds;
    int i, num_resolvers;
    ARGS *global_vars;

    ResolverList * resolvers_local;

    global_vars = (ARGS *)arg;

    resolvers_local = global_vars -> resolvers;

    resolverSockFds =  malloc(sizeof (int) * resolvers_local->size);

    struct sockaddr_in udpServiceAddr, tcpServiceAddr;

    for (i=0; i< resolvers_local->size; i ++)// for each resolver
    {
        Resolver *res=resolvers_local->resolvers[i];
       int sockfd;
       sockfd = CreateClientSocket(res->ipaddress, SOCK_DGRAM, res->udp_port, &res->server_addr );
       if (sockfd == -1)  
       {
           //Error, skip it
           continue; 
       }
       else
       {
            // add this sockfd to sockfd_monitored
            resolverSockFds[i] = sockfd;
            res->sockfd = sockfd;
            i ++ ;
       }
    } 
    num_resolvers = i;

    //TODO:
    int udpServiceFd, tcpServiceFd;

    udpServiceFd = CreateServerSocket( SOCK_DGRAM,  config.service_port, &udpServiceAddr, 1);//1 for open

    tcpServiceFd = CreateServerSocket( SOCK_STREAM, config.service_port, &tcpServiceAddr, 1);

   int max_fd =  maximum(resolverSockFds, num_resolvers);
   max_fd =  MAX2(max_fd, udpServiceFd);
   max_fd =  MAX2(max_fd, tcpServiceFd);

   debug("max_fd:%d\n", max_fd);

   for (;;)
   {
       int count;
       //int timeout = TIMEOUT;
	   //struct timeval timeout={TIMEOUT,0};
       struct timespec timeout={5,0};

       FD_ZERO(&read_fds);
       FD_SET(udpServiceFd,&read_fds);
       FD_SET(tcpServiceFd,&read_fds);
       for( i=0; i< num_resolvers; i ++)
          FD_SET( resolverSockFds[i], &read_fds);

       debug("before select, max_fd=%d, timeout=%d\n", max_fd, timeout);
       //count = select( max_fd, &read_fds, NULL, NULL, NULL); // &timeout); 
       count = pselect( max_fd, &read_fds, NULL, NULL, NULL, NULL); // &timeout); 

       debug("after select, count=%d\n", count);
        
       if (count < 0) //Maybe Interrupted, such as ^C pressed 
       {
            #ifdef DEBUG
       		debug(" Interrupted, count=%d\n", count);
                break;
            #else
                continue;
            #endif
        }

        if(count == 0) //No event in timeout 
        {
            //do something, or continue...
            #ifdef DEBUG
                fprintf(stdout, "listener is waiting ...\n");
       		debug(" count=%d\n", count);
            #endif
            continue;
        }

        // Request from clients via UDP 
        if (FD_ISSET(udpServiceFd, &read_fds) )  
        {
       	    debug(" udpService =%d\n", count);
            udp_query_process();
        }

        // Request from clients via TCP 
        if (FD_ISSET(tcpServiceFd, &read_fds) )
        {
       	    debug(" tcpService =%d\n", count);
            //tcp_request_process();
        }
        
        // Request from upstream forwarders   
        for( i=0; i< num_resolvers; i ++)
        { 
            if ( FD_ISSET (resolverSockFds[i], &read_fds))
            {
       	    	debug("forwarderSocks[%d], %d\n",i, count);
               //udp_reply_process(); 
            }
        }

   }
    return 0;
}
