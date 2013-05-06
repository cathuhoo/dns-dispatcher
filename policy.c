#include "common.h"
#include "mystring.h"
//#include "list.h"
#include "resolvers.h"
#include "ip_prefix.h"
#include "trie.h"
#include "policy.h"
#include "external.h"

int rule_no(RuleSet set)
{
    int i;

    for ( i = 0; i< MAX_RULES; i ++)
    {
        if ( set & (1<<i) ) 
          return i;    
    }
    return -1;
}
void rule_display( Rule * r)
{
    if(r ==NULL)
    {
        fprintf(stdout, "Error: NULL resolver\n");
        return;
    }
    printf("Source:%s |", r->src);
    printf("Destination:%s |", r->dst);
    printf("Action: %d ", r->action.op);
    if (r->action.resolver) 
        printf(" to : %s ", r->action.resolver->name);
    printf("\n");
}

void policy_travel (Policy * policy)
{
    int i;
    if (policy == NULL)
        return ;
    for ( i=0; i < policy->size; i++)
    {
        printf("Rule[%d]:", i);
        rule_display( policy->rules[i]);
    }
}

int action_parse( Action * act, char * str, ResolverList * resolvers)
{
    if ( act == NULL || str == NULL)
        return -1;
    char *token, *tofree, * str2, *op, *resolver_name;
    tofree = str2 = strdup(str);
    token = strsep(&str2, DELIM2);
    if (token == NULL)
    {
        free(tofree); 
        return -1;
    }

    op=strtrim(token);

    if ( 0 ==strcasecmp(op, "Drop") ) 
    {
        free(tofree); 
        act->op = Drop;
        return 0;
    }
    else if ( 0 ==strcasecmp(op, "Refuse") ) 
    {
        free(tofree); 
        act->op = Refuse;
        return 0;
    }
    else if ( 0 ==strcasecmp(op, "Forward") ) 
    {
        act->op = Forward;
        token = strsep( &str2, DELIM2);
        resolver_name=strtrim(token);
        Resolver * res = resolver_list_lookup(resolvers, resolver_name );
        if (res == NULL)
        {
            free(tofree); 
            fprintf(stdout, "ERROR: No resolvers found for this rule:%s \n", str);
            return -1;
        }
        act->resolver = res;
        free(tofree); 
        return 0;
    }
    else
    {
        free(tofree); 
        fprintf(stdout, "ERROR: Unknow Operation %s \n", op);
        return -1;
    }
    
    return -1;
}
Rule * rule_parse(char * line , ResolverList * resolvers)
{
    Rule * pr;
    char *tofree, *token, *str, buffer[MAX_WORD];
    int i=0, result; //, num;

    if ( line ==NULL)
        return NULL;

    if( NULL == (pr=malloc(sizeof(Rule))) )
    {
        fprintf(stdout, "Malloc Error in parse rule\n");
        return NULL;
    }
    tofree = str = strdup(line);
    /* sample: 
      blacklist_ip.txt| * | Drop
      *| blacklist_domain.txt| Forward:ccert
      tsinghua_ip.txt| video.txt| F:telecom
      cernet_ip.txt| video.txt| F:unicom
      *| * | Drop
     */
    //Set default

    while ( ( token = strsep(&str, DELIM)) != NULL)
    {
        strtrim2(buffer, MAX_WORD,token);
        switch (i)
        {
            case 0:
                strcpy(pr->src, buffer);
                break;
            case 1:
                strcpy(pr->dst, buffer);
                break;
            case 2 :
                result = action_parse(&(pr->action), buffer, resolvers);
                if (result )
                {
                    fprintf(stdout, "ERROR: Rule syntax error, unknow action: %s \n", buffer);
                    free(pr);
                    free(tofree);
                    return NULL;
                }
                break;
             default :
                fprintf(stdout, "ERROR: Rule syntax error, at line %s \n", buffer);
                free(pr);
                free(tofree);
                return NULL ;
        }
       i++; 
    }

    free(tofree);
    return  pr;
}

int policy_free(Policy *policy)
{
    int i;
    for (i= 0; i< policy->size; i++)
    {
        if(policy->rules[i]) 
            free(policy->rules[i]);
    }
    prefix_free(&policy->ip_prefix);
    debug("trie_free\n");
    trie_free(policy->trie_dn);
    return 0;
}
int policy_load( char * policy_file, Policy * policy, ResolverList *resolvers)
{
    FILE * fp;
    Rule * rule;
    int num =0, i;
    char line[MAX_LINE], buffer[MAX_LINE];

    //list_init( policy, free, (void *) rule_display, NULL);
    if (policy == NULL)
        return -1;

    policy->size = 0;
    for ( i=0; i< MAX_RULES; i++)
        policy->rules[i] = NULL;


    if( (fp= fopen(policy_file,"r")) == NULL)
    {
        fprintf(stdout, "ERROR: Cannot open policy file:%s to read.\n", policy_file);
        return -1;
    }

    while ( fgets(buffer, MAX_LINE, fp)) 
    {
        num ++;
        strtrim2(line, MAX_LINE, buffer);

        if( (line[0] == '#') || (line[0] == ';') || (line[0]=='\n') )
             continue;

        if ( NULL == (rule = rule_parse(line, resolvers)))
        {
           fprintf(stdout, "ERROR: Parse policy file error, line %d: %s \n",
                   num, line);
           return -1;
        }
        //list_ins_next(policy, policy->tail, rule);
        policy->rules[ policy->size ] = rule;
        policy->size ++;
     }
    fclose(fp);

    prefix_init(&policy->ip_prefix);
    policy_load_ipprefix(policy) ; //, &policy->ip_prefix);

    policy->trie_dn = TrieInit();
    policy_load_domain(policy) ; //, policy->trie_dn);
    return 0;

}
int policy_load_ipprefix( Policy * policy)//, IPPrefix * ip_prefix)
{
    int i, count;
    int wildcards[MAX_RULES] ; // process those rules with a "*" as  ip_prefix

    if (policy == NULL) // || ip_prefix == NULL)
        return -1;
    prefix_init( &(policy->ip_prefix));

    for ( i= 0; i < MAX_RULES; i ++)
        wildcards[i] = -1;
    count=0;

    for ( i=0; i < policy->size; i++)
    {
        if ( strcmp( "*", policy->rules[i]->src ) == 0 )
        {
           wildcards[count] = i;  
           count ++;
        }
        else 
        {
            int rcode = prefix_load( policy->rules[i]->src, &(policy->ip_prefix), i);
            if (rcode == -1)
            {
                fprintf(stdout, "ERROR: policy_load_ipprefix, rule #%d\n", i);
                return -1;
            }
        }
    }
    
    //add those wildcard (*)
    for (i=0; i < count ; i++)
    {
        prefix_setall(&(policy->ip_prefix),  wildcards[i]);
    }
    return 0;
}
int policy_load_domain( Policy * policy) //, trieNode_t * trie)
{
    int i, count;
    int wildcards[MAX_RULES] ; // process those rules with a "*" as  ip_prefix

    if (policy == NULL) // || trie == NULL)
        return -1;

    policy->trie_dn = TrieInit();

    for ( i= 0; i < MAX_RULES; i ++)
        wildcards[i] = -1;

    count=0;

    for ( i=0; i < policy->size; i++)
    {
        if ( strcmp( "*", policy->rules[i]->dst ) == 0 )
        {
           wildcards[count] = i;  
           count ++;
        }
        else 
        {
            int rcode = TrieLoad(policy->trie_dn, policy->rules[i]->dst, i);
            if (rcode == -1)
            {
                fprintf(stdout, "ERROR: policy_load_domain, rule #%d\n", i);
                return -1;
            }
	    printf("After rule[%d], the Trie looks like:\n", i);
	    TrieTravelE(policy->trie_dn);
        }
    }
    
    //add those wildcard (*)
    RuleSet set=0;
    for (i=0; i < count ; i++)
    {
        set = set | ( 1<< wildcards[i]);
    }
    if(set) 
    {
        trie_setall(policy->trie_dn , set );
        TrieAdd(&policy->trie_dn, "*", set); 
    }
    return 0;
}

Action* policy_lookup(Policy * policy, long addr_h, char * domain_name  )
{
    RuleSet *prs;
    RuleSet rs2;
    prs = prefix_lookup(&policy->ip_prefix, &addr_h);
    if( prs == NULL)
    {
        fprintf(stdout, "Domain Name:%s not found in trie\n", domain_name);
        return NULL;
    }
    #ifdef DEBUG
        long addr_n=htonl(addr_h);
        char ipstr[MAX_WORD];
        inet_ntop( AF_INET, &addr_n, ipstr, sizeof(ipstr));
        fprintf(stdout, "IP:%s found in prefix, value:0x%lx\n" ,ipstr, *prs);
    #endif

    trieVal_t * srch=NULL;
    srch = trie_search (policy->trie_dn, domain_name); 
    if( srch == NULL)
    {
        fprintf(stdout, "Domain Name:%s not found in trie\n", domain_name);
        return NULL; 
    }
    else
    {
       rs2=*srch;
       //fprintf(stdout, "Domain Name:%s  found in trie, value:0x%lx\n", domain_name, rs2);
        debug("domain :%s found in trie, value:0x%lx\n" ,domain_name, rs2);
    }

    int num = rule_no( *prs & rs2);

    //return num;
    if (num < policy->size)
    {
        debug("Policy matched rule #%d\n", num);
        Action *pa = &policy->rules[num]->action;
        return pa;
    }
    return NULL;
}    
/*
int main (int argc, char * argv[])
{
    List resolvers;
    Policy policy;
    Resolver *res;
    IPPrefix ip_prefix;
    long addr_h;
    RuleSet *rs=NULL;
    char *ipaddress;

    if(argc <2) 
    {
        fprintf(stderr, "Usage: %s <ip_address> [domain_name]\n", argv[0]);
        exit(-1);
    }

    //Load resolvers
    resolver_load("resolvers.txt", &resolvers);
    list_travel(&resolvers);

    //test : lookup a resolver
    res= list_lookup(&resolvers, "ccert");
    resolvers.display(res); 

    //Load Policy
    policy_load( "policy.txt", &policy, &resolvers);
    policy_travel( &policy);

    //load IP prefix in the policy
    prefix_init(&ip_prefix);
    policy_load_ipprefix( &policy, &ip_prefix); 

    // To lookup some IP address :
    ipaddress=argv[1];
    addr_h = inet_ptoh(ipaddress, NULL);
    addr_h &= 0xFFFFFFFF;


    rs = prefix_lookup( &ip_prefix, &addr_h);
    printf("Lookup for %s rs=0x%lx\n", ipaddress, *rs);

    //////////////////////////////////////////
    //Load Domain in the policy
    trieNode_t * srch=NULL;
    trieNode_t * trie_dn = TrieInit();
    policy_load_domain(&policy, trie_dn);

    printf("Trie Travel:\n");
    TrieTravelE(trie_dn);

    // To lookup some domain names: 
    RuleSet rs2=0;
    if ( argc >=3 )
    {
        srch = trie_search (trie_dn, argv[2]); ///???????????????????????????
        if( srch == NULL)
        {
            fprintf(stdout, "Domain Name:%s not found in trie\n", argv[2]);
        }
        else
        {
            rs2=srch->value;
            fprintf(stdout, "Domain Name:%s  found in trie, value:0x%lx\n", argv[2], rs2);
        }
        
    }
    if (rs!= NULL && rs2 != 0 ) 
    {
        RuleSet rsu =  (*rs) & rs2;
        int ruleNum = rule_no(rsu); 
        fprintf(stdout, "set in ip prefix:0x%lx ;  set in domain:0x%lx , matched rule number:%d\n",
                *rs, rs2, ruleNum) ;
    }

    prefix_free(&ip_prefix);
    trie_free(trie_dn);
    return 0;
}
*/
