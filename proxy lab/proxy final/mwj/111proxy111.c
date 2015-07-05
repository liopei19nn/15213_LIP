/*
 * Proxy lab
 * a simple proxy - accept get request from the client and forward
 * request to the client. This proxy also implements a simmple cache
 * that stores the most recent accessed data. 
 *
 * Name: 		Wenjie Ma
 * AndrewID: 	wenjiem
 */
#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";

/* self defined helper functions */
void doit(int fd);
void read_requesthdrs(rio_t *rp, char *requestbuf, char *hostname);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);
int get_hostport(char *uri, char *hostname, int *port, char *path);
void contact_server(char *host, int port, int client_fd, 
	char *requestbuf, char *uri);
void *thread(void *vargp);

cache* my_cache;

int main(int argc, char **argv)
{
    printf("%s%s%s", user_agent_hdr, accept_hdr, accept_encoding_hdr);

    // same routine as the echo server example on the CSAPP book
    int listenfd, *connfd, clientlen;
    struct sockaddr_in clientaddr;
    pthread_t tid;
    my_cache = (cache*)Malloc(sizeof(cache));
    my_cache->head = NULL;
	my_cache->size = 0;
    init_cache(my_cache);

    // ignore signal
    Signal(SIGPIPE, SIG_IGN);

    if (argc != 2) {
    	fprintf(stderr, "usage: %s <port>\n", argv[0]);
    	exit(1);
    }

    listenfd = open_listenfd(argv[1]);
    if (listenfd < 0) {
    	printf("Open error");
    	return -1;
	}

    while(1) {
    	connfd = malloc(sizeof(int));
    	clientlen = sizeof(clientaddr);
    	*connfd = accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
    	if (*connfd < 0) {
    		printf("Accept error!");
    		return -1;
    	}

    	pthread_create(&tid, NULL, thread, connfd);
    	//doit(connfd);
    	//Close(connfd);
    }

    return 0;
}

/*
 * thread - function that calls doit
 */
void *thread(void *vargp)
{
	int connfd = *((int *)vargp);
	pthread_detach(pthread_self());
	free(vargp);
	doit(connfd);
	return NULL;
}

/*
 * doit - function that take an client file descriptor 
 * 		and process the client request
 */
void doit(int fd) 
{
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char requestbuf[MAX_OBJECT_SIZE];
	char hostname[MAXLINE], path[MAXLINE];
	int port;
    rio_t rio;

    // initiate data
    memset(buf, 0, sizeof(buf));
    memset(method, 0, sizeof(method));
    memset(uri, 0, sizeof(uri));
    memset(version, 0, sizeof(version));
    memset(requestbuf, 0, sizeof(requestbuf));
    memset(hostname, 0, sizeof(hostname));
    memset(path, 0, sizeof(path));

    //printf("\nPROXY >> ACCEPTING cache is now:\n");
    //print_cache(my_cache);
    Rio_readinitb(&rio, fd);
    Rio_readlineb(&rio, buf, MAXLINE);


    //printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version); 
    

    if (strcasecmp(method, "GET")) {             
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        Close(fd);
        return;
    }

    get_hostport(uri, hostname, &port, path);
    sprintf(requestbuf, "GET %s HTTP/1.0\r\n", path);
    //printf("\nPROXY >> Returned from gethostport\n");

    read_requesthdrs(&rio, requestbuf, hostname);
    //printf("\nPROXY >> Returned from read_requesthdrs\n", requestbuf);

    node* found = find_node(my_cache, uri);
    if (found != NULL) {
    	//printf("\nPROXY >> Found node, writing back to client\n");
    	if (rio_writen(fd, found->data, found->size) < 0) {
    		printf("rio written error");
    		return;
    	}
    	Close(fd);
    	return;
    }

    //printf("Entering contact_server, port is: %d\n\n", port);
    contact_server(hostname, port, fd, requestbuf, uri);
    Close(fd);
}

/*
 * contact_server - contact the server using clients' rquest 
 * 				and send the data back to the client, while 
 * 				store data in the cache
 */
void contact_server(char *host, int port, int client_fd, 
	char *requestbuf, char *uri)
{
	int server_fd, length, len;
	rio_t rio;
	char buf[MAXLINE], tmpport[MAXLINE], server_response[MAX_OBJECT_SIZE];

	// initialize data
	memset(server_response, 0, sizeof(server_response));
	memset(buf, 0, sizeof(buf));
	memset(tmpport, 0, sizeof(tmpport));

	// convert int type to string
	sprintf(tmpport, "%d", port);

	// does not exit proxy when open client fd fails
	server_fd = open_clientfd(host, tmpport);
	if (server_fd < 0) {
		printf("Open clientfd error!");
		return;
	}

	Rio_readinitb(&rio, server_fd);
	if (rio_writen(server_fd, requestbuf, strlen(requestbuf)) < 0){
		printf("rio written error");
		return;
	} 

	//printf("writing back to client\n");

	length = 0;
	// send back to client
	while ((len= Rio_readnb(&rio, buf, MAXLINE)) > 0) {
		
		//printf("buf line :%s\n", buf);
		if (rio_writen(client_fd, buf, len) < 0){
    		printf("rio written error");
    		return;
    	}

		// prevent overflow
		if (length + len < MAX_OBJECT_SIZE) {
			//sprintf(server_response,"%s%s", server_response, buf);
			//strcat(server_response, buf);
			// must use memcpy here for difference encoding between server
			// and local memory
			char *temp = server_response + length;
			memcpy(temp, buf, len);
			length += len;
		}
		memset(buf, 0, sizeof(buf));

	}

	if (length < MAX_OBJECT_SIZE) {
		//printf("\nAdding to cache . . . \n");
		add_cache(my_cache, uri, server_response, length);
		//print_cache(my_cache);
		//printf("\nAdding done . . . \n");
	}

	//printf("\n\nresopnse: %s\n\n", server_response);
	close(server_fd);
}

/*
 * get_hostport - parse the uri from the client and extracts
 *   			host name, port number and path infomation
 */
int get_hostport(char *uri, char *hostname, int *port, char *path)
{
	char *hoststart, *hostend, *tmphost;
	char tmp[MAXLINE];
	strncpy(tmp, uri, strlen(uri) + 1);

	//printf("uri is %s\n", uri);

	if ((hoststart = strstr(tmp, "http://")) == NULL)
		return -1;
	hoststart += strlen("http://");
	//printf("hoststart is %s\n", hoststart);

	if ((hostend = strstr(hoststart, "/")) == NULL)
		return -1;

	// get path
	char *ppath;
	if((ppath = strchr(hoststart, '/')) == NULL)
		strncpy(path, "/", strlen("/"));
	else
		strncpy(path, ppath, strlen(ppath));
	//printf("ppath is %s\n", ppath);
	//printf("path is %s\n", path);

	*hostend = '\0';

	tmphost = strsep(&hoststart, ":");

	// check of contains port num
	if (hoststart == NULL)
		*port = 80;
	else
		*port = atoi(hoststart);

	strncpy(hostname, tmphost, strlen(tmphost));

	return 0;

}

/*
 * read_requesthdrs - read the request header from client and 
 * 				build a new header
 */
void read_requesthdrs(rio_t *rp, char *requestbuf, char *hostname) 
{
	char buf[MAXLINE];

	memset(buf, 0, sizeof(buf));

	// build host header
	strcat(requestbuf, "Host: ");
	strcat(requestbuf, hostname);
	strcat(requestbuf, "\r\n");

	// build other headers
	strcat(requestbuf, accept_hdr);
	strcat(requestbuf, accept_encoding_hdr);
	strcat(requestbuf, user_agent_hdr);
	strcat(requestbuf, "Connection: close\r\n");
	strcat(requestbuf, "Proxy-Connection: close\r\n");

	// add other headers
	Rio_readlineb(rp, buf, MAXLINE);
	while (strcmp(buf, "\r\n")) {
		//printf("client req: %s\n", buf);
		if (strstr(buf, "Host:")) 
			;	
		else if (strstr(buf, "Accept:")) 
			;
		else if (strstr(buf, "Accept-Encoding:"))
			;
		else if (strstr(buf, "User-Agent:"))
			;
		else if (strstr(buf, "Connection:"))
			;
		else if (strstr(buf, "Proxy-Connection:"))
			;

		else //if header unspecified, forward them unchanged
			strcat(requestbuf, buf);

		Rio_readlineb(rp, buf, MAXLINE);
	}

	strcat(requestbuf, "\r\n");
}

/*
 * clienterror - prints error information
 */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
