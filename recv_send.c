
#include "recv_send.h"
#include "list.h"

#define TIMEOUT 5


int recv_and_send(Policy * policy, List * resolvers, Configuration *config)
{

    fd_set read_fds;
    int *resolverSockFds;
    int i, num_resolvers;
    ListElmt * plink;

    //For each resolver in resolvers 
    
    if ( policy == NULL || resolvers == NULL || config == NULL)
        return -1;

    resolverSockFds =  malloc(sizeof (int) * resolvers->size);

    i=0;
    for (plink = resolvers->head; plink ; plink = plink->next)// for each resolver
    {
        Resolver *res=plink->data;
        int sockfd;
       //TODO
       //int sock_fd = res->connect();
       if (sockfd == -1)  
       {
           //Error, skip it
           continue; 
       }
       else
       {
            // add this sockfd to sockfd_monitored
            resolverSockFds[i] = sockfd;
            i ++ ;
       }
    } 
    num_resolvers = i;

    //TODO:
    int udpServiceFd, tcpServiceFd;
    //udpServiceFd = service.open_udp_socket();
    //tcpServiceFd = service.open_tcp_socket();

   int max_fd =  maximum(resolverSockFds, num_resolvers);
   max_fd =  MAX2(max_fd, udpServiceFd);
   max_fd =  MAX2(max_fd, tcpServiceFd);

   for (;;)
   {
       int count;
       int timeout = TIMEOUT;

       FD_ZERO(&read_fds);
       FD_SET(udpServiceFd,&read_fds);
       FD_SET(tcpServiceFd,&read_fds);
       for( i=0; i< num_resolvers; i ++)
          FD_SET( resolverSockFds[i], &read_fds);


       count = select( max_fd, &read_fds, NULL, NULL, &timeout); 
        
       if (count < 0) //Maybe Interrupted, such as ^C pressed 
       {
            #ifdef DEBUG
                break;
            #else
                continue;
            #endif
        }

        if(count == 0) //No event in timeout 
        {
            //do something, or continue...
            continue;
        }

        // Request from clients via UDP 
        if (FD_ISSET(udpServiceFd, &read_fds) )  
        {
            process_udp_request();
        }

        // Request from clients via TCP 
        if (FD_ISSET(tcpServiceFd, &read_fds) )
        {
            process_tcp_request();
        }
        
        // Request from upstream forwarders   
        for( i=0; i< num_resolvers; i ++)
        { 
            if ( FD_ISSET (resolverSockFds[i], &read_fds))
            {
               process_reply(); 
            }
        }

   }

    return 0;
}
