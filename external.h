
#ifndef __EXTERNAL_H__
#define __EXTERNAL_H__

#include "config.h"
#include "resolvers.h"
#include "policy.h"
#include "query.h"
#include "dispatcher.h"

extern Configuration config;
extern ResolverList resolvers;
extern Policy policy;
extern QueryList queries;
//char * un_names[MAX_RESOLVERS];
extern Disp_info *disp_addr;
//extern unsigned short id_mapping[MAX_SOCKFD][65536];
//extern unsigned short ** id_mapping; //[MAX_SOCKFD][65536];


#endif

