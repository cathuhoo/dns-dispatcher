#include "common.h"
#include "dispatcher.h"
#include "external.h"

static void * dispatcher_thread_handler( void * args);

static void * dispatcher_thread_handler( void * args)
{
    int index = *((int *) args);

    debug("dispatcher :%d thread is running..\n", index);

    free(args);

    //pthread_t tid_me = pthread_self();

    int listenfd;
    struct sockaddr_in client_addr, server_addr;
    socklen_t addrLen;
    char query_buffer[NS_MAXMSG];

    int queryLen;
    int port_num = PORT_DISPATCHER + index;
    debug("Thread[%d] wants to create port %d\n", index, port_num);

    listenfd = CreateServerSocket(AF_INET, SOCK_DGRAM, "127.0.0.1",  PORT_DISPATCHER + index,
                                   (struct sockaddr *)&server_addr); 
    if( listenfd <0)
    {
        fprintf(stderr, "Error: Cannot create server socket for dispatcher %d, port :%d \n", index, port_num);
        disp_addr[index].sockfd= -1; 
        disp_addr[index].port= -1; 
        return NULL; 
    }
    //disp_addr[index].sockfd= listenfd;
    disp_addr[index].port= port_num;

    fd_set read_fds;
    int max = listenfd +1;

    for (;;)
    {
        FD_ZERO(&read_fds);
        FD_SET(listenfd, &read_fds);
        
        struct timeval timeout={TIMEOUT,0};
        int count = select(max, &read_fds, NULL, NULL, &timeout);
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
            /*
            #ifdef DEBUG
                 fprintf(stdout, "dispatcher is waiting ...\n");
                 debug(" count=%d\n", count);
            #endif
            */
            continue;
            //return NULL;
         }
            
        if (FD_ISSET(listenfd, &read_fds) )
        {
            memset(query_buffer, 0, sizeof(query_buffer));
            addrLen=sizeof(client_addr);
            int num, rcode;
            queryLen = recvfrom(listenfd, (char *) &num , NS_MAXMSG, 0,
                                       (struct sockaddr * )&client_addr, &addrLen);
            debug("DISPATCHER: I recieved: %d bytes:%d\n", queryLen, num);

            Query *pQuery = queries.queries[num];  //querylist_lookup_byIndex(&queries,num);
            if (pQuery == NULL)
            {
                error_report("Can not find query[%d]\n", num);
                continue; 
            }
            rcode = query_parse(pQuery);
            if ( rcode != -1)
            {
            }  
            else
            {
                error_report("Can't parse dispatch ");
                continue;
            }
            long cAddr_h;
            struct sockaddr_in *pca;
            pca = &pQuery->client_addr;
            
            cAddr_h = ntohl(pca->sin_addr.s_addr);

            Action *pAct = policy_lookup(&policy, cAddr_h, pQuery->qname);
            resolver_display( pAct->resolver); 

            if ( pAct == NULL)
            {
                char  strAddr[MAX_WORD];
                error_report("No Policy for this query(from %s for name:%s) \n", 
                            sock_ntop((SA*) &(pQuery->client_addr), sizeof(SA), strAddr, sizeof(strAddr)),
                            pQuery->qname);
                query_free(pQuery);
                queries.queries[num]=NULL; 
            } 
            if( pAct->op == Drop)
            {
                char  strAddr[MAX_WORD];
                error_report("Drop query query(from %s for name:%s) \n", 
                            sock_ntop((SA*) &(pQuery->client_addr), sizeof(SA), strAddr, sizeof(strAddr)),
                            pQuery->qname);
                query_free(pQuery);
                queries.queries[num]=NULL; 
            }
            else if ( pAct->op == Forward || pAct->op == Refuse || pAct->op == Redirect )
            {
                pQuery->status = dispatched;
                pQuery->resolver = pAct->resolver;
                pQuery->op = pAct->op;
                //notify the recv_send that this query is ready for forwarding
                sendto( listenfd, (char *) &num, sizeof(num), 0, (SA*)&client_addr, addrLen); 
                debug("Policy lookup finished for %d: %s \n", num, pQuery->qname );
            }
            
        }

    }  

   return NULL;
}


pthread_t dispatcher(int idx )
{
    pthread_t tid;
    int * pIndex = malloc(sizeof(int));
    *pIndex = idx;
    if ( 0 != pthread_create(&tid, NULL, dispatcher_thread_handler, (void*)pIndex ))
    {
        error_report("Cannot create thread for dispatcher\n");
        return 0;
    }
    return  tid;
}
