#ifndef __HEAD_DNS_DISPATCHER__
#define __HEAD_DNS_DISPATCHER__

typedef struct _resolver{
    char * name;      // the name of the dns resolver
    char * ipaddress; // IP address , IPv4 or IPv6 
    int  udp_port; // udp port
    int  tcp_port; // tcp port 
    char * isp;  // the ISP to which the DNS resolver belongs 
    int rrt; // Round trip time
} Resolver;

typedef enum _operation { 
    Drop = 1, 
    Refuse = 2,
    Forward =3
} Operation;

typedef struct {
    Operation op;
    Resolver * resolver;
} Action;

typedef struct _rule{
    char *src;
    char *dst;
    Action action; 
} Rule;

typedef long RuleSet;

#define CLEARSET(s)  s = s & 0x0

//Add element to set. Here element must little than length of RuleSet (for long int, 32)
#define ADDTOSET(set, element)  set = set | (1 << element) 

#endif
