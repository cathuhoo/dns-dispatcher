#include <stdio.h>
#include <stdlib.h>

#include "query.h"
#include "common.h"

Query * query_new (struct sockaddr_in *cli_addr, unsigned int sockfd, void * query_buffer)
{
    Query *pt;
    if (  NULL == (pt = malloc(sizeof(Query))) )
    {
        error_report("Error on malloc for Query\n");
        return NULL;
    }
    memset(pt, 0, sizeof(Query));
    pt->status = nonprocessed;

    return pt;
}

int querylist_add(QueryList *ql, Query *query)
{
    if ( ql== NULL || query == NULL)
        return -1;

    /*Query * qlist = ql->queries;
    unsigned int i=ql->cur;
    */
    while (ql->queries[ql->cur] != NULL)
    { 
        ql->cur =(ql->cur + 1) % MAX_QUERY_NUM;
    }
    ql->queries[ql->cur] = query; 
    return ql->cur; 
}

int query_free(Query *pt)
{
    if (pt == NULL )
        return -1 ;

    if (pt->qname )
    {
        free(pt->qname);
    }
    
    if(pt->query)
    {
        free(pt->query);
    }
    if (pt->reply)
    {
        free(pt->reply);
    }

    free(pt);
    return 0;
}



