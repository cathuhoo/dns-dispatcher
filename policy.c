#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "common.h"
#include "list.h"
#include "resolvers.h"
#include "policy.h"
#include "mystring.h"
#include "ip_prefix.h"


/*
int  rule_load_source(Rule * r, IPPrefix *prefix)
{

}
*/
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
        rule_display( policy->rules[i]);
    }
}

int action_parse( Action * act, char * str, List * resolvers)
{
    if ( act == NULL || str == NULL)
        return -1;
    char *token, *tofree, * str2, *op, *resolver_name;
    tofree = str2 = strdup(str);
    token = strsep(&str2, DELIM2);
    if (token == NULL)
       return -1;
    else
    {
        op=strtrim(token);
        if ( 0 ==strcasecmp(op, "Drop") ) 
        {
            act->op = Drop;
            return 0;
        }
        if ( 0 ==strcasecmp(op, "Refuse") ) 
        {
            act->op = Refuse;
            return 0;
        }
        if ( 0 ==strcasecmp(op, "Forward") ) 
        {
            act->op = Forward;
            token = strsep( &str2, DELIM2);
            resolver_name=strtrim(token);
            Resolver * res = list_lookup(resolvers, resolver_name );
            if (res == NULL)
            {
                fprintf(stdout, "ERROR: No resolvers found for this rule:%s \n", str);
                return -1;
            }
            act->resolver = res;
            return 0;
        }
        else
        {
            fprintf(stdout, "ERROR: Unknow Operation %s \n", op);
            return -1;
        }
    }
    return -1;
}
Rule * rule_parse(char * line , List * resolvers)
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

int policy_load( char * policy_file, Policy * policy, List *resolvers)
{
    FILE * fp;
    Rule * rule;
    int num =0;
    char line[MAX_LINE], buffer[MAX_LINE];

    //list_init( policy, free, (void *) rule_display, NULL);
    if (policy == NULL)
        return -1;
    policy->size = 0;


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

        #ifdef DEBUG 
            fprintf(stdout, "parsing rule :%s\n" , line);
        #endif
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
    return 0;

}
int policy_load_ipprefix( Policy * policy, IPPrefix * ip_prefix)
{
    int i, count;
    int wildcards[MAX_RULES] ; // process those rules with a "*" as  ip_prefix
    //RuleSet ruleset;


    if (policy == NULL || ip_prefix == NULL)
        return -1;

    for ( i= 0; i < MAX_RULES; i ++)
        wildcards[i] = -1;
    count=0;

    /*
    for ( i=0; i < policy->size; i++)
    {
        if ( strcmp( "*", policy->rules[i]->src ) == 0 )
        {
           wildcards[count] = i;  
           count ++;
        }
    }
    */
    for ( i=0; i < policy->size; i++)
    {
        if ( strcmp( "*", policy->rules[i]->src ) == 0 )
        {
           wildcards[count] = i;  
           count ++;
        }
        else 
        {
            int rcode = prefix_load( policy->rules[i]->src, ip_prefix, i);
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
        prefix_setall(ip_prefix ,  wildcards[i]);
    }
    return 0;
}

int main (int argc, char * argv[])
{
    List resolvers;
    Policy policy;
    Resolver *res;
    IPPrefix ip_prefix;
    long addr_h;
    RuleSet *rs;
    char *ipaddress;

    resolver_load("resolvers.txt", &resolvers);
    list_travel(&resolvers);
    res= list_lookup(&resolvers, "ccert");
    resolvers.display(res); 

    policy_load( "policy.txt", &policy, &resolvers);
    policy_travel( &policy);

    prefix_init(&ip_prefix);

    policy_load_ipprefix( &policy, &ip_prefix); 

    //policy_load_domain( &policy, &dn_sufix);

    ipaddress=argv[1];
    addr_h = inet_ptoh(ipaddress, NULL);
    addr_h &= 0xFFFFFFFF;

    printf("inet_ptoh %s addr_h=0x%lx\n", ipaddress, addr_h);

    rs = prefix_lookup( &ip_prefix, &addr_h);
    printf("Lookup for %s rs=0x%lx\n", ipaddress, *rs);

    prefix_free(&ip_prefix);
    //IPPrefix 
    return 0;
}
