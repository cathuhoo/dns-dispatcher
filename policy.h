#ifndef __HEAD_DNS_DISPATCHER__
#define __HEAD_DNS_DISPATCHER__

#include "resolvers.h" // Resolver struct is needed

#define MAX_RULES 32

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
    char src[MAX_WORD];
    char dst[MAX_WORD];
    Action action; 
} Rule;

typedef struct _policy{
    int size;
    Rule *rules[MAX_RULES];
} Policy;

typedef long RuleSet;

#define CLEARSET(s)  s = s & 0x0

//Add element to set. Here element must little than length of RuleSet (for long int, 32)
#define ADDTOSET(set, element)  set = set | (1 << element) 

#endif
