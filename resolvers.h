#ifndef __HEAD_DNS_RESOLVER__
#define __HEAD_DNS_RESOLVER__

#define MAX_LINE 1024
#define MAX_WORD 64
#define DELIM  "|"

typedef struct _resolver{
        char  name[MAX_WORD];      // the name of the dns resolver
        char  ipaddress[MAX_WORD]; // IP address , IPv4 or IPv6 
        int  udp_port; // udp port
        int  tcp_port; // tcp port 
        char  isp[MAX_WORD];  // the ISP to which the DNS resolver belongs 
        int  rrt; // Round trip time
        int openflag; // is it a open resolver ? 
} Resolver;

#endif
