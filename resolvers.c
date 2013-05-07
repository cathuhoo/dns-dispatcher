#include "common.h"
//#include "list.h"
#include "resolvers.h"
#include "mystring.h"
#include "external.h"

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
    pres->current_txid =1;
    pres->sockfd = -1;

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
/*
int  resolver_connect( Resolver * res)
{
    //int sock_fd;
    if(res ==NULL)
    {
        fprintf(stdout, "Error: NULL resolver\n");
        return -1;
    }
    printf("try to connect:%s | ", res->name);

    //TODO;
    //

    return 1; 
}
*/
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

void resolver_list_init( ResolverList *rl, void (*match)( void * key1, void * key2), void (*display)(void *data)  )
{
    if (NULL == rl)
        return ;
    rl->size =0;
    rl->match = match;
    rl->display = display;

    int i;
    for (i=0; i< MAX_RESOLVERS; i++)
    {
        rl->resolvers[i] = NULL;
    }
}

//Load resolvers from a text file into resolver array or link ?
int resolver_list_load(char * source_file, ResolverList * rl )
{
    FILE *fp;
    Resolver *res;
    int num=0;
    char line[MAX_LINE], buffer[MAX_LINE];

    resolver_list_init(rl, (void *) resolver_match, (void*)resolver_display);

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
        //list_ins_next(resolvers, resolvers->tail, res);

        rl->resolvers[rl->size] = res;
        rl->size ++; 
    }
  
   fclose(fp); 
   return 0; 
}
Resolver * resolver_list_lookup(ResolverList *rl, char *resolver_name )
{
    int i;
    Resolver * res;
    for (i=0; i < rl->size; i++)
    {
        res= rl->resolvers[i];
        if (res)
        {
             if ( 0 == strcasecmp(resolver_name, res->name))
                 return res;
        }
        else
        {
            //error
        }
    }
    return NULL;
}
void resolver_list_travel(ResolverList *rl) 
{
  int i;

  for (i=0; i < rl->size; i++)
  {
     rl->display(rl->resolvers[i]);
     fprintf(stderr, "Resolver [%d]: %p\n", i, rl->resolvers[i]);
  }

}

void resolver_list_free(ResolverList *rl) 
{
    if (rl == NULL)
        return ;
    int i;
    for ( i = 0 ; i < rl->size ; i++)
    {
      if( NULL != rl->resolvers[i] )
      {
        Resolver * r = rl->resolvers[i];
        free(r);
      }
    }
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


