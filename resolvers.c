#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#include "list.h"
#include "resolvers.h"
#include "mystring.h"


Resolver * resolver_parse(char * line)
{
    Resolver * pres;
    char *tofree, *token, *str, buffer[MAX_WORD];
    int i=0;

    if ( line ==NULL)
        return NULL;

    if( NULL == (pres=malloc(sizeof (Resolver))) )
    {
        fprintf(stdout, "Malloc Error in parse resolver\n");
        return NULL;
    }
    tofree = str = strdup(line);
    /* secret|184.22.34.216|5300|5300|burst
     * ccert|202.112.57.6:53
     * telecom|219.141.136.10
     */
    //Set default
    pres->udp_port = 53;
    pres->tcp_port = 53;

    while ( ( token = strsep(&str, DELIM)) != NULL)
    {
        strtrim2(buffer, MAX_WORD,token);
        switch (i)
        {
            case 0:
                strcpy(pres->name, buffer);
                break;
            case 1:
                strcpy(pres->ipaddress, buffer);
                break;
            case 2 :
                //pres->udp_port =atoi(buffer);
                pres->udp_port = (int)strtol(buffer, (char **)NULL, 10);
                break;
            case 3 :
                //pres->tcp_port =atoi(buffer);
                pres->tcp_port = (int)strtol(buffer, (char **)NULL, 10);
                break;
            case 4 :
                strcpy(pres->isp ,buffer);
                break;
        }
       i++; 
    }

    free(tofree);

    return pres;
}

//return 0: mismatch
//return 1: match
//return -1: Error
int resolver_match( char * resolver_name, Resolver * res)
{
    if ( resolver_name ==NULL || res == NULL) 
    {
        fprintf(stdout, "Resolver name or point is NULL\n");
        return -1;
    } 
    if ( 0 == strcasecmp(resolver_name, res->name))
        return 1;
    else
        return 0;
}
void resolver_display( Resolver * res)
{
    if(res ==NULL)
    {
        fprintf(stdout, "Error: NULL resolver\n");
        return;
    }
    printf("Name:%s | ", res->name);
    printf("IP:%s |", res->ipaddress);
    printf("uPort:%d |", res->udp_port);
    printf("tPort:%d ", res->tcp_port);
    printf("\n");
}

//Load resolvers from a text file into resolver array or link ?
int resolver_load(char * source_file, List * resolvers )
{
    FILE *fp;
    Resolver *res;
    int num=0;
    char line[MAX_LINE], buffer[MAX_LINE];

    list_init(resolvers, free, (void*)resolver_display, (void *) resolver_match);

    if ( (fp=fopen(source_file,"r")) == NULL )
    {
        fprintf(stdout, "ERROR: Cannot open file:%s to read.\n", source_file);
        return -1;
    }
    while (fgets(buffer, MAX_LINE, fp))
    {
        num ++;
        strtrim2(line, MAX_LINE, buffer);
        if( (line[0] == '#') || (line[0] == ';') || (line[0]=='\n') )
            continue;

        if ( NULL == (res= resolver_parse(line)))
        {
            fprintf(stdout, "ERROR: Parse resolver file error, line %d: %s \n", 
                   num, line);
            return -1;
        } 
        list_ins_next(resolvers, resolvers->tail, res);
    }
  
   fclose(fp); 
   return 0; 
}


//int connect_resolver( Resolver* resolver);
//

/*
int main()
{
    List resolvers;
    Resolver *res;

    load_resolvers("resolvers.txt", &resolvers);
    list_travel(&resolvers);
    list_destroy(&resolvers);
}
*/


