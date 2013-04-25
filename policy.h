#ifndef __HEAD_DNS_DISPATCHER__
#define __HEAD_DNS_DISPATCHER__

#include "resolvers.h" // Resolver struct is needed
#include "ip_prefix.h" //IPPrefix struct
#include "list.h"      //List 
#include "trie.h"

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
    IPPrefix ip_prefix;
    trieNode_t * trie_dn;
} Policy;


//#define CLEARSET(s)  s = s & 0x0

//Add element to set. Here element must little than length of RuleSet (for long int, 32)
//#define ADDTOSET(set, element)  set = set | (1 << element) 

void policy_travel (Policy * policy);
int policy_free(Policy *policy);
int policy_load( char * policy_file, Policy * policy, ResolverList *resolvers);
int policy_load_ipprefix( Policy * policy); //, IPPrefix * ip_prefix);
int policy_load_domain( Policy * policy); //, trieNode_t * trie);
Action* policy_lookup(Policy * policy, long addr_h, char * domain_name  );

#endif
