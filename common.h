#ifndef __COMMON__H__
#define __COMMON__H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <sys/time.h>



#define MAX_RESOLVERS 64
#define MAX_LINE 1024
#define MAX_WORD 256 
#define DELIM  "|"
#define DELIM2  ":"

#ifndef NS_MAXMSG 
#define NS_MAXMSG 4096 
#endif

#define MAX_TXID 65535
//#define MAX_SOCKFD 64


#define UDP 0 //used to identify the protocol the query comes from
#define TCP 1

#define MAX_TCP_CLIENTS 256 

#define MAX2(a,b)  a>b?a:b

#define BOOL unsigned char


#define TRUE  1
#define FALSE 0

#define PORT_DISPATCHER 61234

#define TIME_SLEEP 15 //seconds


#define TIMEOUT 5 //seconds, for select, and signal_handler 

#define QUERY_TIMEOUT 1000 //ms , for clean up old query items

#ifdef DEBUG
   #define debug(fmt, ...)  \
   { \
       fprintf(config.fd_log, "Debug: "); \
       fprintf(config.fd_log, fmt, ##__VA_ARGS__);\
       fflush(config.fd_log); \
   }   

   #define debug_point(msg) \
   {\
       fprintf(config.fd_log,"So Far So Good:%s(%d):%s\n",__FILE__, __LINE__, msg);\
       fflush(config.fd_log);\
   } 
#else
   #define debug(fmt, ...)    
   #define debug_point(fmt, ...) 
#endif

#define my_log(fmt, ...) \
    {\
        fprintf(config.fd_log, fmt, ##__VA_ARGS__) ;\
        fflush(config.fd_log);  \
    }

typedef long RuleSet;

#define trieVal_t RuleSet
#define SA struct sockaddr

//Add element to set. Here element must little than length of RuleSet (for long int, 32)
#define ADDTOSET(set, element)  set = set | (1 << element) 
#define CLEARSET(s)  s = s & 0x0

ssize_t readn(int fd, void *vptr, size_t n);
ssize_t  writen(int fd, const void *vptr, size_t n);

int CreateUnixServerSocket(int addrFamily, int protocol, char * strPath, int port,  struct sockaddr * serv_addr );
int CreateClientSocket(int addr_family, char * server_address,int protocol, int server_port, SA *client_addr);
// addr_family = AF_INET| AF_LOCAL

int CreateServerSocket(int addr_family, int protocol, char * str_addr,  int port, struct sockaddr * server_addr);
// addr_family = AF_INET| AF_LOCAL
// protocol = SOCK_STREAM| SOCK_DGRAM


unsigned long getMillisecond();
int maximum(int array[], int size);
int minimum(int array[], int size);


char * sock_ntop(const struct sockaddr *sa, socklen_t salen, char * str, int sizeStr);

#define  error_report(fmt, ...)  \
    { \
        fprintf(stderr, fmt, ##__VA_ARGS__); fprintf(stderr, "%s(%d)\n",__FILE__, __LINE__); \
    }


#endif

