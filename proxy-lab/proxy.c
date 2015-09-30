/*
* A Web proxy that acts as intermediate between 
* Web browser and the Web.
*
* Authors: Li Pei 
* Andrew ID : lip
*
* Note: handles only GET requests
*  
* Model :
*  ________                _______                 ________
* |        | -> Content ->|       | -> Content -> |        |
* | server |              | proxy |               | client |
* |________| <- request <-|_______| <- Request <- |________|
*                          _|___|_  
*                         |       |
*                         | cache |
*                         |_______|          
*
* Proxy Function: 
* Get Request from client, if the URL has visit before, get 
* content from server and send it back, if not, send request  
* to server. Once received requested content from server, 
* save it in the cache, and send back to client.
*
*/

#include <stdio.h>
#include "csapp.h"
#include "cache.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";

void *doit(void *arg);
void serve(int connfd);
void parse_url(char *url, char *host, int *port, char *path, char *cgiargs);
void read_headers(rio_t *rp, char* host_header, char *other_headers);
int external_header(char* browser_request);
void make_request(int fd, char *url, char *host, char *path, char *host_header, char *other_headers, char* host_port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);


// information about client
typedef struct clientInfo_t{
    struct sockaddr_in clientaddr;
    int connfd;

}clientInfo_t;
// initialize proxy cache
cache_LL* cache;

/*
* Main function
*/
int main(int argc, char **argv)
{

    printf("%s%s%s", user_agent_hdr, accept_hdr, accept_encoding_hdr);

    int listenfd;
    int clientlen;
    struct clientInfo_t *clientInfo;

    //Thread ID
    pthread_t tid;

    //initialize proxy cache
    cache = (cache_LL*) Calloc(1, sizeof(cache_LL));
    cache->head = NULL;
    cache -> tail = NULL;
    cache->size = 0;
    cache_init();

    // Check command line args
    Signal(SIGPIPE, SIG_IGN);

    // If there is no proxy port preset, fault!
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // Get proxy listen-file-descriptor
    listenfd = Open_listenfd(argv[1]);

    //Iterative server
    while(1)
    {
        clientInfo = Malloc(sizeof(struct clientInfo_t));

        clientlen = sizeof(struct sockaddr_in);
        // Build connection
        clientInfo -> connfd = Accept(listenfd,(SA *)&clientInfo -> clientaddr,(socklen_t *)&clientlen);
        // Create new thread for this request
        Pthread_create(&tid, NULL, doit, (void *)clientInfo);
    }
    return 0;
}

/*
* serve the request from client with a thread
*/
void *doit(void *arg)
{
  // rebuild client info  
  struct clientInfo_t *clientInfo;

  clientInfo = (struct clientInfo_t*)arg;

  // sent the connect file descriptor
  int connfd = clientInfo -> connfd;

  // detach the working thread
  Pthread_detach(pthread_self());

  Free(arg);

  // Print client information
  struct sockaddr_in clientaddr = clientInfo -> clientaddr;
  unsigned short client_port = ntohs(clientaddr.sin_port);
  char *client_IP = inet_ntoa(clientaddr.sin_addr);
  printf("proxy request from %s:%hu \n",client_IP,client_port);
  
  // serve the client request
  serve(connfd);
  
  // after serve, close connection
  Close(connfd);
  return NULL;
}


/*
* Serves a client's request.
* connfd is the file descriptor
*/
 void serve(int connfd)
 {
    // Obatain the method : "GET http://host:port/content"
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char buf[MAXLINE], host[MAXLINE];
    char path[MAXLINE], cgiargs[MAXLINE];
    char host_header[MAXLINE], other_headers[MAXLINE];

    // set the robust IO file descriptor
    rio_t rio;

    // Read in a request from client
    Rio_readinitb(&rio, connfd); // initialize rio buffer with file descriptor
    Rio_readlineb(&rio, buf, MAXLINE); // read in line

    // move from buffer into string format
    sscanf(buf, "%s %s %s", method, url, version);

    // if the method is not "GET", we can not do anything
    if(strcasecmp(method,"GET"))
    {
        clienterror(connfd,method,"501","Not Implemented","Proxy does not implement this method");    
        return;
    }

    // read the header
    read_headers(&rio, host_header, other_headers);

    // store current url
    char url_store[MAXLINE];
    strcpy(url_store, url);

    //Default server port 
    int port = 80;
    // Parse URL out of request
    parse_url(url_store, host, &port, path, cgiargs);
    printf("HOST = %s,HOST PORT = %d\n",host,port);

    // get the host port
    char host_port[MAXLINE];
    sprintf(host_port,"%d",port);
    // get request content from server or proxy cache
    make_request(connfd, url, host, path, host_header, other_headers, host_port);
 }

/*
* Reads in and ignores headers of requests.
*/
void read_headers(rio_t *rp, char *host_header, char *other_headers)
{
   char buf[MAXLINE];
   strcpy(other_headers, "");
   Rio_readlineb(rp, buf, MAXLINE);
   //if we are not get "\r\n", keep reading
   while(strcmp(buf, "\r\n"))
   {
       Rio_readlineb(rp, buf, MAXLINE);
       // get other external header
       if (external_header(buf))
       {
            strcat(other_headers,buf);
       }
   }
   
}

/*
* Parses URL from GET request into hostname and path
*/
void parse_url(char *url, char *host,int *port, char *path, char *cgiargs)
{
    char *path_ptr;
    memset(host, 0, sizeof(host));
    memset(path, 0, sizeof(path));

    sscanf(url, "http://%s", url);

    // Get whole path
    if ((path_ptr = strchr(url, '/')) == NULL)
    {
        strcpy(cgiargs, "");
        path[0] = '\0';
    }
    else
    {
        strcpy(path, path_ptr);
        path_ptr[0] = '\0';
    }
    strcpy(host, url);
    
    // Get request port
    char *port_ptr;

    if ((port_ptr = strchr(host, ':')) != NULL)
    {

        *port = atoi(port_ptr+1);
        *port_ptr = '\0';
    }
    return;
}

/* Make request create request to server, get content and sent to client.
* if the URL is in proxy's cache, just send the content back to client, if 
* not, we have to request it in the 
* server, and sent it back to client.
*/
void make_request(int fd, char *url, char *host,
    char *path, char *host_header, char *other_headers, char* host_port)
{

    // check if the URL have been saved in cahce
    web_object* found = checkCache(cache, url);
    //If the object is found, write the data back to the client
    if(found != NULL) {
        Rio_writen(fd, found->data, found->size);
        return;
    }

    // proxy now is the client of server
    int client_fd;
    char buf[MAXBUF], reply[MAXBUF];
    rio_t rio;
    client_fd = Open_clientfd(host, host_port);
    if (client_fd <= -1)
    {
        clienterror(fd, host, "DNS","DNS ERROR","DNS Error : No Available Connection!");
        return;
    }

    //The following code adds the necessary information to make buf a complete request
    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    if (!strlen(host_header))
        sprintf(buf, "%sHost: %s\r\n", buf, host);
    else
        sprintf(buf, "%sHost: %s\r\n", buf, host_header);
    sprintf(buf,"%s%s%s%s",buf,user_agent_hdr,accept_hdr,accept_encoding_hdr);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sProxy-Connection: close\r\n", buf);
    sprintf(buf, "%s%s\r\n", buf, other_headers);
    
    Rio_writen(client_fd, buf, strlen(buf));
    Rio_readinitb(&rio, client_fd);

    // write content to cache and client
    int read_return;
    char cache_object[MAX_OBJECT_SIZE];
    int cache_object_size = 0;
    // read from client until end
    while((read_return = Rio_readnb(&rio, reply, MAXBUF)) > 0)
    {
        // write to client
        Rio_writen(fd, reply, read_return);
        // store it in a temporary cache_object string
        if (cache_object_size + read_return < MAX_OBJECT_SIZE)
        {
            char *temp = cache_object + cache_object_size;
            memcpy(temp, reply, read_return);
            cache_object_size += read_return;
        }
        memset(reply, 0, sizeof(reply));
    }

    // write to cache
    // if the cache_object_size < Max allowed object size
    // write it into cache
    if (cache_object_size < MAX_OBJECT_SIZE)
    {
        addToCache(cache, cache_object, url, cache_object_size);
    }
    Close(client_fd);
}
/*
* judge whether a request have external head
*/
int external_header(char* browser_request)
{
    if(strstr(browser_request,"User-Agent:"))
    {
        return 0;
    }
    else if(strstr(browser_request,"Accept:"))
    {
        return 0;
    }
    else if(strstr(browser_request,"Accept-Language:"))
    {
        return 0;
    }
    else if(strstr(browser_request,"Accept-Encoding:"))
    {
        return 0;
    }
    else if(strstr(browser_request,"Connection:"))
    {
        return 0;
    }
    else if(strstr(browser_request,"Proxy-Connection:"))
    {
        return 0;
    }else{
        return 1;    
    }
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
         char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}