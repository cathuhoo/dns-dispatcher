#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdio.h>

#define BOOL unsigned char
#define TRUE  1
#define FALSE 0

#define DEFAULT_CONFIG "/usr/local/dadder/dadder.config"
#define DEFAULT_LOG "/usr/local/dadder/log.txt"
#define DEFAULT_PID "/usr/local/dadder/pid.txt"

#define LOG_NONE    0
#define LOG_ERROR   1
#define LOG_QUERY   2
#define LOG_FORWARD 4
#define LOG_REPLY   8
#define LOG_INFO    16
#define LOG_MEASURE 32 
#define LOG_DEBUG   64 

typedef struct
{

    char * file_config;
    char * file_policy;
    char * file_resolvers;
    // Log 
    char * file_log;
    char * file_pid;
    FILE * fd_log;
    FILE * fd_pid;

    int  num_threads;

    int    service_port;
    int    tcpservice_port;
    BOOL   daemonize ;

    
    int    log_level;

    //int    (*log)(char );

} Configuration;

int config_load( Configuration * config );
int config_set_default( Configuration * config);
void config_display(Configuration * config);
int config_free(Configuration * config);


#endif
