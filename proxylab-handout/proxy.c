#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close";
static const char *proxy_connection_hdr = "Proxy-Connection: close";
void echo(int connfd);


int sfd;
struct sockaddr_in ip4addr;

int parse_request(char* request, int fd, char* host, char* port, char* resource)
{
    return 0;
}

int read_bytes(int fd)
{
    char buf[MAXBUF];
    ssize_t nread = 0;
    for (;;) {
		socklen_t peer_addr_len = sizeof(struct sockaddr_storage);
		nread = recv(fd, buf, MAXBUF, 0);

		if (nread == -1)
			continue;               /* Ignore failed request */

        if (nread == 0)
            break;

	}
    printf("Received:%s", buf);
}

void *thread(void *vargp) 
{  
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self()); 
    char request[MAXBUF];
    char host[MAXBUF];
    char port[MAXBUF];
    char resource[MAXBUF];
    int req_val = 0;
    printf("in thread function\n");

    req_val = parse_request(request, connfd, host, port, resource);
    read_bytes(connfd);

    Free(vargp);                    
    // echo(connfd);
    Close(connfd);
    return NULL;
}

int make_listening_socket(int port)
{
    memset(&ip4addr, 0, sizeof(struct sockaddr_in));
    ip4addr.sin_family = AF_INET;
    ip4addr.sin_port = htons(port);
    ip4addr.sin_addr.s_addr = INADDR_ANY;

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    if (bind(sfd, (struct sockaddr *)&ip4addr, sizeof(struct sockaddr_in)) < 0) {
        close(sfd);
        perror("bind error");
        exit(EXIT_FAILURE);
    }
    if (listen(sfd, 100) < 0) {
        close(sfd);
        perror("listen error");
        exit(EXIT_FAILURE);
    }
    return sfd;
}

void make_client(int port)
{
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid; 
        printf("making client socket");

    listenfd = make_listening_socket(port);
    printf("made client socket and am currently listening");

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int)); 
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen); 
        Pthread_create(&tid, NULL, thread, connfdp);
    }
}


int main(int argc, char *argv[])
{
    int port;
    if (argc < 2){
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    	exit(0);
    }
    signal(SIGPIPE, SIG_IGN);


    port = atoi(argv[1]);
    make_client(port);


    return 0;
}
