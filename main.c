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
#include "query.h"

Configuration config;
ResolverList resolvers;
Policy policy;
QueryList  queries;

pthread_t tid_listener;
pthread_t *tid_dispatchers;
pthread_t tid_sender;


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

void signal_handler(int sig)
{
    switch(sig) {
    
     case SIGHUP:
        if(config.file_log !=NULL)
        {
            fclose(config.fd_log);
            config.fd_log=fopen(config.file_log,"a");
        }
       break;

     case SIGINT:
     case SIGTERM:
        debug("I: Signal terminate(SIGTERM), dump and bye.\n");
        if(config.file_log != NULL)
            fclose(config.fd_log);

        resolver_list_free(&resolvers);
        policy_free(&policy);
        config_free(&config);

        exit(0); // don't return to the select loop in forwarder()
      break;
    }
}


int main(int argc, char* argv[])
{
    int oc;
    int error=0;

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
        resolver_list_load(config.file_resolvers, &resolvers);
        #ifdef DEBUG
            resolver_list_travel(&resolvers);
        #endif
    }
    else
    {
        fprintf(stderr, "ERROR: No DNS resolvers found \n");    
        error=1;
    }

    if(NULL == config.file_policy)
    {
        my_log(config.fd_log, "ERROR: no policy file in config file \n");    
        error=1;
    }
    else
    {
        policy_load( "policy.txt", &policy, &resolvers);
        #ifdef DEBUG
            policy_travel( &policy);
            TrieTravelE(policy.trie_dn);
        #endif
        
    }
    if (!error)
    {
        #ifdef DEBUG
            long addr_h = inet_ptoh("166.111.1.1", NULL);
            Action * pa = policy_lookup(&policy, addr_h, "mail.google.com");
            printf("returned address pa=%p \n", pa);
            if (pa != NULL)
            {
                printf("op:%d\n",pa->op);
                printf("re:%s\n", pa->resolver->name);
            }
        #endif

        signal(SIGTERM,signal_handler); // catch kill Terminate signal 
        signal(SIGINT,signal_handler); // catch kill Interrupt signal 
        signal(SIGHUP,signal_handler); // catch kill Interrupt signal 

        //This is the main loop, which :
        //  (1) recieves DNS queries from downstream client(users), and dispatches them to resolver_selectors ;  
        //  (2) receives DNS replies from upstream   
          
        // A thread to recieve queries from clients, and replies to the clients
        tid_listener = listener();//&resolvers, &config, &queries);

        // Some threads to select upstream resolvers according to policy and <src_ip, target_domain>
        //TODO
        //dispatcher(&policy, &resolvers, &config, &queries)

        //A thread to send queries to upstream resolvers, and send replies back to the clients
        //TODO
        //sender(&resolvers, &config, &queries);

        pthread_join(tid_listener, NULL);
        //pthread_join(tid_listener, NULL);
    }
        

    //Free all the memory
    //trie_free(policy.trie_dn);
    resolver_list_free(&resolvers);
    policy_free(&policy);
    config_free(&config);
    return 0;
}

