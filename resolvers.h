#ifndef __HEAD_DNS_RESOLVER__
#define __HEAD_DNS_RESOLVER__

#include "common.h"
//#include "list.h"

typedef struct _resolver{
        char  name[MAX_WORD];      // the name of the dns resolver
        char  ipaddress[MAX_WORD]; // IP address , IPv4 or IPv6 
        int  udp_port; // udp port
        int  tcp_port; // tcp port 
        char  isp[MAX_WORD];  // the ISP to which the DNS resolver belongs 
        int  rrt; // Round trip time
        int openflag; // is it a open resolver ? 
        //int (*connet) (int protocol );

        // assigned by listener, to keep state to the upstream resolvers
        // used by sender, to forward dns queries 
        struct sockaddr_in server_addr;
        int sockfd;

        u_int16_t current_txid;


} Resolver;

typedef struct _resolver_list{
    Resolver *resolvers[MAX_RESOLVERS];
    //int   (*match)(const void *key1, const void *key2);
    int   (*match)(char *key1,  Resolver *key2);
    void  (*display)(void *data);
    int size;
} ResolverList;

void resolver_list_init(ResolverList *rl, void (*match)( void * key1, void * key2), void (*display)(void *data)  );
int resolver_match( char * resolver_name, Resolver * res);
void resolver_display( Resolver * res);
int resolver_list_load(char * source_file, ResolverList * resolvers );
void resolver_list_free(ResolverList *rl) ;
Resolver * resolver_list_lookup(ResolverList *resolvers, char *resolver_name );
void resolver_list_travel(ResolverList *rl);

//#define resolver_free(res) list_destroy(res)
//#define resolver_travel(res) list_travel(res)
#endif
