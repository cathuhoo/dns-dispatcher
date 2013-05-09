#ifndef __DNS_QUERY_H__
#define __DNS_QUERY_H__

#include <netinet/in.h>
#include "resolvers.h"
#include "policy.h"

#define QR Query_Record
#define MAX_QUERY_ID 65535

#define MAX_QUERY_NUM 0x200 

typedef enum _process_flag {
    nonprocessed  =0,
    dispatched    =1,
    forwarded     =2,
    answered      =3,
    replied       =4
} Status;

typedef struct _query{

    u_int16_t  old_txid;
    char          *qname;
    struct        sockaddr_in client_addr;
    void          *query;
    unsigned short   queryLen;
    unsigned long time_query;
    unsigned long time_forward;

    Resolver * resolver ; // to forward to
    Operation op;

    unsigned int  sockfd; //where the query comes from
    unsigned char from;  // from TCP or UDP 

    unsigned int  new_txid;
    Status        status; // not precess
    

    //void          *reply;
    //unsigned short replyLen;


} Query;

typedef struct _query_array{
    unsigned int cur;
    Query   * queries[MAX_QUERY_NUM];
    int  ** id_mapping;
    int min_fd, max_fd;
} QueryList;

typedef struct _forwarded{
    unsigned int index; // a pointer to query index;
} ForwardedIndex; 
// sockfd, oxid, index
//

//When the listener gets a DNS query from a client, it create a Query
//The memory should be freed in the sender thread, after its reply forwarded.
Query * query_new (struct sockaddr_in *cli_addr, unsigned int sockfd, void * query_buffer, int queryLen);

//Used by listener thread. When the listener gets a DNS query from client,
//it add the query to the global varable  
//Return value is the index in the query list.( The index will be dilivered to the sender thread)
int querylist_init(QueryList *ql);

int querylist_add(QueryList *ql, Query *query);

//Used by dispatcher and sender  thread. 
//dispatcher:
//   The dispatcher gets a notification(including index) from listener,and then
//   to decide which resolver this query should be delivered to.
//sender:
//   The sender gets a notification(including index number) from listern that a reply arrives, 
//   and then forward the reply to the client
Query * querylist_lookup_byIndex(QueryList *ql, unsigned int index);

int  query_parse( Query * query);


//Used by listener thread. When the listener gets a DNS replay from upstream resolvers,
//it match the reply to the item in query list 
//Return value is the index in the query list( The index will be delivered to the sender thread)
unsigned int querylist_match( QueryList *ql, void * dns_reply, unsigned int sockfd);

//the sender use it to free the memory allocated in the listener thread(with query_new);
int query_free(Query * query);
void querylist_free(QueryList *ql);


int query_id_mapping_alloc(QueryList *ql, int min, int max);
void query_id_mapping_free(QueryList *ql);

#endif
