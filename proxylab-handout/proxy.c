#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include "csapp.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define SBUFSIZE 500
#define LOGSIZE 500
#define MAXEVENTS 64
#define READ_REQUEST 1
#define SEND_REQUEST 2
#define READ_RESPONSE 3
#define SEND_RESPONSE 4


/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char *host_init = "Host: ";
static const char *accept_line_hdr = "Accept */* \r\n";
static const char *end_line = "\r\n";
static const char *default_port = "80";
static const char *version_hdr = " HTTP/1.0";
static const char *colon = ":";

int efd;

struct request_info{
    int client_fd;
    int server_fd;
    int state;
    char buf[MAX_OBJECT_SIZE];
    int total_read_client;
    int total_write_server;
    int written_to_server;
    int read_from_server;
    int written_to_client;
};

struct request_info all_events[sizeof(struct request_info) * 64];
int size_of_all_events = sizeof(all_events)/sizeof(struct request_info);
struct epoll_event *events;


/* ------------------------------------- Cache functions -------------------------------------------------*/

typedef struct cache_node{
    char* url;
    char* response;
    int size;
    struct cache_node* next;
    struct cache_node* previous;
} cache_node;

typedef struct {
    int cache_size;
    int number_of_objects;
    struct cache_node* head;
} cache_list;

cache_list head_cache;


void cache_init(cache_list *cache) 
{
    cache->cache_size = 0;
    cache->number_of_objects = 0;
    cache->head = NULL;
}

void add_cache(cache_list *cache, char* url, char* content, int size)
{
    if (size > MAX_OBJECT_SIZE){
        return;
    }
    
    if (cache->cache_size + size > MAX_CACHE_SIZE){
        return;
    }

    cache->cache_size += size;
    cache->number_of_objects += 1;

    cache_node* new_node = malloc(sizeof(cache_node));
    new_node->next = cache->head;
    new_node->url = malloc(strlen(url) * sizeof(char));
    new_node->response = malloc(size);
    new_node->size = size;
    strcpy(new_node->url, url);
    memcpy(new_node->response, content, size);

    cache->head = new_node;

}

int cache_search(cache_list *cache, char* url)
{
    struct cache_node *current = cache->head;
    int result = 0;

    while(current){
        if (!strcmp(current->url, url)) { 
            result = 1;
        }
        current = current->next;
    }
    return result;
}

int get_data_from_cache(cache_list *cache, char* url, char* response)
{
    // printf("Getting data from cache\n");
    struct cache_node *current = cache->head;
    int result = 0;

    while(current){

        if (!strcmp(current->url, url)) { 
            memcpy(response, current->response, current->size);
            result = current->size;
            break;
        }
        current = current->next;
    }
    return result;
}

void free_cache(cache_list *cache)
{
    struct cache_node *current = cache->head;
    while(current){
        free(current->url);
        free(current->response);
        struct cache_node *temp = current->next;
        free(current);
        current = temp;
    }
}

/* ------------------------------------- Logger code -------------------------------------------------*/
FILE *logfile;

void print_to_log_file(char* url)
{
    char message[MAXBUF] = {0};
    char* p = message;
    time_t now;
    char time_str[MAXLINE] = {0};

    now = time(NULL);
    strftime(time_str, MAXLINE, "%d %b %H:%M:%S ", localtime(&now));
    strcpy(message, time_str);
    strcat(message, ">|  ");
    strcat(message, url);

    fprintf(logfile, "%s", p);
    fflush(logfile);
}

/* ------------------------------------- SIGINT handler -------------------------------------------------*/

void sigint_handler(int sig)  
{
    fprintf(logfile, "In sigint handler\n");
    free_cache(&head_cache);
    fclose(logfile);
    free(events);

    exit(0);
}

/*---------------------------------------- Epoll helper functions -----------------------------------------*/
int sfd;
struct sockaddr_in ip4addr;

void deregister_socket(struct request_info info)        //Code to deregister a socket from epoll and clean it in the datastructure
{
    for(int j = 0; j < size_of_all_events; j++){ 
        if (info.client_fd == all_events[j].client_fd || info.server_fd == all_events[j].server_fd){
            close(info.client_fd);
            close(info.server_fd);
            if(errno == EBADF){
                continue; //Just means one of the file descriptors possibly wasn't ready
            }
            
            info.state = 0;
            info.total_read_client = 0;
            info.total_write_server = 0;
            info.read_from_server = 0;
            info.written_to_client = 0;
            info.written_to_server = 0;
            info.client_fd = sfd;
            info.server_fd = -1;
            all_events[j] = info;

            j = size_of_all_events;
        }
    }
}

void make_url(char* url, char* protocal, char* port, char* host, char* resource)
{
    char final_url[MAXLINE] = {0};
    char* p = final_url;

    strcpy(final_url, protocal);
    strcat(final_url, "://");
    strcat(final_url, host);
    if(strcmp(port, "80")){
        strcat(final_url, colon);
        strcat(final_url, port);
    }
    strcat(final_url, resource);
    strcat(final_url, "\n\0");

    strcpy(url, p);
}

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
	if(strlen(request) == 0) {
        return -1;
    }

	if(!strstr(request, "/")){ 
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

void create_and_register_send_socket(int sfd, char* port, char* host, struct request_info current_event)
{
	struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          
    s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
        if (sfd == -1)
    continue;
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break;                  
        close(sfd);
    }
    if (rp == NULL) {               
        fprintf(stderr, "Could not connect\n");
    }
    freeaddrinfo(result);     

    if (fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK) < 0) { //Set as non-blocking
		fprintf(stderr, "error setting socket option\n");
		exit(1);
	}     

    //Register the socket with epoll
   	struct epoll_event event;
    event.events = EPOLLOUT; 
    event.data.fd = sfd;
    epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
    current_event.state = SEND_REQUEST;
    current_event.server_fd = sfd;
    return;
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

/* ------------------------------------- Epoll states -------------------------------------------------*/

void init_request_info(struct request_info info, int sfd)
{
    info.state = READ_REQUEST;
    info.total_read_client = 0;
    info.total_write_server = 0;
    info.read_from_server = 0;
    info.written_to_client = 0;
    info.written_to_server = 0;
    info.client_fd = sfd;
    info.server_fd = -1;

    return;
}

void read_request_state(struct request_info current_event) //TEST
{
    printf("read_request_state() function called!\n");
    
    char request[MAXBUF] = {0};
    char type[BUFSIZ] = {0};
    char host[BUFSIZ];
    char port[BUFSIZ];
    char resource[BUFSIZ] = {0};
    char protocal[BUFSIZ] = {0};
    char version[BUFSIZ] = {0};
    char url[BUFSIZ] = {0};
    int sfd = 0;
    int req_val = 0;

    int total_read = 0;
    char temp_buf[MAXBUF] = {0};
    char* p = current_event.buf;

    while(1) {
        ssize_t nread = 0;
		
        nread = recv(current_event.client_fd, (p + total_read), MAXBUF, 0);
        total_read += nread;
        strcpy(temp_buf, p + total_read - 4);
        
        if (errno == EWOULDBLOCK || errno == EAGAIN){
            continue; //DON'T CHANGE STATE, continue reading until notified by epoll that there is more data
        }
        else { //cancel client request and deregister your socket
        	deregister_socket(current_event);
        	return;
        }
        if(!strcmp(temp_buf, "\r\n\r\n")){
            break;
        }
	}

    current_event.total_read_client = total_read

    req_val = parse_request(request, type, protocal, host, port, resource, version);
    // printf("request = %s\n", request);
    // printf("type = %s\n", type);
    // printf("protocal = %s\n", protocal);
    // printf("host = %s\n", host);
    // printf("port = %s\n", port);
    // printf("resource = %s\n", resource);
    // printf("version = %s\n\n", version);
    make_url(url, protocal, port, host, resource);
    print_to_log_file(url);


    if (req_val == 0){
        int is_in_cache = cache_search(&head_cache, url);
        char request_to_forward[MAX_OBJECT_SIZE];
        // printf("url = %s\n", url);
        // printf("in cache = %d\n", is_in_cache);

        if(is_in_cache){
            printf("Is in cache\n");
            current_event.read_from_server = get_data_from_cache(&head_cache, url, request_to_forward);
            current_event.buf = request_to_forward;
            current_event.state = SEND_RESPONSE;
            return;
        }
        else{
            // char* p = new_request;
            char new_request[MAXBUF] = {0};

            strncat(new_request, type, BUFSIZ);
            strncat(new_request, " ", 2);
            strncat(new_request, resource, BUFSIZ);
            strncat(new_request, version_hdr, BUFSIZ);
            strncat(new_request, end_line,BUFSIZ);
            strncat(new_request, host_init, BUFSIZ);
            strncat(new_request, host, BUFSIZ);
            strncat(new_request, colon, BUFSIZ);
            strncat(new_request, port, BUFSIZ);
            strncat(new_request, end_line, BUFSIZ);
            strncat(new_request, user_agent_hdr, BUFSIZ);
            strncat(new_request, accept_line_hdr, BUFSIZ);
            strncat(new_request, connection_hdr, BUFSIZ);
            strncat(new_request, proxy_connection_hdr, BUFSIZ);
            strncat(new_request, end_line, BUFSIZ);
            // printf("new_request: \n\n%s\n", p);
            
            current_event.total_write_server = strlen(new_request);
            //Set up a new socket, and use it to connect() to the web server
            //Register the socket with the epoll instance for writing
            create_and_register_send_socket(sfd, port, host, current_event); 
        }

    }
    else {
        // simply close connection and move on
    }

    return;
}

void send_request_state(struct request_info current_event) //TEST
{
    printf("send_request_state() function called!\n");

    int total_read = current_event.written_to_server;
    int nread = 0;
    while (current_event.total_write_server > 0)
    {
        nread = write(current_event.server_fd, current_event.buf, current_event.total_read_client);

        if (nread == 0){
        	struct epoll_event event;  //register socket for reading
            event.events = EPOLLIN; 
            event.data.fd = current_event.server_fd;
            epoll_ctl(efd, EPOLL_CTL_ADD, current_event.server_fd, &event);
        	current_event.state = READ_RESPONSE; //Change state
            break;
        }
        else if (errno == EWOULDBLOCK || errno == EAGAIN){
            continue; //DON'T CHANGE STATE, continue reading until notified by epoll that there is more data
        }
        else{
            deregister_socket(current_event);
            return;
        }
        current_event.written_to_server += nread;
        current_event.total_write_server -= nread;
    }
    
    return;
}

void read_response_state(struct request_info current_event) //TEST
{
    printf("read_response_state() function called!\n");

    int total_read = current_event.read_from_server;
    ssize_t nread = 0;
    char* p = current_event.buf

    while(1) {
		nread = recv(sfd, (p + total_read), MAXBUF, 0);
        total_read += nread;
		if (nread == -1) {
            printf("error: %s\n", strerror(errno));
			continue;     
        }         

        if (nread == 0){
            struct epoll_event event;  //register socket for writing
            event.events = EPOLLOUT; 
            event.data.fd = current_event.client_fd;
            epoll_ctl(efd, EPOLL_CTL_ADD, current_event.client_fd, &event);
        	current_event.state = SEND_RESPONSE; //Change state
            break;
        }
        else if (errno == EWOULDBLOCK || errno == EAGAIN){
            continue; //DON'T CHANGE STATE, continue reading until notified by epoll that there is more data
        }
        else{
            deregister_socket(current_event);
            return;
        }
        current_event.read_from_server += nread;
    }

    return;
}

void send_response_state(struct request_info current_event) //TEST
{
    printf("send_response_state() function called!\n");

    int total_read = current_event.read_from_server;
    int nread = 0;
    while (current_event.read_from_server > 0)
    {
        nread = write(current_event.server_fd, current_event.buf, current_event.read_from_server);

        if (nread == 0){
            break;
        }
        else if (errno == EWOULDBLOCK || errno == EAGAIN){
            continue; //DON'T CHANGE STATE, continue reading until notified by epoll that there is more data
        }
        else{
            deregister_socket(current_event);
            return;
        }
        current_event.written_to_client += nread;
        current_event.read_from_server -= nread;
    }
    deregister_socket(current_event); // close file descriptors, and we are done!

    return;
}

/* ------------------------------------- main() -------------------------------------------------*/
int main(int argc, char *argv[])
{
    int port;
    if (argc < 2){
	    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    	exit(0);
    }
    // signal(SIGPIPE, SIG_IGN);
    // signal(SIGINT, sigint_handler);

    port = atoi(argv[1]);

    int listenfd, connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
	struct epoll_event event;
	size_t n; 
	int i;
	logfile = fopen("proxy_log.txt", "w"); //open logger file

    listenfd = make_listening_socket(port);
    cache_init(&head_cache);    //Initiate cache

	// set fd to non-blocking (set flags while keeping existing flags)
	if (fcntl(listenfd, F_SETFL, fcntl(listenfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
		fprintf(stderr, "error setting socket option\n");
		exit(1);
	}

	if ((efd = epoll_create1(0)) < 0) {
		fprintf(stderr, "error creating epoll fd\n");
		exit(1);
	}

    event.data.fd = listenfd;
	event.events = EPOLLIN | EPOLLET; 
	if (epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &event) < 0) {
		fprintf(stderr, "error adding event\n");
		exit(1);
	}

    for(int j = 0; j < size_of_all_events; j++){ //initialize datastructure
        struct request_info initialized_struct;
        initialized_struct.state = 0;
        all_events[j] = initialized_struct;
    }

	events = calloc(MAXEVENTS, sizeof(event));

	while (1) {
		// wait for event to happen (timeout of 1 second)
		n = epoll_wait(efd, events, MAXEVENTS, 1);

        if(n < 0){ //If error
		    fprintf(stderr, "Error with epoll_wait\n");
        }
        else if(n == 0){ //If timeout
        	fprintf(stderr, "Timeout with epoll_wait\n");
        }

		for (i = 0; i < n; i++) { // Loop through all events
			if ((events[i].events & EPOLLERR) ||
					(events[i].events & EPOLLHUP) ||
					(events[i].events & EPOLLRDHUP)) {
                //Close things here
				fprintf (stderr, "epoll error on fd %d\n", events[i].data.fd);
				close(events[i].data.fd);
				continue;
			}

			if (listenfd == events[i].data.fd) {
				clientlen = sizeof(struct sockaddr_storage); 

				// loop and get all the connections that are available
				while ((connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen)) > 0) {

					if (fcntl(connfdp, F_SETFL, fcntl(connfdp, F_GETFL, 0) | O_NONBLOCK) < 0) {
						fprintf(stderr, "error setting socket option\n");
						exit(1);
					}
					event.data.fd = connfdp;
					event.events = EPOLLIN | EPOLLET; 
					if (epoll_ctl(efd, EPOLL_CTL_ADD, connfdp, &event) < 0) {
						fprintf(stderr, "error adding event\n");
						exit(1);
					}
                    struct request_info new_event;
                    init_request_info(new_event, connfdp)

                    for(int j = 0; j < size_of_all_events; j++){ //Add event to the data structure
                        if(all_events[j].state == 0){
                            all_events[j] = new_event;
                            j = size_of_all_events;
                        }
                    }

				}

				if (errno == EWOULDBLOCK || errno == EAGAIN) {
					// no more clients to accept()
				} else {
					perror("error accepting");
				}

			} else { //corresponds to the client socket

                struct request_info current_event;

                for(int j = 0; j < size_of_all_events; j++){ //Find correct event that is up to bat
                    if (events[i].data.fd == all_events[j].client_fd || events[i].data.fd == all_events[j].server_fd){
                        current_event = all_events[j]
                        j = size_of_all_events;
                    }
                }

                switch(current_event.state) {
                    case READ_REQUEST:
                        read_request_state(current_event);
                        break;
                    case SEND_REQUEST:
                        send_request_state(current_event);
                        break;
                    case READ_RESPONSE:
                        read_response_state(current_event);
                        break;
                    case SEND_RESPONSE:
                        send_response_state(current_event);
                        break;                                                
                    default : 
                        fprintf(stderr, "Error: State of current event not valid ");
                }

			}
		}
	}
	free(events);

    return 0;
}



