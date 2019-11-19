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
#define NTHREADS 30
#define SBUFSIZE 500
#define LOGSIZE 500

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

/* ------------------------------------- Cache functions -------------------------------------------------*/
sem_t write_sem;
sem_t count_sem;
int readers;

typedef struct {
    char* url;
    char* response;
    char* size;
    struct cache_node* next;
    struct cache_node* previous;
} cache_node;

typedef struct {
    int cache_size;
    int number_of_objects;
    cache_node* head;
} cache_list;

cache_list *head_cache;


void cache_init(cache_list *cache) 
{
    sem_init(&write_sem, 0, 1);
    sem_init(&count_sem, 0, 1);
    readers = 0;

    cache->cache_size = 0;
    cache->number_of_objects = 0;
    cache->head = NULL;
}

void add_cache(cache_list *cache, char* url, char* content, int size)
{
    P(&write_sem);
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

    V(&write_sem);
}

int cache_search(cache_list *cache, char* url)
{
    P(&count_sem);
    readers++;
    if (readers == 1) 
        P(&write_sem);
    V(&count_sem);        

    cache_node *current;
    int result = 0;

    for (current = cache->head; current != NULL; current = current->next) {
        if (!strcmp(current->url, url)) {
            result = 1; 
        }
    }

    P(&count_sem);
    readers--;
    if (readers == 0) 
        V(&write_sem); 
    V(&count_sem);

    return result;
}

int get_data_from_cache(cache_list *cache, char* url, char* response)
{
    P(&write_sem);
    cache_node *current;
    int result = 0;

    while(current){
        if (!strcmp(current->url, url)) { 
            memcpy(response, current->response, current->size);
            result = current->size;
            break;
        }
        current = current->next;
    }
    V(&write_sem); 
    return result;
}


/* ------------------------------------- Thread pool code -------------------------------------------------*/
typedef struct {
    int *buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;

sbuf_t sbuf;


void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = calloc(n, sizeof(int)); 
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
    Sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */
    Sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
    Sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
}

void sbuf_deinit(sbuf_t *sp)
{
    free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);                          /* Wait for available slot */
    P(&sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item;   /* Insert the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->items);                          /* Announce available item */
}

int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);                          /* Wait for available item */
    P(&sp->mutex);                          /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
    V(&sp->mutex);                          /* Unlock the buffer */
    V(&sp->slots);                          /* Announce available slot */
    return item;
}

/* ------------------------------------- Logger code -------------------------------------------------*/
FILE *logfile;

typedef struct {
    char **buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} log_t;

log_t log_buf;


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

void log_init(log_t *log, int n)
{
    log->buf = calloc(n, sizeof(int)); 
    log->n = n;                       /* Buffer holds max of n items */
    log->front = log->rear = 0;        /* Empty buffer iff front == rear */
    Sem_init(&log->mutex, 0, 1);      /* Binary semaphore for locking */
    Sem_init(&log->slots, 0, n);      /* Initially, buf has n empty slots */
    Sem_init(&log->items, 0, 0);      /* Initially, buf has zero data items */
}

void log_deinit(log_t *log)
{
    free(log->buf);
}

void log_insert(log_t *log, char* item)
{
    P(&log->slots);                          /* Wait for available slot */
    P(&log->mutex);                         /* Lock the buffer */
    log->buf[(++log->rear)%(log->n)] = calloc(strlen(item), sizeof(char*));  
    strcpy(log->buf[(log->rear)%(log->n)], item);           /* Insert the item */
    V(&log->mutex);                          /* Unlock the buffer */
    V(&log->items);                          /* Announce available item */
}

char* log_remove(log_t *log)
{
    char* item;
    P(&log->items);                          /* Wait for available item */
    P(&log->mutex);                          /* Lock the buffer */
    item = log->buf[(++log->front)%(log->n)];  /* Remove the item */
    V(&log->mutex);                          /* Unlock the buffer */
    V(&log->slots);                          /* Announce available slot */
    return item;
}

void* logger(void* vargp)
{
    Pthread_detach(pthread_self());
    logfile = fopen("proxy_log.txt", "w");
    log_init(&log_buf, LOGSIZE);

    while(1){
        char* url = log_remove(&log_buf);
        print_to_log_file(url);
        free(url);
    }
    // log_deinit(&log_buf);
}


/*---------------------------------------- Proxy code -----------------------------------------*/
int sfd;
struct sockaddr_in ip4addr;

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

int read_bytes(int fd, char* p)
{
    int total_read = 0;
    ssize_t nread = 0;
    char temp_buf[MAXBUF] = {0};
    while(1) {
		nread = recv(fd, (p + total_read), MAXBUF, 0);
        total_read += nread;
        strcpy(temp_buf, p + total_read - 4);
		if (nread == -1) {
            fprintf(stdout, "Error opening file: %s\n", strerror( errno ));
			continue;     
        }         

        if (nread == 0){
            break;
        }
        
        if(!strcmp(temp_buf, "\r\n\r\n")){
            break;
        }
	}
    return total_read;
}

int create_send_socket(int sfd, char* port, char* host, char* request, int length, char* p)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;


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

    ssize_t nread = 0;
    
    sleep(1);
    while (length > 0)
    {
        nread = write(sfd, request, length);

        if (nread <= 0)
            break;
        request += nread;
        length -= nread;
    }

    int total_read = 0;
    nread = 0;
    while(1) {
		nread = recv(sfd, (p + total_read), MAXBUF, 0);
        total_read += nread;
		if (nread == -1) {
            printf("error: %s\n", strerror(errno));
			continue;     
        }         

        if (nread == 0){
            break;
        }
    }
    // sleep(1);
    return total_read;
}

void forward_bytes_to_client(int sfd, char* request_to_forward, int length)
{
    ssize_t nread;
    // printf("length = %d\n", length);
    while (length > 0)
    {
        nread = write(sfd, request_to_forward, length);
        if (nread <= 0){
            printf("error: %s\n", strerror(errno));
            break;
        }
        request_to_forward += nread;
        length -= nread;
    }
    return;
}

void send_request(int sfd, char* request, int length)
{
    ssize_t nread;

    while (length > 0)
    {
        nread = write(sfd, request, length);
        if (nread <= 0)
            break;
        request += nread;
        length -= nread;
    }
    return;
}

void run_proxy(int connfd) 
{  
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

    read_bytes(connfd,request);

    req_val = parse_request(request, type, protocal, host, port, resource, version);
    // printf("request = %s\n", request);
    // printf("type = %s\n", type);
    // printf("protocal = %s\n", protocal);
    // printf("host = %s\n", host);
    // printf("port = %s\n", port);
    // printf("resource = %s\n", resource);
    // printf("version = %s\n\n", version);
    make_url(url, protocal, port, host, resource);
    // print_to_log_file(url);
    log_insert(&log_buf, url);

    if (req_val == 0){
        char new_request[MAXBUF] = {0};

        if(cache_search(head_cache,url)){
            
        }
        else{
            // char* p = new_request;

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

            
        }
        int request_length = 0;
        request_length = strlen(new_request);
        char request_to_forward[102400];
        int response_length = create_send_socket(sfd, port, host, new_request, request_length, request_to_forward);            forward_bytes_to_client(connfd, request_to_forward, response_length);
    }
    else {
        // simply close connection and move on
    }

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

void *thread(void *vargp)
{
    Pthread_detach(pthread_self());
    while(1){
        int connection = sbuf_remove(&sbuf);
        run_proxy(connection);
        Close(connection);
    }
    return NULL;
}


void make_client(int port)
{
    int listenfd, connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid; 

    listenfd = make_listening_socket(port);
    sbuf_init(&sbuf, SBUFSIZE);

    for (int i = 1; i < NTHREADS; i++)  { /* Create worker threads */
        Pthread_create(&tid, NULL, thread, NULL);
    }

    Pthread_create(&tid, NULL, logger, NULL);

    cache_init(head_cache);

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        if ((connfdp = accept(listenfd, (SA *) &clientaddr, &clientlen)) < 0){
            perror("Error with accepting connection!\n");
            break;
        }
        sbuf_insert(&sbuf, connfdp);
    }
    sbuf_deinit(&sbuf);
    Close(listenfd);
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
