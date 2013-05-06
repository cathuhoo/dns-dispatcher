#include "common.h"
#include "ini.h"
#include "config.h"
#include "external.h"

void daemonize_init()
{
    int i;
    if(getppid()==1)  // parent pid ==1, the init process 
        return; /* already a daemon */
    i=fork();
    if (i<0)
        exit(1); /* fork error */
    if (i>0)
        exit(0); /* parent exits */
    /* child (daemon) continues */
    setsid(); /* obtain a new process group */

    for (i=getdtablesize();i>=0;--i)
        close(i); /* close all descriptors */
    i=open("/dev/null",O_RDWR);
    dup(i);
    dup(i); /* handle standart I/O */

    umask(027); /* set newly created file permissions */

    //chdir(WORK_DIR); /* change running directory */

    signal(SIGCHLD,SIG_IGN); /* ignore child */
    signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);

    //the following signals have been captured in forwarder.c
    //signal(SIGHUP,signal_handler); /* catch hangup signal */
    //signal(SIGTERM,signal_handler); /* catch kill signal */
    //signal(SIGINT,signal_handler); /* catch kill signal */
}

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    Configuration* pconfig = (Configuration*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("main", "file_resolvers")) 
    {
        if(pconfig->file_resolvers == NULL)
           pconfig->file_resolvers = strdup(value);
    } 
    else if (MATCH("main", "file_policy")) 
    {
        if(pconfig->file_policy == NULL)
           pconfig->file_policy = strdup(value);
    } 
    else if (MATCH("main", "file_log")) 
    {
        if(pconfig->file_log == NULL)
           pconfig->file_log = strdup(value);
    } 
    else if (MATCH("main", "file_pid")) 
    {
        if(pconfig->file_pid == NULL)
           pconfig->file_pid = strdup(value);
    } 
    else if (MATCH("main", "service_port")) 
    {
        pconfig->service_port = atoi(value);
    } 
    else if (MATCH("main", "tcpservice_port")) 
    {
        pconfig->tcpservice_port = atoi(value);
    } 
    else if (MATCH("main", "num_threads")) 
    {
        pconfig->num_threads = atoi(value);
    } 
    else if (MATCH("main", "daemonize")) 
    {
        if (!strcmp(value, "yes") )
            pconfig->daemonize = TRUE;
        else if ( (!strcmp(value , "no")) && (pconfig->daemonize == 0)  )
            pconfig->daemonize = FALSE;
        else 
        {
            fprintf(stderr, "Warning: invalid value:\"%s\" for name:\"%s\" in section:\"%s\", ignored.\n",
                     value, name ,section);
            pconfig->daemonize = 0;
        }
    } 
    else if (MATCH("main", "log_level")) 
    {
        pconfig->log_level = atoi(value);
    } 
    else 
    {
        fprintf(stderr, "Warning: unknown section or name:\"%s\":\"%s\", ignored.\n", section, name);
    }
    return 0;
}

int config_set_default( Configuration * config)
{
    if( config == NULL)
        return -1;

    memset(config, 0, sizeof (Configuration));
    config->service_port = 53;
    config->tcpservice_port = 53;
    config->log_level = LOG_DEBUG ;
    config->num_threads = 3;
    config->daemonize = FALSE;
    config->fd_log = stderr;
    
    return 0;
}
int config_load( Configuration * config )
{
    if (config ==NULL )
    {
        fprintf(stderr, "Configuration is NULL\n");
        return -1;
    }
    if(config->daemonize) 
        daemonize_init();

    if ( config->file_config != NULL) 
    {
        if (ini_parse(config->file_config, handler, config) < 0) 
        {
            fprintf(stderr, "Can't load %s\n", config->file_config);
            return -1;
        }
    }

    if( config->file_log == NULL)
    {
        config->fd_log = stderr;
    }
    else
    {
        config->fd_log = fopen(config->file_log, "a");
        if( config->fd_log ==NULL) 
        {
            fprintf(stderr, "ERROR: unable to open log file: %s to write(append).\n", config->file_log);
            return (-1);
        }
    }   

    if(config->file_pid != NULL)
    {
        config->fd_pid = fopen(config->file_pid,"w"); // open again  
        if (config->fd_pid == NULL) 
        {
            fprintf(stderr,  "Error: Unable to open pid_file:%s to write\n", config->file_pid); 
            return -1;
        }
        fprintf(config->fd_pid, "%d" , getpid());
        fclose(config->fd_pid);
    }
    
    return 0;
}
void config_display(Configuration * config)
{
    fprintf(stdout, "Configuration:\n\
            \tfile_config: %s\n\
            \tfile_policy: %s\n\
            \tfile_resolvers: %s\n\
            \tfile_log: %s\n\
            \tfile_pid: %s\n\
            \tnum_threads: %d\n\
            \tservice_port(udp): %d\n\
            \tservice_port(tcp): %d\n\
            \tdaemonize: %d\n\
            \tlog_level: %d\n",
            config->file_config,\
            config->file_policy,\
            config->file_resolvers,\
            config->file_log,\
            config->file_pid,\
            config->num_threads,\
            config->service_port,\
            config->tcpservice_port,\
            config->daemonize,\
            config->log_level
           );
}
int config_free(Configuration * config)
{
    if (config->file_config )
        free(config->file_config);

    if (config->file_policy )
        free(config->file_policy);

    if (config->file_resolvers )
        free(config->file_resolvers);

    if (config->file_log )
        free(config->file_log);

    if (config->file_pid )
        free(config->file_pid);

    return 0;
}
/*
int main(int argc, char* argv[])
{
    Configuration config;
    int rcode;

    if( 0 > load_config("configure.ini", &config) )
    {
        printf("Load configuration error:configure.ini\n");    
    }

    printf("Main: Config loaded from : file_blacklist=%s, service_port=%d\n",
        config.file_blacklist, config.service_port);
    return 0;
}

*/
