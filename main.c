#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <pthread.h>

#include "common.h"
#include "config.h"
//#include "blacklist.h"
//#include "hashtbl.h"
//#include "badip.h"
//#include "forwarder.h"

#include "resolvers.h"
#include "policy.h"
#include "ip_prefix.h"
#include "trie.h"


void usage(char * self_name)
{
    fprintf(stderr,"%s [options...]\n", self_name);
    fprintf(stderr,"valid options:\n");
    fprintf(stderr," -c <configuration_file>: default /usr/local/dadder/dadder.conf;\n");
    fprintf(stderr," -d: daemonize, or run in background;\n");
    fprintf(stderr," -l <log_file>: file to log message; this value can also be configured in confguration file.\n");
    fprintf(stderr," -p <pid_file>: file to write pid; this value can also be configured in confguration file.\n");
    fprintf(stderr," -h: Help(this message).\n");
     

}

Configuration config;
List resolvers;
Policy policy;
Resolver *res;
IPPrefix ip_prefix;
trieNode_t * trie_dn ;

int main(int argc, char* argv[])
{
    int oc;

    config_set_default(&config);

    //The options in command line will not be overwritten by those in configuration file
    while( ( oc=getopt(argc, argv, "c:dhl:n:p:u:t:")) != -1 )
    {
        switch (oc)
        {
            case 'c': 
                config.file_config = strdup(optarg);
                //remember to free  the memory of config_file 
                break;
            case 'd': //daemonize 
                config.daemonize=TRUE;
                break;
            case 'h':
                usage(argv[0]);
                break; 
            case 'l': //log file
                config.file_log = strdup(optarg);
                //remember to free  the memory 
                break;
            case 'n': 
                config.num_threads =  (int)strtol(optarg, (char **)NULL, 10); 
                if(errno == EINVAL || errno == ERANGE )
                {
                    fprintf(stderr, "erro number of threads:%s\n", optarg );
                    return -1;
                }
                break;
            case 'p': //pid file
                config.file_pid = strdup(optarg);
                //remember to free  the memory 
                break;

            case 'u': 
                config.service_port =  (int)strtol(optarg, (char **)NULL, 10); 
                if(errno == EINVAL || errno == ERANGE )
                {
                    fprintf(stderr, "UDP port number error:%s\n", optarg );
                    return -1;
                }
                break;

            case 't': 
                config.tcpservice_port =  (int)strtol(optarg, (char **)NULL, 10); 
                if(errno == EINVAL || errno == ERANGE )
                {
                    fprintf(stderr, "TCP Port Number Error:%s\n", optarg );
                    return -1;
                }
                break;

            default:
                usage(argv[0]);
                break; 
        }// switch(oc)
    }//end of while (parse the options)

    if( config_load(&config)<0 ) //The values can be overwrite by command line options 
    {
        fprintf(stderr, "ERROR: load configuration file failed: %s.\n", config.file_config);
        exit(-1);
    }
    config_display(&config);

    if(config.file_resolvers)
    {
        resolver_load(config.file_resolvers, &resolvers);
    }
    resolver_travel(&resolvers);

    if(NULL == config.file_policy)
    {
        fprintf(stderr, "ERROR: no policy file \n");    
    }
    else
    {
        policy_load( "policy.txt", &policy, &resolvers);
        policy_travel( &policy);
        prefix_init(&ip_prefix);

        policy_load_ipprefix(&policy, &ip_prefix);

        trie_dn = TrieInit();
        policy_load_domain(&policy, trie_dn);
        printf("Trie Travel:\n");
        TrieTravelE(trie_dn);
    }
    
    trie_free(trie_dn);
    resolver_free(&resolvers);
    policy_free(&policy);

    config_free(&config);
    return 0;
}

