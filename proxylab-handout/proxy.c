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
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char *host_init = "Host: ";
static const char *accept_line_hdr = "Accept */* \r\n";
static const char *end_line = "\r\n";
static const char *default_port = "80";
static const char *version_hdr = " HTTP/1.0";
static const char *http_hdr = " http://";
static const char *colon = ":";
void echo(int connfd);


int sfd;
struct sockaddr_in ip4addr;

void parse_host_and_port(char* request, char* host, char* port)
{
    char request_cpy[MAXBUF] = {0};
    strcpy(request_cpy, request);

    char *temp = strstr(request_cpy,"Host:");
    if (temp){
        temp = temp + 6;

        char* p = strchr(temp,':');
        char* n = strchr(temp, '\r');

        if (p < n){
            *p = '\0';
            strcpy(host, temp);
            char* temp_p = p + 1;
            p = strchr(temp_p,'\r');
            *p = '\0';
            strcpy(port, temp_p);
        }
        else {
            memcpy(port, default_port, strlen(default_port));
            *n = '\0';
            strcpy(host, temp);

        }
    }
    else {
        return;
    }
}

int parse_request(char* request, char* type, char* protocol, char* host, char* port, char* resource, char* version){
	char url[MAXBUF];
	
	if((!strstr(request, "/")) || strlen(request) == 0) {
		return -1;
    }

    parse_host_and_port(request, host, port);
	sscanf(request,"%s %s %s", type, url, version);

    char* test = strstr(url, "://");
	
	if (test) {
    	strcpy(resource, "/");
		sscanf(url, "%[^:]://%*[^/]%s", protocol, resource);
        if(!strcmp(resource, "/\0")){
            resource = "/\0";
        }

    }
	else {
    	strcpy(resource, "/");
		sscanf(url, "[^/]%s", resource);
    }

	return 0;
}

// int read_bytes_from_server(int fd, char* p)
// {
//     printf("in read_bytes");
//     int total_read = 0;
//     ssize_t nread = 0;
//     char temp_buf[MAXBUF] = {0};
//     char temp[MAXBUF] = {0};
//     while(1) {
// 		nread = recv(fd, (p + total_read), MAXBUF, 0);
//         total_read += nread;
//          printf("nread = %d\n", nread);
// 		if (nread == -1) {
//             printf("error: %s\n", strerror(errno));
// 			continue;     
//         }         

//         if (nread == 0){
//             break;
//         }
//     }
// }

int read_bytes(int fd, char* p)
{
    int total_read = 0;
    ssize_t nread = 0;
    char temp_buf[MAXBUF] = {0};
    char temp[MAXBUF] = {0};
    while(1) {
		nread = recv(fd, (p + total_read), MAXBUF, 0);
        total_read += nread;
        int index = nread - 4;
        strcpy(temp_buf, p + nread - 4);
        // printf("nread = %d\n", nread);
		if (nread == -1) {
            printf(errno);
			continue;     
        }         

        if (nread == 0){
            break;
        }
        
        if(!strcmp(temp_buf, "\r\n\r\n")){
            break;
        }


        // if(strcmp(p[total_read - 3], "\r\n")){
        //     break;
        // }
	}
        // printf("Received:%zd\n", total_read);

    return total_read;
}

int create_send_socket(int sfd, char* port, char* host, char* request, char* length, char* p)
{
        /* Obtain address(es) matching host/port */
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;
    char buffer[MAXBUF];


    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* TCP socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */
    s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }
    /* getaddrinfo() returns a list of address structures.
    Try each address until we successfully connect(2).
    If socket(2) (or connect(2)) fails, we (close the socket
    and) try the next address. */
    
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
        if (sfd == -1)
    continue;
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  /* Success */
        close(sfd);
    }
    if (rp == NULL) {               /* No address succeeded */
        fprintf(stderr, "Could not connect\n");
    }
    freeaddrinfo(result);           /* No longer needed */

    ssize_t nread;
    // printf("in send_request \n");

    while (length > 0)
    {
        nread = write(sfd, request, length);
        if (nread <= 0)
            break;
        request += nread;
        length -= nread;
    }

        // printf("in read_bytes");
    int total_read = 0;
    nread = 0;
    char temp_buf[MAXBUF] = {0};
    char temp[MAXBUF] = {0};
    while(1) {
		nread = recv(sfd, (p + total_read), MAXBUF, 0);
        total_read += nread;
        //  printf("nread = %d\n", nread);
		if (nread == -1) {
            printf("error: %s\n", strerror(errno));
			continue;     
        }         

        if (nread == 0){
            break;
        }
    }
    return total_read;
}

forward_bytes_to_client(int sfd, char* request_to_forward, int length)
{
    ssize_t nread;
    // printf("length = %d\n", length);
    while (length > 0)
    {
        nread = write(sfd, request_to_forward, length);
        if (nread <= 0)
            printf("error: %s\n", strerror(errno));
            break;
        request_to_forward += nread;
        length -= nread;
    }
}

void send_request(int sfd, char* request, int length)
{
    ssize_t nread;
    // printf("in send_request \n");

    while (length > 0)
    {
        nread = write(sfd, request, length);
        if (nread <= 0)
            break;
        request += nread;
        length -= nread;
    }
    // printf("exiting send request\n");
    // printf("nread = %d\n", nread);
}

void *thread(void *vargp) 
{  
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self()); 
    char request[MAXBUF] = {0};
    char type[BUFSIZ] = {0};
    char host[BUFSIZ];
    char port[BUFSIZ];
    char resource[BUFSIZ] = {0};
    char protocal[BUFSIZ] = {0};
    char version[BUFSIZ] = {0};
    char host_and_port[MAXBUF] = {0};
    int sfd = 0;

    int req_val = 0;
    // printf("in thread function\n");
    read_bytes(connfd,request);
    // printf("%s\n", request);

    req_val = parse_request(request, type, protocal, host, port, resource, version);
    // printf("request = %s\n", request);
    // printf("type = %s\n", type);
    // printf("protocal = %s\n", protocal);
    // printf("host = %s\n", host);
    // printf("port = %s\n", port);
    // printf("resource = %s\n", resource);
    // printf("version = %s\n\n", version);

    // make_host_and_port(host_and_port, host, port);
    char* port_pointer = port;

    // printf("host_and_port is now = %s\n", host_and_port);



    if (req_val == 0){
        char new_request[MAXBUF] = {0};
        char* p = new_request;
        // snprintf(new_request, MAXBUF, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
        // type, " ", resource, version_hdr, end_line, host_init, host, colon, port, end_line,
        // user_agent_hdr, accept_line_hdr, connection_hdr, proxy_connection_hdr, end_line);

        strncat(new_request, type, BUFSIZ);
        strncat(new_request, " ", 2);
        strncat(new_request, resource, BUFSIZ);
        strncat(new_request, version_hdr, BUFSIZ);
        strncat(new_request, end_line,BUFSIZ);
        strncat(new_request, host_init, BUFSIZ);
        strncat(new_request, host, BUFSIZ);
        // if (strcmp(port, "80")){
        //     strncat(new_request, colon, BUFSIZ);
        //     strncat(new_request, port, BUFSIZ);
        // }
        // else {
        //     strncat(new_request, ":", 2);
        //     strncat(new_request, "80", 4);
        // }
                    strncat(new_request, colon, BUFSIZ);
            strncat(new_request, port, BUFSIZ);
        strncat(new_request, end_line, BUFSIZ);
        strncat(new_request, user_agent_hdr, BUFSIZ);
        strncat(new_request, accept_line_hdr, BUFSIZ);
        strncat(new_request, connection_hdr, BUFSIZ);
        strncat(new_request, proxy_connection_hdr, BUFSIZ);
        strncat(new_request, end_line, BUFSIZ);
        // printf("new_request: \n\n%s\n", p);
        int request_length = 0;
        request_length = strlen(new_request);

        // printf("request length = %d\n", request_length);
        // printf("request length = %d\n", strlen(request));
        char request_to_forward[MAXBUF];
        request_length = create_send_socket(sfd, port, host, new_request, request_length, request_to_forward);
        // send_request(sfd, new_request, request_length);
        // printf("going into read_bytes\n");
        // read_bytes_from_server(sfd, request_to_forward);
        // printf("request_to_forward = %s\n", request_to_forward);
        forward_bytes_to_client(connfd, request_to_forward, request_length);
    }
    else {
        // simply close connection and move on
    }

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

    listenfd = make_listening_socket(port);
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int)); 
        if ((*connfdp = accept(listenfd, (SA *) &clientaddr, &clientlen)) < 0){
            perror("Error with accepting connection!\n");
            break;
        }
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
