#ifndef __DNS_QUERY_H__
#define __DNS_QUERY_H__

#include <netinet/in.h>

#define QR Query_Record
#define MAX_QUERY_ID 65535

#define MAX_QUERY_NUM 2*65536

typedef enum _process_flag {
    nonprocessed  =1,
    dispatched    =2,
    forwarded     =3,
    answered      =4,
    replied       =5
} Status;

typedef struct _query{

    unsigned int  old_txid;
    char          *qname;
    struct        sockaddr_in client_addr;
    void          *query;
    unsigned long time_query;

    int           sockfd;

    unsigned int  new_txid;
    Status        status; // not precess

    void          *reply;


} Query;

typedef struct _query_array{
    int cur;
    Query   queries[MAX_QUERY_NUM];
} QueryList;

typedef struct _forwarded{
    unsigned int index; // a pointer to query index;
} ForwardedIndex; 

//Query * query_new();

#endif
