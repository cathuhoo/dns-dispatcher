
#define MAX_LINE 1024
#define MAX_WORD 64
#define DELIM  "|"
#define DELIM2  ":"

#ifdef DEBUG
   #define debug(fmt, ...) {fprintf(fd_log, "Debug: "); fprintf(fd_log, fmt, ##__VA_ARGS__);fflush(fd_log); }   
   #define debug_point(msg){fprintf(fd_log,"SFSG:%s(%d):%s\n",__FILE__, __LINE__, msg);fflush(fd_log);} 
#else
   #define debug(fmt, ...)    
   #define debug_point(fmt, ...) 
#endif
