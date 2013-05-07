/*
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
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
#include <sys/un.h> // for sockaddr_un

#include <pthread.h>
#include <arpa/nameser.h>
*/

#include "common.h"

ssize_t readn(int fd, void *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nread;
    char    *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;      /* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;              /* EOF */

        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);      /* return >= 0 */
} ///end readn 


// Write "n" bytes to a descriptor. 
ssize_t  writen(int fd, const void *vptr, size_t n)
{
    size_t      nleft;
    ssize_t     nwritten;
    const char  *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;       // and call write() again 
            else
                return(-1);         // error 
        }

        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
} // end writen 

void print_time (FILE * fd) //char * str_result)
{
    if(fd == NULL) 
        fd = stderr;
    time_t ct=time(NULL);
    time_t *tp=&ct;
    struct tm *tmp;
    tmp=localtime(tp);
    fprintf(fd, "%4d-%02d-%02d %02d-%02d-%02d ",
            1900+tmp->tm_year,1+tmp->tm_mon, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
}
/*
char * strReverse(char *srcStr, char* dstStr)
{
    int i, length;
    if (srcStr == NULL || dstStr == NULL)
        return NULL;

    length = strlen(srcStr);
    for ( i =0; i < length; i++)
        dstStr[i] = srcStr[length - i - 1 ];
    dstStr[length] = '\0';

    return dstStr; 
}

char * strTrim(char * s){

    int head, tail, length,new_length, i;
    length=strlen(s); 

    for (head=0; head < length; head ++)
        if ( ! ISSPACE(s[head]) )
            break;

    for (tail=length-1; tail >head ; tail --)
        if ( ! ISSPACE(s[tail]) )
            break;

    new_length = tail - head +1;
    if (head != 0)
    {
        for (i=0; i< new_length; i++)
            s[i] = s[head+i];
    }
    s[new_length] = '\0';
    return s;  
     
}
*/

//int CreateClientUnixSocket( char * server_path, int protocol, 

int CreateUnixServerSocket(int addrFamily, int protocol, char * strPath, int port,  struct sockaddr * serv_addr )
{
    int listenfd;

    struct sockaddr_un * servaddr = (struct sockaddr_un *)serv_addr;

    listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);

    unlink(strPath);
    bzero(servaddr, sizeof(struct sockaddr_un));
    servaddr->sun_family = AF_LOCAL;
    strcpy(servaddr->sun_path, strPath);
    bind(listenfd, (SA *) servaddr, sizeof(struct sockaddr_un));
    listen(listenfd, 5);

    return listenfd;
}

//Return sockfd on success
//otherwise return -1;
int CreateClientSocket(int addr_family, char * strServAddr,int protocol, int server_port, SA *saServAddr)
{
    int sockfd, rcode;

    sockfd = socket(addr_family, protocol, 0);
    if(sockfd ==-1)
    {
        fprintf(stderr, "Error: CreateClientSocket: to %s error: %s(%d) in %s(%d).\n" , 
                strServAddr, strerror(errno), errno, __FILE__, __LINE__);
        return -1;
    }
    bzero(saServAddr, sizeof(SA));

    switch ( addr_family)
    {
        case AF_INET :
        {
            struct sockaddr_in *psa = (struct sockaddr_in *)saServAddr;
            psa->sin_family = addr_family;

            psa->sin_port = htons(server_port);
            rcode=inet_pton(addr_family, strServAddr, &(psa->sin_addr));
            if (rcode <=0)
            {
                fprintf(stderr, "Error: CreateClientSocket: inet_pton error:%s\n", strServAddr);
                return -1;
            }
            return sockfd;
        }

        case AF_LOCAL:
        {
            struct sockaddr_un *psa = (struct sockaddr_un *)saServAddr;

            psa->sun_family = AF_LOCAL;
            strcpy( psa->sun_path, strServAddr);
            connect( sockfd, (SA *) psa, sizeof(SA));
            return sockfd;
        }
    } //switch

    return -1;
}

int CreateServerSocket(int addr_family, int protocol, char * str_addr, int port, struct sockaddr * servaddr )
{
    int socket_fd, rcode;

	socket_fd = socket(addr_family, protocol, 0);
    if(socket_fd ==-1)
    {
        fprintf(stderr,"Error: CreateServerSocket: error: %s(%d) in %s(%d).\n" , 
                strerror(errno), errno, __FILE__, __LINE__);
        return -1;
    }
    bzero(servaddr, sizeof(struct sockaddr));
    switch(addr_family){
        case AF_INET:
        {
            struct sockaddr_in * server_addr = (struct sockaddr_in *) servaddr;
            server_addr->sin_family      = addr_family;
            long longAddr; 
            if(1 != inet_pton(AF_INET, str_addr, &longAddr))
            {
                fprintf(stderr, "address error:%s \n", str_addr);
                close(socket_fd);
                return -1;
            }
            server_addr->sin_addr.s_addr = longAddr; //htonl(str_addr);
            server_addr->sin_port        = htons(port);

            rcode=bind(socket_fd, (struct sockaddr *) server_addr, sizeof(struct sockaddr));
            if(rcode ==-1)
            {
                fprintf(stderr, "Error: CreateServerSocket: bind on port %d error, errno=%d\n", port, errno);
                close(socket_fd);
                return -1;
            }
            if(protocol ==SOCK_STREAM)
            {
                rcode=listen(socket_fd, MAX_TCP_CLIENTS);
                if(rcode ==-1)
                {
                    fprintf(stderr, "Error: listen on socket %d error, errno=%d in %s(%d).\n", 
                          socket_fd, errno, __FILE__, __LINE__);
                    close(socket_fd);
                    return -1;
                }
            }
            return socket_fd;
        }
        case AF_LOCAL:
        {
            struct sockaddr_un  *unp = (struct sockaddr_un *) servaddr;
            unlink(str_addr);
            unp->sun_family = AF_LOCAL;
            strcpy(unp->sun_path, str_addr);
            rcode = bind(socket_fd, (struct sockaddr *) unp, sizeof( struct sockaddr));
            if(rcode != 0)
            {
                fprintf(stderr, "Error: bind on socket %d error, errno=%d:(%s) in %s(%d).\n", 
                      socket_fd, errno, strerror(errno),__FILE__, __LINE__);
                close(socket_fd);
                return -1;
            }
            if ( protocol == SOCK_STREAM )
            {
                rcode = listen(socket_fd, 1); //MAX_TCP_CLIENTS);
                if(rcode ==-1)
                {
                    fprintf(stderr, "Error: listen on socket %d error, errno=%d:(%s) in %s(%d).\n", 
                      socket_fd, errno, strerror(errno),__FILE__, __LINE__);
                    close(socket_fd);
                    return -1;
                }
            }
            return socket_fd;
        }
    }//switch
    fprintf(stderr, "Error: Unknown Address_family :%d in %s(%d).\n", 
            addr_family, __FILE__, __LINE__);
    close(socket_fd);
    return -1;
}

//You must be sure that the size of array is large enough, 
//that is equal or greater than range
int initialize_random_number(int array[], int range)
{
    int i,tmp,r;
    for (i=0; i<range; i++)
        array[i]=i+1;
    srand ( time(NULL) );
    //for(i=range-1; i>0; i--)
    for(i=range-1; i>0; i--)
    {
        r = rand() % i;
        tmp = array[r];
        array[r] = array[i];
        array[i] = tmp;
    }
    return 0;
}
//generate a random and not repeatable number
int get_random(int array[], int range)
{
    static int pointer=0;
    int r1, r2, top, bottom, result;

    result = array[pointer];
//    return array[pointer];

    //swap 2 elements in the other half of array
    if (pointer < range/2) 
    {
        top=range;
        bottom=range/2 ;
    }
    else
    {
        top=range/2 ;
        bottom=0;
    } 
    r1= rand() % (top - bottom) +bottom; 
    r2= rand() % (top - bottom) +bottom; 
    int tmp=array[r1];
    array[r1]=array[r2];
    array[r2]=tmp;
    
    //printf("pointer=%d, r1=%d , r2=%d number=%d\n", pointer, r1, r2, result);

    pointer = (pointer +1 ) % range;
    return result;

}

//Get Time in millisecond
unsigned long getMillisecond()
{
    struct timeval tv;
    if(gettimeofday(&tv,NULL) != 0)
        return 0;
    return (tv.tv_sec*1000) + (tv.tv_usec / 1000);
}

int maximum(int array[], int size)
{
    int i, max;
    max=INT_MIN;
    for (i=0; i < size; i++)
    {
        max= MAX2 (array[i], max);
    }
    return max;
}

int minimum(int array[], int size)
{
    int i, min;
    min = INT_MAX;
    for (i=0; i < size; i++)
    {
        min = array[i] > min ? min: array[i];
    }
    return min;
}

#ifdef	HAVE_SOCKADDR_DL_STRUCT
#include	<net/if_dl.h>
#endif

//char * sock_ntop(const struct sockaddr *sa, socklen_t salen, char * str, int sizeStr);

/* include sock_ntop */
//char * sock_ntop(const struct sockaddr *sa, socklen_t salen)
char * sock_ntop(const struct sockaddr *sa, socklen_t salen, char * str, int sizeStr)
{
    //static char str[128];		/* Unix domain is largest */

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeStr-8) == NULL)
			return(NULL);
		if (ntohs(sin->sin_port) != 0) {
            char	*portstr= malloc(8);
			snprintf(portstr,8, ":%d", ntohs(sin->sin_port));
			strcat(str, portstr);
            free(portstr);
		}
		return(str);
	}
/* end sock_ntop */

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		str[0] = '[';
		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, sizeStr - 9) == NULL)
			return(NULL);
		if (ntohs(sin6->sin6_port) != 0) {
            char	*portstr= malloc(8);
			snprintf(portstr, 8, "]:%d", ntohs(sin6->sin6_port));
			strcat(str, portstr);
            free(portstr);
			return(str);
		}
		return (str + 1);
	}
#endif

#ifdef	AF_UNIX
	case AF_UNIX: {
		struct sockaddr_un	*unp = (struct sockaddr_un *) sa;

			/* OK to have no pathname bound to the socket: happens on
			   every connect() unless client calls bind() first. */
		if (unp->sun_path[0] == 0)
			strcpy(str, "(no pathname bound)");
		else
			snprintf(str, sizeStr, "%s", unp->sun_path);
		return(str);
	}
#endif

#ifdef	HAVE_SOCKADDR_DL_STRUCT
	case AF_LINK: {
		struct sockaddr_dl	*sdl = (struct sockaddr_dl *) sa;

		if (sdl->sdl_nlen > 0)
			snprintf(str, sizeStr, "%*s (index %d)",
					 sdl->sdl_nlen, &sdl->sdl_data[0], sdl->sdl_index);
		else
			snprintf(str, sizeStr, "AF_LINK, index=%d", sdl->sdl_index);
		return(str);
	}
#endif
	default:
		snprintf(str, sizeStr, "sock_ntop: unknown AF_xxx: %d, len %d",
				 sa->sa_family, salen);
		return(str);
	}
    return (NULL);
}

////////////////

/////////////
/*
char *
Sock_ntop(const struct sockaddr *sa, socklen_t salen)
{
	char	*ptr;

	if ( (ptr = sock_ntop(sa, salen)) == NULL)
		error_report("sock_ntop error");	
	return(ptr);
}
*/
