
#ifndef  __DNS_RECV_SEND_HEAD_H__
#define  __DNS_RECV_SEND_HEAD_H__

#include "policy.h"
#include "resolvers.h"
#include "config.h"

pthread_t listener(char *names[], int num); //Policy * policy, List *resolvers, Configuration *config);

void * listen_thread_handler(void * arg);

#endif