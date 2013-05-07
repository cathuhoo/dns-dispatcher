
#ifndef  __DNS_RECV_SEND_HEAD_H__
#define  __DNS_RECV_SEND_HEAD_H__

#include "common.h"
#include "query.h"
#include "resolvers.h"
#include "list.h"
#include "policy.h"
#include "config.h"
#include "external.h"


pthread_t recv_send(); //char *names[], int num); //Policy * policy, List *resolvers, Configuration *config);

static void * listen_thread_handler(void * arg);

int tcp_query_process(int sockfd);

int udp_query_process(int sockfd);

int reply_process(int sockfd, int udpServiceFd, int tcpServiceFd);

int forward_query_process(int sockfd);

#endif
