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

//Data Structures
#include "resolvers.h"
#include "policy.h"
#include "ip_prefix.h"
#include "trie.h"
#include "query.h"

//Threads
#include "recv_send.h"
#include "dispatcher.h"
//#include "sender.h"

Configuration config;
ResolverList resolvers;
Policy policy;
QueryList  queries;
//char * un_names[MAX_RESOLVERS];

Disp_info *disp_addr;


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
        debug("Signal terminate(SIGTERM), free memory and bye.\n");
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
        my_log("ERROR: no policy file in config file \n");    
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

        pthread_t tid_listener;
        pthread_t *tid_dispatchers;
        pthread_t tid_sender;

        //This is the main loop, which :
        //  (1) recieves DNS queries from downstream client(users), and dispatches them to resolver_selectors ;  
        //  (2) receives DNS replies from upstream   
          
        // Some threads to select upstream resolvers according to policy and <src_ip, target_domain>
        tid_dispatchers = malloc(sizeof(pthread_t) * config.num_threads);
        disp_addr = malloc(sizeof(Disp_info) * config.num_threads);
        int i;
        
        for( i=0; i < config.num_threads; i ++)
        {
            //disp_addr.path_name[i] = malloc(MAX_WORD);
            //sprintf(disp_addr[i].path_name,"/tmp/dispatcher_%d\n",i);             
            tid_dispatchers[i] = dispatcher(i);  //disp_addr[i]->path_name);
        }
        
        debug("After dispatcher, now ready to create recv_send threads\n");
        // A thread to recieve queries from clients, and replies to the clients
        tid_listener = recv_send();

        pthread_join(tid_listener, NULL);
        for( i=0; i < config.num_threads; i ++)
            pthread_join(tid_dispatchers[i], NULL);
        pthread_join(tid_sender, NULL);
        //pthread_join(tid_listener, NULL);
    }
        

    //Free all the memory
    //trie_free(policy.trie_dn);
    resolver_list_free(&resolvers);
    policy_free(&policy);
    config_free(&config);
    return 0;
}

