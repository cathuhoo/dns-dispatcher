
#ifndef __IP_PREFIX_H__
#define __IP_PREFIX_H__

#include "list.h"   //List
#include "common.h" // RuleSet

#define MAX_HASH_24 256*256*256
#define MAX_HASH_32 256*256

typedef struct _prefix {

    List     *prefix32;

    RuleSet  *prefix24;

} IPPrefix;

typedef struct _hash_item
{
    long ipaddr;
    RuleSet ruleset;
} HashItem;

int prefix_init(IPPrefix * prefix);

int prefix_add(IPPrefix *prefix, char * ipstr, int mask, int rule_no);

//RuleSet * prefix_lookup(IPPrefix *prefix, char *ipstr);
RuleSet* prefix_lookup(IPPrefix *prefix , long * addr );
int prefix_load(char * file_name, IPPrefix * prefix, int rule_no);
int prefix_setall(IPPrefix * prefix, int rule_no);
long inet_ptoh( char *ipstr, long * addr);
int prefix_free( IPPrefix *prefix);
#endif
