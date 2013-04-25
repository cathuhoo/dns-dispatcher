#include <stdio.h>
#include <stdlib.h>

#include "query.h"
#include "common.h"

Query * query_new()
{
    Query *pt;
    if (  NULL == (pt = malloc(sizeof(Query))) )
    {
        error_report("Error on malloc for Query\n");
        return NULL;
    }
    memset(pt, 0, sizeof(Query));
    return pt;
}

void query_free(Query *pt)
{
    if (pt == NULL )
        return ;

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
}



