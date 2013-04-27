#include <pthread.h>

#include "common.h"
#include "sender.h"
#include "external.h"


void * sender_thread_handler()
{
    debug("sender thread is running..\n");
    pthread_t tid_me = pthread_self();

    #ifdef DEBUG
        for (;;)
        {
            sleep(TIME_SLEEP);
            debug("sender thread[%lu] is running..\n", (unsigned long)tid_me);
        }
    #endif

}


pthread_t sender()
{
    debug("Sender Begin\n");
    
    pthread_t tid;
    if ( 0 != pthread_create(&tid, NULL, &sender_thread_handler, NULL))
    {
        error_report("Cannot create thread for sender\n");
        return 0;
    }
    return tid;
}
