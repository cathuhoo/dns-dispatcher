#include "common.h"
#include "query.h"

#include "external.h"

int query_id_mapping_alloc(QueryList *ql, int min, int max)
{
    if( NULL ==( ql->id_mapping  =  (int **) malloc(sizeof(int *) * (max+1))))
    {
        fprintf(stderr, "Error on malloc for id_mapping 1\n");
        return -1;
    }
    int i, j;
    for (i = min; i <= max; i ++)
    {
        if( NULL == ( ql->id_mapping[i] = (int *) malloc( sizeof(int) * 65536)))
        {
            fprintf(stderr, "Error on malloc for id_mapping 2\n");
            return -1;
        }
        for (j=0; j< 65536; j++)
        {
            ql->id_mapping[i][j] = -1;
        }
    }
    
    ql->min_fd = min;
    ql->max_fd = max;

    return 0;
}
void query_id_mapping_free(QueryList *ql)
{
    int i;
    for ( i = ql->min_fd ; i <= ql->max_fd; i++)
    {
        if (ql->id_mapping[i] != NULL)
        {
            free(ql->id_mapping[i]);     
        }
    } 
    free( ql->id_mapping);
}

Query * query_new (struct sockaddr_in *cli_addr, unsigned int sockfd, void * query_buffer, int queryLen)
{
    Query *pt;
    if (  NULL == (pt = malloc(sizeof(Query))) )
    {
        my_log("Error on malloc for new Query\n");
        return NULL;
    }
    memset(pt, 0, sizeof(Query));
    pt->status = nonprocessed;
    memcpy(&(pt->client_addr), cli_addr, sizeof(struct sockaddr_in));

    pt->sockfd = sockfd;
    if( NULL == (pt->query = malloc(queryLen)))
    {
        my_log("Error: Cannot allocate memory for query_buffer\n");
        free(pt);
        return NULL;
    }
    memcpy(pt->query, query_buffer, queryLen);
    pt->queryLen = queryLen;

    return pt;
}

int querylist_add(QueryList *ql, Query *query)
{
    if ( ql== NULL || query == NULL)
        return -1;

    /*Query * qlist = ql->queries;
    unsigned int i=ql->cur;
    */
    int i = ql->cur;

    while (ql->queries[i] != NULL)
    { 
        unsigned long time_now = getMillisecond();  
        unsigned delta = time_now - ql->queries[i]->time_query;

        if(delta >QUERY_TIMEOUT) 
        {
            my_log("Queries[%d](%s) timeouts, deleted \n", i, ql->queries[i]->qname);
            query_free(ql->queries[i]);
            break;
        }
        else
        { 
            i =(i+1) % MAX_QUERY_NUM;
        }
    }

    ql->queries[i] = query;
    ql->cur = (i+1) % MAX_QUERY_NUM; 
    return i; 
}

Query * querylist_lookup_byIndex(QueryList *ql, unsigned int index)
{
    return ql->queries[index];
}

int query_free(Query *pt)
{
    if (pt == NULL )
        return -1 ;

    if (pt->qname )
    {
        free(pt->qname);
    }

    if(pt->from == TCP )
        close(pt->sockfd);
    
    if(pt->query)
    {
        free(pt->query);
    }

    free(pt);
    return 0;
}
void querylist_free(QueryList *ql)
{
    int i;
    for (i=0; i< MAX_QUERY_NUM; i++)
    {
        if( ql->queries[i] != NULL )
            query_free(ql->queries[i]);
    }
    if(ql->id_mapping != NULL) 
        query_id_mapping_free(ql);
}
int querylist_init( QueryList *ql)
{
    int i;
    if (ql == NULL)
        return -1;
    ql->cur = 0;
    for (i=0; i< MAX_QUERY_NUM; i++)
    {
        ql->queries[i] = NULL;
    }
    ql->id_mapping = NULL;
    ql->min_fd = 0;
    ql->max_fd = 0;
    
    return 0;
}

int query_parse(Query * q)
{
    int n;

    if (q == NULL )
        return -1;
    if( q->query == NULL)
        return -1;

    ns_msg handle;  

    memset(&handle, 0, sizeof(handle));

    if (ns_initparse(q->query, q->queryLen, &handle) < 0)
    {
        error_report("E: parse query error on ns_initparse: %s, queryLen:%d\n", strerror(errno),q->queryLen);
        return -1;
    }

    if((n=ns_msg_count(handle, ns_s_qd))<=0)
    {
        error_report("E: Weird Packet:No Question? queryLen:%d\n", q->queryLen);
        return -1;
    }

    ns_rr rr;
    if ( ns_parserr(&handle, ns_s_qd,0, &rr) <0 )
    {
        error_report("E: Parse Question error.\n");
        return -1;
    }

    u_int16_t id = ns_msg_id(handle);

    char *name = ns_rr_name(rr);
    int len = strlen(name);

/*
    char strAddr[MAX_WORD];
    sock_ntop((SA*) &(q->client_addr), sizeof(SA),  strAddr,sizeof(strAddr));
    //int srcPort = ntohs(q->client_addr.sin_port);
    debug("Q: from: %s name:%s(len=%d) id:%d len:%d\n", strAddr, name,len, id, q->queryLen  );
*/    
    if ( NULL == (q->qname = malloc( len+1 )))
    {
        my_log("Error: malloc for qname:%s error.\n", name);
        return -1;
    }
    strncpy(q->qname, name, len);
    q->qname[len] = '\0';
    q->old_txid = id;
    q->time_query=getMillisecond();
    sock_ntop((SA*) &(q->client_addr), sizeof(SA), q->str_client_addr, sizeof(q->str_client_addr));
  
    return 0;
}



