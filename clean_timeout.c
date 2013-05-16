#include "common.h"
#include "external.h"



//static void * clean_timeout_handler( NULL);

static void * clean_timeout_handler()
{
    while (! parentRequestStop )
    {
        int i;
        sleep(30);
        debug("clean timeout thread is working...\n");
        for ( i=0; i< MAX_QUERY_NUM; i++)
        {
            Query * qr = queries.queries[i]; 
            if (qr == NULL) 
                continue;
            unsigned long time_now = getMillisecond();
            unsigned      delta = time_now - qr->time_query;
            if(delta >QUERY_TIMEOUT)
            {
                my_log("Queries[%d](%s) timeouts, free by clean thread\n", i, qr->qname);
                querylist_free_item(&queries, i);
            }
        }
    } 
    debug(" clean thread ready to exit\n");
    pthread_exit(NULL);
}

pthread_t clean_timeout()
{
    pthread_t tid;
    if ( 0 != pthread_create(&tid, NULL, clean_timeout_handler, NULL ))
    {
        my_log("Error: Cannot create thread for clean_timeout\n");
        return 0;
    }
    return  tid;
}
