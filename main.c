#include "common.h"

//Data Structures
#include "resolvers.h"
#include "policy.h"
#include "ip_prefix.h"
#include "trie.h"
#include "query.h"

//Threads
#include "recv_send.h"
#include "dispatcher.h"

Configuration config;
ResolverList resolvers;
Policy policy;
QueryList  queries;

Disp_info *disp_addr;

//unsigned short  ** id_mapping ; //[MAX_SOCKFD][65536];


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

        querylist_free(&queries);
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
    if(config.daemonize) 
        daemonize_init();

    if(config.file_resolvers)
    {
        resolver_list_load(config.file_resolvers, &resolvers);
    }
    else
    {
        fprintf(stderr, "ERROR: No DNS resolvers found \n");    
        error=1;
    }

    if(NULL == config.file_policy)
    {
        fprintf(stderr, "ERROR: no policy file in config file \n");    
        error=1;
    }
    else
    {
        policy_load( "policy.txt", &policy, &resolvers);
    }
    if (!error)
    {

        signal(SIGTERM,signal_handler); // catch kill Terminate signal 
        signal(SIGINT,signal_handler); // catch kill Interrupt signal 
        signal(SIGHUP,signal_handler); // catch kill Interrupt signal 


        querylist_init(&queries);

        pthread_t tid_listener;
        pthread_t *tid_dispatchers;

        // Some threads to select upstream resolvers according to policy and <src_ip, target_domain>
        tid_dispatchers = malloc(sizeof(pthread_t) * config.num_threads);
        disp_addr = malloc(sizeof(Disp_info) * config.num_threads);
        int i;
        for( i=0; i < config.num_threads; i ++)
        {
            tid_dispatchers[i] = dispatcher(i);  //disp_addr[i]->path_name);
            if(0 == tid_dispatchers[i]) 
            {
              error_report("Error: Cannot create dispatcher [%d]\n", i);  
              error=1;
              goto error_out;
            }
        }
        
        sleep(1);
        debug("After dispatcher, now ready to create recv_send threads\n");
        // A thread to recieve queries from clients, and replies to the clients
        tid_listener = recv_send();

        pthread_join(tid_listener, NULL);
        for( i=0; i < config.num_threads; i ++)
            pthread_join(tid_dispatchers[i], NULL);
        free(tid_dispatchers);
        //pthread_join(tid_listener, NULL);
    }
        

error_out:
    fprintf(stderr, "Ooops! Clean memory of resolvers...\n");
    querylist_free(&queries);
    resolver_list_free(&resolvers);
    policy_free(&policy);
    config_free(&config);
    return 0;
}

