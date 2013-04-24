
#ifndef  __DNS_RECV_SEND_HEAD_H__
#define  __DNS_RECV_SEND_HEAD_H__

#include "policy.h"
#include "resolvers.h"
#include "config.h"

int recv_and_send(Policy * policy, List *resolvers, Configuration *config);

#endif
