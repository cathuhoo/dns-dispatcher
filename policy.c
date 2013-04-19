#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "list.h"
#include "policy.h"
#include "mystring.h"


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
        if ( 0 ==strncmp(op, "DROP",4) ) 
        {
            act->op = Drop;
            return 0;
        }
        if ( 0 ==strncmp(op, "Refuse",6) ) 
        {
            act->op = Refuse;
            return 0;
        }
        if ( 0 ==strncmp(op, "Forward",6) ) 
        {
            act->op = Forward;
            token = strsep( &str2, DELIM2);
            resolver_name=strtrim(token);
            Resolver * res = list_lookup( resolver_name, resolvers );
            if (res == NULL)
            {
                fprintf(stdout, "ERROR: No resolvers found for this rule:%s \n", str);
                return -1;
            }
        }
        else
        {
            fprintf(stdout, "ERROR: Unknow Operation %s \n", op);
            return -1;
        }
    }
}
Rule * rule_parse(char * line , List * resolvers)
{
    Rule * pr;
    char *tofree, *token, *str, buffer[MAX_WORD];
    int i=0, result, num;

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
                    fprintf(stdout, "ERROR: Rule syntax error, at line %d \n", num);
                    free(pr);
                    free(tofree);
                    return NULL;
                }
                break;
             default :
                fprintf(stdout, "ERROR: Rule syntax error, at line %d \n", num);
                free(pr);
                free(tofree);
                return NULL ;
        }
       i++; 
    }

    free(tofree);
    return  pr;
}

int policy_load( char * policy_file, List * policy, List *resolvers)
{
    FILE * fp;
    Rule * rule;
    int num =0;
    char line[MAX_LINE], buffer[MAX_LINE];

    list_init( policy, free, (void *) rule_display, NULL);

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
        list_ins_next(policy, policy->tail, rule);
     }

    fclose(fp);
    return 0;

}

int main (int argc, char * argv[])
{
    List resolvers, policy;

    resolver_load("resolvers", &resolvers);
    list_travel(&resolvers);

    policy_load( "policy.txt", &policy, &resolvers);
}
