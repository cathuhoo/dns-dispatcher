//#include "common.h"
//#include "external.h"

#include "dispatcher.h"

static void * dispatcher_thread_handler( void * args);

static void * dispatcher_thread_handler( void * args)
{
    int index = *((int *) args);
    free(args);

    int listenfd;
    struct sockaddr_in client_addr, server_addr;
    socklen_t addrLen;
    char query_buffer[NS_MAXMSG];

    int queryLen;
    int port_num = PORT_DISPATCHER + index;

    listenfd = CreateServerSocket(AF_INET, SOCK_DGRAM, "127.0.0.1",  PORT_DISPATCHER + index,
                                   (struct sockaddr *)&server_addr); 
    if( listenfd <0)
    {
        my_log("Error: Cannot create server socket for dispatcher %d, port :%d \n", index, port_num);
        disp_addr[index].sockfd= -1; 
        disp_addr[index].port= -1; 
        pthread_exit(NULL); 
    }
    disp_addr[index].port= port_num;
    
    //Tell the recv_send thread where I am ready
    disp_addr[index].ready= TRUE;

    debug("Dispatcher[%d] is ready\n", index);

    fd_set read_fds;
    int max = listenfd +1;
    int num, rcode;
    

    for (;;)
    {
        if (parentRequestStop)
        {
            //debug("dispacher[%d] will quit now on request.\n", index);
            break;
        }
        //debug("Dispatcher[%d] is working...\n", index);
	while(parentRequestPause){};

        FD_ZERO(&read_fds);
        FD_SET(listenfd, &read_fds);
        
        struct timeval timeout={TIMEOUT,0};
        int count = select(max, &read_fds, NULL, NULL, &timeout);
        if (count < 0) //Maybe Interrupted, such as ^C pressed 
        {
            //break;
            continue;
        }
        if(count == 0) //No event in timeout 
        {
            continue;
        }
            
        if (FD_ISSET(listenfd, &read_fds) )
        {
            memset(query_buffer, 0, sizeof(query_buffer));
            addrLen=sizeof(client_addr);
            //queryLen = recvfrom(listenfd, (char *) &num , NS_MAXMSG, 0,
            queryLen = recvfrom(listenfd, (char *) &num , sizeof(num), 0,
                                       (struct sockaddr * )&client_addr, &addrLen);

	    if( queryLen < 2 || num <0 || num >= MAX_QUERY_NUM) 
	    {
		my_log("Error: read index of query error, len=%d, num=%d\n",
			queryLen, num);
		continue;
	    }
            Query *pQuery = queries.queries[num];  //querylist_lookup_byIndex(&queries,num);
            if (pQuery == NULL)
            {
                my_log("Error: Cannot find query[%d]\n", num);
                continue; 
            }
            rcode = query_parse(pQuery);
            if ( rcode == -1)
            {
                my_log("Error: Can't parse query in dispatch\n");
                //it should be freed here
                querylist_free_item(&queries,num); 
                continue;
            }
            long cAddr_h;
            struct sockaddr_in *pca;
            pca = &pQuery->client_addr;
            
            cAddr_h = ntohl(pca->sin_addr.s_addr);

            Action *pAct = policy_lookup(&policy, cAddr_h, pQuery->qname);
            //resolver_display( pAct->resolver); 

            //char  strAddr[MAX_WORD];
            if ( pAct == NULL)
            {
		/*
                my_log("Error: No Policy for this query(from %s for name:%s) \n", 
                            sock_ntop((SA*) &(pQuery->client_addr), sizeof(SA), strAddr, sizeof(strAddr)),
                            pQuery->qname);
		*/
                //query_free(pQuery);
                //queries.queries[num]=NULL; 
                querylist_free_item(&queries,num); 
            } 
            if( pAct->op == Drop)
            {
		/*
                my_log("Dropped query(from %s for name:%s) \n", 
                            sock_ntop((SA*) &(pQuery->client_addr), sizeof(SA), strAddr, sizeof(strAddr)),
                            pQuery->qname);
		*/
                //query_free(pQuery);
                //queries.queries[num]=NULL; 
                querylist_free_item(&queries,num); 
            }
            else if ( pAct->op == Forward || pAct->op == Refuse || pAct->op == Redirect )
            {
                pQuery->status = dispatched;
                pQuery->resolver = pAct->resolver;
                pQuery->op = pAct->op;
                //notify the recv_send that this query is ready for forwarding
                sendto( listenfd, (char *) &num, sizeof(num), 0, (SA*)&client_addr, addrLen); 
                //debug("Policy lookup finished for %d: %s \n", num, pQuery->qname );
            }
            
        } //IF(ISSET()

    }  //for(;;)

   //debug("Dispatcher thread[%d] will exit...\n", index);
   pthread_exit(NULL);
}

pthread_t dispatcher(int idx )
{
    pthread_t tid;
    int * pIndex = malloc(sizeof(int));
    *pIndex = idx;
    if ( 0 != pthread_create(&tid, NULL, dispatcher_thread_handler, (void*)pIndex ))
    {
        my_log("Error: Cannot create thread for dispatcher\n");
        return 0;
    }
    return  tid;
}
