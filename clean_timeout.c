#include "common.h"
#include "external.h"



//static void * clean_timeout_handler( NULL);

static void * clean_timeout_handler()
{
    while (! parentRequestStop )
    {
        int i;
	sleep(10);
	while(parentRequestPause){};
        debug("clean timeout thread is working...\n");
        for ( i=0; i< MAX_QUERY_NUM; i++)
        {
            //lock
            //debug("TO lock queries[%d]\n",i);
            pthread_mutex_lock(&query_mutex[i]);
	    if( queries.queries[i] != NULL)
	    {
                Query * qr = queries.queries[i]; 
            	/*if (qr == NULL) 
		{
            	   pthread_mutex_unlock(&query_mutex[i]);
                   continue;
		}
		*/
                unsigned long time_now = getMillisecond();
                unsigned long delta = time_now - qr->time_query;
                if(delta >QUERY_TIMEOUT)
                {
                    //my_log("CLEAN_TIMEOUT: Queries[%d](%s) timeouts:%ld, free by clean thread\n",
                    //        i, qr->qname,delta);
                    //querylist_free_item(&queries, i);
                    query_free(qr);
                    queries.queries[i] = NULL;
                }
	    }
            //unlock
            pthread_mutex_unlock(&query_mutex[i]);
            //debug(" unlocked queries[%d]\n",i);
        }
    } 
    debug("clean thread ready to exit\n");
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
