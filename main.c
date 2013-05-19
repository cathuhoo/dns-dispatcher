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
#include "clean_timeout.h"

//Global varables:

Configuration config;
ResolverList resolvers;
Policy policy;
QueryList  queries;
Disp_info *disp_addr;

BOOL  parentRequestStop;
BOOL  parentRequestPause;
pthread_mutex_t query_mutex[MAX_QUERY_NUM]; 

//only used in main.c 
static pthread_t tid_recv_send, tid_timeout;
static pthread_t *tid_dispatchers;

void usage(char * self_name)
{
    //while( ( oc=getopt(argc, argv, "c:dhl:n:p:u:t:")) != -1 )
    fprintf(stderr,"%s [options...]\n", self_name);
    fprintf(stderr,"valid options:\n");
    fprintf(stderr," -c <configuration_file>\n");
    fprintf(stderr," -d: daemonize, or run in background.\n");
    fprintf(stderr," -h: Help(this message).\n");
    fprintf(stderr," -l <log_file>: file to log message.\n");
    fprintf(stderr," -p <pid_file>: file to write pid.\n");
    fprintf(stderr," -t <tcp_port>: tcp port to listen.\n");
    fprintf(stderr," -u <udp_port>: udp port to listen.\n");
}

void signal_handler(int sig)
{
    int i;
    switch(sig) {
    
     case SIGHUP:
        parentRequestPause = TRUE;
        sleep(TIMEOUT+1); // wait for threads to pause 
        if(config.file_log !=NULL)
        {
            fclose(config.fd_log);
            config.fd_log=fopen(config.file_log,"a");
        }
        parentRequestPause = FALSE;
       break;

     case SIGPIPE:
        //debug("SIGPIPE signal catched!\n");

        break;

     case SIGINT:
     case SIGTERM:
        debug("Signal terminate(SIGTERM), free memory and bye.\n");
        parentRequestStop = TRUE;
        sleep(TIMEOUT + 2); // wait for thread exit

        if(config.file_log != NULL)
            fclose(config.fd_log);

        pthread_join(tid_recv_send, NULL);
        for( i=0; i < config.num_threads; i ++)
            pthread_join(tid_dispatchers[i], NULL);
        free(tid_dispatchers);
        free(disp_addr);

        pthread_join(tid_timeout, NULL);

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

    //For Policy Check only, useless for the whole service
    //BOOL policy_check = FALSE;
    //char  *policy_check_ip, *policy_check_domain;
    //policy_check_ip = policy_check_domain =NULL;
    //END Policy check
    

    //The options in command line will not be overwritten by those in configuration file
    while( ( oc=getopt(argc, argv, "c:dhl:n:p:u:t:D:I:P")) != -1 )
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
            //For policy check only, useless for running
	   /*
            case 'P': //daemonize 
                policy_check=TRUE;
                break;

            case 'I': 
                policy_check_ip = strdup(optarg);
                break;

            case 'D': 
                policy_check_domain = strdup(optarg);
                break;
	    */

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
    //config_display(&config);
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
        if( 0> policy_load( "policy.txt", &policy, &resolvers))
        {
            error =1;
            goto error_out;
        }
    }

    if (!error)
    {

        signal(SIGTERM,signal_handler); // catch kill Terminate signal 
        signal(SIGINT,signal_handler); // catch kill Interrupt signal 
        signal(SIGHUP,signal_handler); // catch kill Interrupt signal 
        signal(SIGPIPE,signal_handler); // catch kill Interrupt signal 

        querylist_init(&queries);
        parentRequestStop = FALSE;
        parentRequestPause = FALSE;

        int i;
        for(i = 0; i < MAX_QUERY_NUM; i ++ ) 
        {
            //query_mutex[0] = PTHREAD_MUTEX_INITIALIZER;
            pthread_mutex_init(&query_mutex[i], NULL);
        }

        // Some threads to select upstream resolvers according to policy 
        tid_dispatchers = malloc(sizeof(pthread_t) * config.num_threads);
        disp_addr = malloc(sizeof(Disp_info) * config.num_threads);
        memset(disp_addr, 0, sizeof(Disp_info) * config.num_threads);

        for( i=0; i < config.num_threads; i ++)
        {
            tid_dispatchers[i] = dispatcher(i);  //disp_addr[i]->path_name);
            if(0 == tid_dispatchers[i]) 
            {
                free(disp_addr);
                free(tid_dispatchers);
                error_report("Error: Cannot create dispatcher [%d]\n", i);  
                goto error_out;
            }
        }
        
        //Wait for dispatcher ready
        for ( i=0 ; i < config.num_threads; i++)
        {
            while (! disp_addr[i].ready ) 
            { 
                debug("waiting for dispatcher[%d] \n", i);
            }
            //debug("dispatcher [%d] OK\n", i);
        }
        
        // A thread to recieve queries from clients, and replies to the clients
        tid_recv_send = recv_send();

        // A thread to clean up all timeout queries 
        //debug("clean_time thread begin");
        tid_timeout = clean_timeout();

        //Wait for recv_send thread_exit
        pthread_join(tid_recv_send, NULL);

        pthread_join(tid_timeout, NULL);

        //Stop the dispatcher
        parentRequestStop = TRUE;

        //Wait for dispatcher thread_exit
        for( i=0; i < config.num_threads; i ++)
            pthread_join(tid_dispatchers[i], NULL);
        free(tid_dispatchers);
        free(disp_addr);
    }

error_out:
    fprintf(stderr, "Ooops! It's too bad, Free memory ...\n");

//    getchar();
//clean_out:
    querylist_free(&queries);
    policy_free(&policy);
    resolver_list_free(&resolvers);
    config_free(&config);
    return 0;
}
/*
void policy_check(char *ip, char *domain)
{
    long addr_h;
    if ( ip != NULL)
    {
        addr_h= inet_addr(ip);
        RuleSet *prs = prefix_lookup(&policy.ip_prefix, &addr_h);
        if( prs == NULL)
            fprintf(stderr, "IP address :%s (%lx) not found in IP prefix\n", 
                    ip, addr_h);
        else
            fprintf(stderr, "IP address %s is in rules set:%lx \n", 
                    ip, addr_h);
    }
    if (domain != NULL)
    {

    }
    if ( ip != NULL && domain != NULL)
    {
    }

}
*/
