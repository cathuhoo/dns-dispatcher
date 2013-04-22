#include <stdio.h>

#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <math.h>

#include "mystring.h"
#include "ip_prefix.h"


// addr is in host order (a value returned from ntohl()) 
int prefix_hash_match( long  *addr, HashItem *hi)
{
    //unsigned long addr;

    if ( hi == NULL)
    {
        fprintf(stdout, "HashIterm is NULL\n");
        return -1;
    }
    if ( *addr == hi->ipaddr )
    {
        return 1;
    }
    else return 0;
} 

int prefix_init( IPPrefix *prefix)
{
    int i;
    if(prefix == NULL)
        return -1;
    if ( NULL == (prefix->prefix24 = malloc( sizeof(RuleSet) * MAX_HASH_24) ) )
    {
        fprintf(stdout, "Error: malloc for prefix24 failed\n");
        return -1;
    }
    for (i=0; i< MAX_HASH_24; i ++)
        CLEARSET(prefix->prefix24[i]) ;

    if ( NULL == (prefix->prefix32 = malloc( sizeof(List) * MAX_HASH_32) ) )
    {
        fprintf(stdout, "Error: malloc for prefix32 failed\n");
        return -1;
    }
    for (i=0; i< MAX_HASH_32; i++)
        list_init(&prefix->prefix32[i], free, NULL, (void*) prefix_hash_match) ;

    return 0;
}

int prefix_free( IPPrefix *prefix)
{
    int i;
    if (NULL == prefix )
        return 0;
    if ( prefix -> prefix24)
    {
        free( prefix -> prefix24);
    }
    if ( prefix -> prefix32)
    {
        for (i=0; i < MAX_HASH_32 ; i ++)
        {
            list_destroy( &prefix -> prefix32[i]);
        }
        free( prefix -> prefix32);
    }

    return 0;
}

//given an IP address "addr"(in host order), shift right "32-mask" times  
int ip_index(long * addr, int mask)
{
    return ((*addr)& 0xFFFFFFFF) >> (32-mask);
    //return (*addr) >> (32-mask);
}

int ipstr_hash(  char  * ipstr)
{
    long idx, hash;
    if (1 != inet_pton(AF_INET, ipstr, &idx)) 
    {
        fprintf(stdout, "IP address error:%s \n", ipstr);
        return -1;
    }
    //idx=ntohl(idx);
    hash = (idx & 0x0FFFF) ^ ( idx >> 16);
    hash = hash ^ ( (idx >>8) & 0x0FFFF);
    return hash;
}
long ip_hash( long *addr) // network order
{
    long  hash;
    *addr &=0xFFFFFFFF;

    hash = (*addr & 0x0FFFF) ^ ( *addr >> 16);
    hash = hash ^ ( (*addr >>8) & 0x0FFFF);
    return hash & 0xFFFF;
}

HashItem * ip_hashitem(long *addr, int rule_no) //addr is in network order
{
    //long idx;
    HashItem *pt;

    if ( NULL ==( pt = malloc(sizeof( HashItem))))
    {
       fprintf(stdout, "Error: Can not allocate memory for IP HashItem\n");
       return NULL;
    }
    pt->ipaddr = *addr ; 
    pt->ruleset = ( 1 << rule_no);
    return pt;
}

int prefix32_add( List prefix32[], long * addr, int rule_no)
{
   int idx;
   HashItem *pt;

   idx = ip_hash( addr);
   if ( NULL !=( pt = list_lookup(&prefix32[idx], addr)) ) // find it, ip is alread in the hash
   {
        pt->ruleset  |= (1 << rule_no);
        return 0;
   }
   else
   {
       pt = ip_hashitem( addr, rule_no);
       if(pt) 
       {
            list_ins_next(&prefix32[idx],NULL, pt); 
            return 0;
       }
       else
       {
           return -1;
       }
   }
}


int prefix24_add(RuleSet  prefix24[], long * addr, int rule_no)
{
    long idx = ip_index( addr, 24);
    
    prefix24[idx] |= (1<<rule_no); 

    return 0;
}
long inet_ptoh( char *ipstr, long * addr)
{
    long lAddr, *ptr;

    if (ipstr == NULL )
        return -1;

    if (addr == NULL)
        ptr = &lAddr;
    else
        ptr= addr;

    if ( 1 != inet_pton(AF_INET, ipstr, ptr)) 
    {
        fprintf(stdout, "ERROR: IP address format error: %s\n", ipstr);
        return -1;
    }

    lAddr = ntohl( *ptr);

    return lAddr & 0xFFFFFFFF;
}

int prefix_add(IPPrefix *prefix, char * ipstr, int mask, int rule_no)
{
    long  addr_h, addr_n, addr_i, pre;
    int i, sub, p;

#ifdef DEBUG
    fprintf(stdout, "prefix_add:%s\n", ipstr);
#endif
    addr_h = inet_ptoh(ipstr, &addr_n);

    if (mask == 24) // add it directly in to prefix24
    {
        prefix24_add(prefix->prefix24, &addr_h, rule_no);
    }
    else if (mask ==32 ) //add it to hash table of prefix32
    {
        prefix32_add(prefix->prefix32, &addr_h, rule_no);

    }
    else if ( mask > 0 && mask < 24)   
    {
        sub = 24 - mask;
        pre = addr_h & (0xFFFFFFFF << (32-mask)); 
        //p = pow(2, sub);
        p = 1 << sub;

        for (i = 0; i < p; i++ )
        {
            addr_i = pre | (i <<8)  ;
            //fprintf(stdout, "addr[%d]:%lx\n", i, addr_i);
            prefix24_add (prefix->prefix24, &addr_i, rule_no); 
        }

    }
    else if ( mask > 24  && mask < 32)   
    {
        sub= 32 - mask;

        pre = addr_h & ( 0xFFFFFFFF << (32-mask));

        p = 1 << sub;

        for ( i =0; i < p; i ++)
        {
            addr_i = pre | i;
            //fprintf(stdout, "addr_[%d]:%lx\n", i, addr_i);
            prefix32_add (prefix->prefix32, &addr_i, rule_no); 
        } 

    }
    else
    {
        fprintf(stdout, "ERROR: prefix must be less than 32\n");
        return -1;
    }

    return 0;

}

//RuleSet * prefix32_lookup( List  prefix32[] , char * ipstr)
RuleSet * prefix32_lookup( List  prefix32[] , long *addr) // network order
{
    int idx;
    HashItem *ptr;

    idx=ip_hash(addr);

    ptr=list_lookup(&prefix32[idx],  addr);
    if (ptr)
        return &ptr->ruleset;
    else 
        return NULL;
}


RuleSet* prefix_lookup(IPPrefix *prefix , long * addr )
{
    RuleSet * rs;

    rs =  prefix32_lookup(prefix->prefix32 , addr);

    if(rs) 
        return rs;
    else
    {
        int idx =  ip_index(addr, 24);
        return &prefix->prefix24[idx]; 
    }
}



int prefix_load(char * file_name, IPPrefix * prefix, int rule_no)
{

    char buffer[MAX_LINE], line[MAX_LINE];
    char *ipstr, * pt, *pmask;
    int mask, num=0;

    if (file_name == NULL || prefix == NULL)
    {
        fprintf(stdout, "ERROR: prefix_load : either file_name or prefix is NULL\n");
        return -1;
    }
    
    FILE * fp;
    if (  NULL == (fp = fopen(file_name, "r")))
    {
        fprintf(stdout, "ERROR: prefix_load: open file: %s failed.\n", file_name);
        return -1;
    } 

    while ( fgets(buffer, MAX_LINE, fp))
    {
        num ++;
        strtrim2(line, MAX_LINE, buffer);
        if( (line[0] == '#') || (line[0] == ';') || (line[0]=='\n') )
           continue;
#ifdef DEBUG
        fprintf(stdout, "DEBUG:%s\n", line);
#endif

        if ( NULL== (pt=strchr(line, '/')))
        {
            mask = 32;
            ipstr = line;
        }
        else
        {
            pmask = pt +1 ;
            mask = (int) strtol(pmask, (char **) NULL, 10  );
            *pt = '\0';
            ipstr = line;
        }
        prefix_add(prefix , ipstr, mask, rule_no);
    }

    fclose(fp);

    return 0;
} 

int prefix_setall(IPPrefix * prefix, int rule_no)
{

    int i;
    List * preList, *plist;
    ListElmt * pElem ;
    HashItem * ph;
    RuleSet * rs;

    if (prefix == NULL)
        return -1;
    
    rs = prefix -> prefix24;

    for ( i = 0; i < MAX_HASH_24; i++)
    {
        rs[i] = rs[i] | (1 <<rule_no); 
    }

    preList = prefix -> prefix32 ; 

    for ( i = 0; i < MAX_HASH_32; i++)
    {
        plist = &preList[i]; //????????????
        pElem = plist -> head;
        while(pElem)
        {
            ph = (HashItem *) pElem ->data;
            if(ph)
                ph->ruleset |= (1 <<rule_no); 
            pElem = pElem->next;
        }
    }
    return 0;
}
/*

int main(int argc , char * argv[])
{
    char ipaddress[]  = "1.2.3.4";
    char ipaddress2[] = "1.2.4.5";
    long addr;
    long idx;
    IPPrefix prefix;
    RuleSet * rs;

    if (argc <3) 
    {
        fprintf(stdout, "Usage: %s <mask> <rule_no>\n", argv[0]);
        exit (-1);
    }

    prefix_init(&prefix);
    
    prefix_add(&prefix, ipaddress, atoi(argv[1]), atoi(argv[2]));
    //prefix_add(&prefix, ipaddress2, 32,8);

    //RuleSet *rs = prefix_lookup( &prefix, ipaddress);
    //printf("ipaddress:%s rs=0x%0lX\n", ipaddress, *rs);

    //inet_pton(AF_INET, ipaddress2, & addr);

    idx = inet_ptoh(ipaddress, NULL);
    rs = prefix_lookup( &prefix, &idx);
    printf("ipaddress: %s rs=0x%lx\n", ipaddress, *rs);

    idx = inet_ptoh(ipaddress2, NULL);
    rs = prefix_lookup( &prefix, &idx);
    printf("ipaddress2: %s rs=0x%lx\n", ipaddress2, *rs);

    prefix_free(&prefix);

}
*/
