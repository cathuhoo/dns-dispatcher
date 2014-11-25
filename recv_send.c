#include "recv_send.h"

/* 
 * Open and listen to the following sockets:
 *  1. one udp server socket to accept client queries via udp
 *  2. one tcp server socket to accept client queries via tcp
 *  3. multiple sockets fds to the dispatcher threads
 *  4. multiple udp client sockets to all the upstream resolvers
 *  
 *  When a DNS query comes from a client, this process will:
 *  1. add the query to the query list;
 *  2. select and notify a dispatcher thread to select the right resolver
 *
 * When a notification  comes from the dispatcher
 *  Forward the query to the selected upstream resolver
 *
 * When a DNS reply comes from an upstream resolver, this thread will:  
 *  Forward the reply to right client, according to the item in the query list, if any;
 */

//#define  TCP_SERVICE

static void * listen_thread_handler();

pthread_t recv_send() 
{
    pthread_t tid;

    if ( 0 != pthread_create(&tid, NULL, &listen_thread_handler, NULL))
    {
        my_log("Error: Cannot create thread for listener\n");
        return 0;
    }
    return tid;
}

static void * listen_thread_handler( )
{
    fd_set read_fds;
    int *resolverSockFds;
    int i, num_resolvers;

    // Service: recieve DNS queries from Clients
    int udpServiceFd; 
    int tcpServiceFd;

    struct sockaddr_in udpServiceAddr, tcpServiceAddr;

    udpServiceFd = CreateServerSocket(AF_INET, SOCK_DGRAM,  "0.0.0.0", config.service_port, 
                                     (SA*) &udpServiceAddr);
    if(udpServiceFd < 0) 
    {
        my_log("Cannot create socket for udp service, service port=%d\n", config.service_port);
        pthread_exit(NULL);
    }
/*
    tcpServiceFd = CreateServerSocket(AF_INET, SOCK_STREAM, "0.0.0.0", config.tcpservice_port, 
                                     (SA *) &tcpServiceAddr);
    if(tcpServiceFd < 0) 
    {
        my_log("Cannot create socket for tcp service, service port=%d\n", config.tcpservice_port);
        pthread_exit(NULL);
    }
*/
    tcpServiceFd = 0; // for disable TCP service


    //Connect to all the resolvers
    resolverSockFds =  malloc(sizeof (int) * resolvers.size);
    for (i=0; i< resolvers.size; i++)// for each resolver
    {
       Resolver *res=resolvers.resolvers[i];
       int sockfd = CreateClientSocket(AF_INET, res->ipaddress, SOCK_DGRAM, res->udp_port,(SA*) &res->server_addr );
       if (sockfd == -1)  
       {
           my_log("Error: CreateClientSocket Error for resolver: %s\n", res->name);
           free(resolverSockFds);
           pthread_exit(NULL);
       }
       else
       {
            // add this sockfd to sockfd_monitored
            resolverSockFds[i] = sockfd;
            res->sockfd = sockfd;
       }
    } 
    num_resolvers = i;

    //Connect to dispatchers:
    int *dispatcherSockFds=malloc( sizeof(int) * config.num_threads);
    for (i=0; i< config.num_threads; i ++)
    {
        int sockfd;
        struct sockaddr_in server_addr;
        int port = disp_addr[i].port;
        if ( port == -1) 
            continue;
        sockfd = CreateClientSocket(AF_INET, "127.0.0.1", SOCK_DGRAM, port , (SA *)&server_addr);
        if (sockfd == -1)
        {
           my_log("Error: Connect to dispatcher[%d] failed, port: %d \n", i, disp_addr[i].port);
           free(resolverSockFds);
           free(dispatcherSockFds); 
           pthread_exit(NULL);
        }
        else
        {
            dispatcherSockFds[i] = sockfd; 
            disp_addr[i].sockfd = sockfd;
            memcpy( &(disp_addr[i].server_addr) ,  &server_addr, sizeof(SA));
        }
    }

   int max_fd;
   max_fd =  MAX2(udpServiceFd, tcpServiceFd);

   int max_fd1 =  maximum(resolverSockFds, num_resolvers);
   int min_fd1 =  minimum(resolverSockFds, num_resolvers);

   if ( 0 > query_id_mapping_alloc(&queries, min_fd1, max_fd1))
    {
       my_log("Error: Memory allocate error for id_mapping\n");
       free(resolverSockFds);
       free(dispatcherSockFds); 
       pthread_exit(NULL);
    }
   int max_fd2 =  maximum(dispatcherSockFds, config.num_threads);

   max_fd =  MAX2(max_fd, max_fd1);
   max_fd =  MAX2(max_fd, max_fd2) +1 ;

   debug("RECV_SEND thread is ready(max_fd1 = %d)\n", max_fd1);
   for (;;)
   {
       int count;
	   struct timeval timeout={TIMEOUT,0};

        if (parentRequestStop)
        {
            //debug("recv_send will quit now on request.\n"); 
            break; 
        }
        while(parentRequestPause){};

       FD_ZERO(&read_fds);
       FD_SET(udpServiceFd,&read_fds);

       //FD_SET(tcpServiceFd,&read_fds);

       for( i=0; i< num_resolvers; i ++)
          FD_SET( resolverSockFds[i], &read_fds);

       for( i=0; i< config.num_threads; i ++)
          FD_SET( dispatcherSockFds[i], &read_fds);

       count = select( max_fd, &read_fds, NULL, NULL, &timeout); 

        if (count < 0) //Maybe Interrupted, such as ^C pressed 
        {
           continue;
        }
        else if(count == 0) //No event in timeout 
        {
            continue;
        }

        // Request from clients via UDP 
        if (FD_ISSET(udpServiceFd, &read_fds) )  
        {
            udp_query_process(udpServiceFd);
        }
	/*
        // Request from clients via TCP 
        if (FD_ISSET(tcpServiceFd, &read_fds) )
        {
            tcp_query_process(tcpServiceFd);
        }
	*/

        // Request from upstream resolvers   
        for( i=0; i< num_resolvers; i ++)
        { 
            if ( FD_ISSET (resolverSockFds[i], &read_fds))
            {
                reply_process(resolverSockFds[i], udpServiceFd, tcpServiceFd); 
            }
        }
        // Notification  from  dispatcher   
        for( i=0; i< config.num_threads; i ++)
        { 
            if ( FD_ISSET (dispatcherSockFds[i], &read_fds))
            {
                forward_query_process(dispatcherSockFds[i]); 
            }
        }
        
   } //end for(;;)
   free(resolverSockFds);
   free(dispatcherSockFds); 
   debug("RECV_SEND thread will exit\n");
   pthread_exit(NULL);
}

int notify_dispatcher(int index)
{
    int i = index % config.num_threads; //dispatcher number
    int bytes = sendto(disp_addr[i].sockfd, (char * )&index , sizeof(index), 0, 
	  		(SA *) &(disp_addr[i].server_addr), sizeof(SA));
    if (bytes < sizeof(index))
    {
	my_log("Error: notify_dispatcher sendto error: bytes=%d\n", bytes);
	return -1;
    }
    return 0;
}

int udp_query_process(int sockfd)
{

    struct sockaddr_in client_addr;
    socklen_t addrLen;
    char query_buffer[NS_MAXMSG];

    int queryLen; 

    memset(query_buffer, 0, sizeof(query_buffer));
    addrLen=sizeof(client_addr);

    queryLen = recvfrom(sockfd, query_buffer, NS_MAXMSG, 0,
                                       (struct sockaddr * )&client_addr, &addrLen);

    //Add the query to the query list
    Query *ptrQuery = query_new( &client_addr, sockfd, query_buffer, queryLen);
 
    if (ptrQuery == NULL )  
    {
        my_log("Error: Can not allocate memory for Query\n");
        return -1;
    }
     
    ptrQuery->from = UDP;
    int index = querylist_add(&queries, ptrQuery);
    notify_dispatcher(index);

    return 0;
} 

int tcp_query_process(int sockfd)
{
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(struct sockaddr_in);

    int clientSock = accept(sockfd, (SA*) &client_addr, &length); 

/*
//<<<<<<< HEAD
    struct linger ling;
    ling.l_onoff = 1;          
    ling.l_linger = 0; //TIMEOUT;
    setsockopt(clientSock, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
//=======
//>>>>>>> f82c24e940bbdaa4be0bfabfbdae69810168a68c
*/
    if( clientSock < 0)
    {
        my_log("Error on accept TCP connection from client\n");
        return -1;
    }

    //Close socket right after close(), to  prevent TCP trap into TIME_WAIT state
    struct linger ling;
    ling.l_onoff = 1;          
    ling.l_linger = 1; //TIMEOUT;
    //setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
    setsockopt(clientSock, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));

    unsigned short lenRequest,bytes;
    bytes = readn(clientSock, &lenRequest, 2);
    if( bytes <2)
    {
        my_log("Error on reading length of request over TCP from client\n");
        return -1;
    }
    lenRequest = ntohs(lenRequest);

    char query_buffer[NS_MAXMSG];

    bytes = readn(clientSock, query_buffer, lenRequest);
    if(bytes < lenRequest)
    {
        my_log("Error on reading DNS message over TCP from client\n");
        return -1;
    }

    //Add the query to the query list
    Query *ptrQuery = query_new( &client_addr, clientSock, query_buffer, lenRequest);
 
    if (ptrQuery == NULL )  
    {
        my_log("Error: Cannot allocate memory for Query in TCP \n");
        return -1;
    }
     
    ptrQuery->from = TCP;

    int index = querylist_add(&queries, ptrQuery);
    notify_dispatcher(index);

    return 0;
    
}

int forward_query_process(int sockfd)
{
    int num, bytes;
    struct sockaddr_in src_addr;
    socklen_t len = sizeof(src_addr);
    char *strProto;

    //read the index number in the query list from dispatcher
    bytes = recvfrom( sockfd, &num, sizeof(num), 0, (SA*) &src_addr, &len); 

    if ( num <0 || num >=  MAX_QUERY_NUM)
    {
        my_log("ERROR: Query num index error\n"); 
        return -1; 
    }
    pthread_mutex_lock(&query_mutex[num]);
    Query * qr = queries.queries[num];
    if( qr == NULL)
    {
        my_log("Wierd: No query found  queries[%d]\n ", num);
    	pthread_mutex_unlock(&query_mutex[num]);
    	debug("Forward Query: Lock on %d released\n", num);
        return -1;
    }
    strProto = ((qr->from == TCP) ? "TCP":"UDP"); 
    my_log("Got %s query from:%s for: %s, len:%d, set to queries[%d]\n", 
            strProto, qr->str_client_addr, qr->qname,qr->queryLen, num); 

    Resolver *res = qr->resolver;
    //TODO : multiple resolvers

    //modify query: change the txid to res->current_txid
    u_int16_t new_id = htons(res->current_txid);
    memcpy(qr->query, &new_id, 2);

    bytes = sendto(res->sockfd, qr->query, qr->queryLen, 0,
                   (struct sockaddr *) &(res->server_addr), sizeof(struct sockaddr_in));

    queries.id_mapping[res->sockfd][res->current_txid] = num;
    qr->status = forwarded; 
    qr->new_txid = res->current_txid;

    my_log("Sent UDP Query to:%s, sockfd:%d, new_txid:%d, length:%d bytes\n",
            res->name, res->sockfd,  res->current_txid, bytes);
    
    res->current_txid = (res->current_txid +1) & 0xFFFF; // mod 65536

    if (res->current_txid == 0) // I don't like a zero id.
            res->current_txid =1;
    
    pthread_mutex_unlock(&query_mutex[num]);
    debug("Forward Query: Lock on %d released\n", num);
    return 0;
}


int reply_process(int sockfd, int udpServiceFd, int tcpServiceFd)
{
    struct sockaddr_in server_addr;
    socklen_t addrLen;
    u_char response_buffer[NS_MAXMSG];
    char pStr[MAX_WORD];
    //ns_msg handle;

    int responseLen; 
    memset(response_buffer, 0, sizeof(response_buffer));
    addrLen=sizeof(server_addr);
    responseLen = recvfrom(sockfd, response_buffer, NS_MAXMSG, 0,
                                       (struct sockaddr * )&server_addr, &addrLen);
    if (responseLen <=0 )
    {
        my_log("Error: Recvfrom resolver error in udp_reply_process\n");
        return -1;
    }

    sock_ntop((struct sockaddr *)&server_addr, sizeof(server_addr),pStr, sizeof(pStr));
    my_log("Got UDP Reply from: %s\n", pStr);

    u_int16_t id;
    memcpy( &id,response_buffer,2);
    id=ntohs(id);

    int idx = queries.id_mapping[sockfd][id];

    if( idx == -1 || idx >= MAX_QUERY_NUM ) 
    {
        my_log("Error: Index error, id_mapping[%d][%x] = %d\n", sockfd, id, idx); 
        return -1;
    }

    pthread_mutex_lock(&query_mutex[idx]);

    Query * qr = queries.queries[idx];
    if( qr == NULL) // No query exists for this reply. 
    {
        my_log("Warning: no query for this response, txid=%d\n", id);
        pthread_mutex_unlock(&query_mutex[idx]);
    	debug("Lock on %d released\n", idx);
       return -1; 
    }

    u_int16_t old_txid = qr->old_txid;
    old_txid = htons(old_txid);

    //modify the response, and sendto to the right client_address;
    memcpy(response_buffer, &old_txid, 2);

    if(qr->from == UDP)
    {
        int bytes = sendto( udpServiceFd, response_buffer, responseLen, 0,
                       (SA *)&(qr->client_addr), sizeof(struct sockaddr_in) );
        if(bytes <0)
        {
            my_log("Error: Error on send reply to client , errno=%d, message:%s\n",
                    errno, strerror(errno));
            pthread_mutex_unlock(&query_mutex[idx]);
    	    debug("Lock on %d released\n", idx);
            return -1; 
        }
        my_log("Sent UDP reply to client: %s for: %s , txid(c):%d, txid(s):%d, queries[%d].\n",
                qr->str_client_addr, qr->qname, old_txid, id, idx);
                
    }
    else if( qr->from == TCP)
    {
        //write 2 bytes of the length of the response
        unsigned short lenReply = htons(responseLen); 
        int bytes = writen( qr->sockfd, &lenReply, 2); 
        if(bytes != 2) {
            my_log("Error: write length of TCP response error\n");
                    close(qr->sockfd);
            pthread_mutex_unlock(&query_mutex[idx]);
    	    debug("Lock on %d released\n", idx);
            return -1;
        }
        bytes = writen(qr->sockfd, response_buffer, responseLen); 
        if(bytes <0) {
            my_log("Error: write TCP response error, client must have closed its socket.\n");
            close(qr->sockfd);
            pthread_mutex_unlock(&query_mutex[idx]);
    	    debug("Lock on %d released\n", idx);
            return -1;
        }
        close(qr->sockfd);
        my_log("Sent TCP reply to client: %s for: %s , txid(c):%d, txid(s):%d, queries[%d]\n",
                qr->str_client_addr, qr->qname, old_txid, id, idx);
    }
    query_free(qr);
    queries.queries[idx]=NULL;
    pthread_mutex_unlock(&query_mutex[idx]);
    debug("Lock on %d released\n", idx);
    //querylist_free_item(&queries, idx);
    
    return 0;
} 

