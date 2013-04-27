
#include "common.h"
#include "dispatcher.h"

#include "external.h"


void * dispatcher_thread_handler()
{
    debug("dispatcher thread is running..\n");

    pthread_t tid_me = pthread_self();
 
    for (;;)
    {
        sleep(TIME_SLEEP);
        debug("dispatcher thread[%lu] is running..\n", (unsigned long)tid_me);
    }  
}


pthread_t dispatcher()
{
    debug("Dispatcher Begin\n");
    
    pthread_t tid;
    if ( 0 != pthread_create(&tid, NULL, &dispatcher_thread_handler, NULL))
    {
        error_report("Cannot create thread for dispatcher\n");
        return 0;
    }
    return  tid;
}
