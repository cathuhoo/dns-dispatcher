#ifndef __COMMON__H__
#define __COMMON__H__


#include <string.h>
#include <netinet/in.h>


#define MAX_LINE 1024
#define MAX_WORD 64
#define DELIM  "|"
#define DELIM2  ":"

#define MAX_DNS_MSG 4096
#define MAX_TCP_CLIENTS 256 

#define MAX2(a,b)  a>b?a:b

#define BOOL unsigned char


#define TRUE  1
#define FALSE 0


#ifdef DEBUG
   #define debug(fmt, ...) {fprintf(fd_log, "Debug: "); fprintf(fd_log, fmt, ##__VA_ARGS__);fflush(fd_log); }   
   #define debug_point(msg){fprintf(fd_log,"SFSG:%s(%d):%s\n",__FILE__, __LINE__, msg);fflush(fd_log);} 
#else
   #define debug(fmt, ...)    
   #define debug_point(fmt, ...) 
#endif

#define my_log(level, fmt, ...) { fprintf(fd_log, fmt, ##__VA_ARGS__) ;fflush(fd_log);  }

typedef long RuleSet;

#define trieVal_t RuleSet
//Add element to set. Here element must little than length of RuleSet (for long int, 32)
#define ADDTOSET(set, element)  set = set | (1 << element) 
#define CLEARSET(s)  s = s & 0x0

#endif
