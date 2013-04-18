#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resolvers.h"
#include "list.h"

Resolver * resolver_parse(char * line)
{
    Resolver * pres;
    char *tofree, *token, *str;
    int i=0;

    if ( line ==NULL)
        return NULL;

    if( NULL == (pres=malloc(sizeof (Resolver))) )
    {
        fprintf(stdout, "Malloc Error in parse resolver\n");
        return NULL;
    }
    tofree = str = strdup(line);
    /*
     * secret|184.22.34.216|5300|5300|burst
     * tsinghuaa|166.111.8.28|53||CERNET
     * ccert|202.112.57.6:53
     * telecom|219.141.136.10
     */
    //Set default
    pres->udp_port = 53;
    pres->tcp_port = 53;

    while ( ( token = strsep(&str, DELIM)) != NULL)
    {
        switch (i)
        {
            case 0:
                strcpy(pres->name, token);
                break;
            case 1:
                strcpy(pres->ipaddress, token);
                break;
            case 2 :
                pres->udp_port =atoi(token);
                break;
            case 3 :
                pres->tcp_port =atoi(token);
                break;
            case 4 :
                strcpy(pres->isp ,token);
                break;
        }
       i++; 
    }

    return pres;
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
int load_resolvers(char * source_file, List * resolvers )
{
    FILE *fp;
    Resolver *res;
    int num=0;
    char line[MAX_LINE], buffer[MAX_LINE];

    list_init(resolvers, free, (void*)resolver_display);

    if ( (fp=fopen(source_file,"r")) == NULL )
    {
        fprintf(stdout, "ERROR: Cannot open file:%s to read.\n", source_file);
        return -1;
    }
    while (fgets(buffer, MAX_LINE, fp))
    {
        num ++;
        strtrim(line, MAX_LINE, buffer);
        if( (line[0] == '#') || (line[0] == ';') || (line[0]=='\n') )
            continue;

        if ( NULL == (res= resolver_parse(line)))
        {
            fprintf(stdout, "ERROR: Parse resolver file error, line %d: %s \n", 
                   num, line);
            return -1;
        } 
        list_ins_next(resolvers, NULL, res);
    }
  
   fclose(fp); 
   return 0; 
}

//int connect_resolver( Resolver* resolver);
//

int main()
{
    List resolvers;
    Resolver *res;

    load_resolvers("resolvers.txt", &resolvers);
    list_travel(&resolvers);
    list_destroy(&resolvers);
}


