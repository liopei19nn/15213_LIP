/*
* A Web proxy that acts as intermediate between Web browser and the Web.
*
* Note: handles only GET requests
*
* Authors: Alex Lucena & Saumya Dalal
*
*/
#include <stdio.h>
#include "csapp.h"
#include "cache.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


/* Debug output macro taken from malloc lab */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";

struct clientInfo_t{
    struct sockaddr_in clientaddr;
    int connfd;

};
//parse_url(url_store, host, &port, path, cgiargs);
void serve(int connfd);
void read_headers(rio_t *rp, char* host_header, char *other_headers);
void parse_url(char *url, char *host, int *port, char *path, char *cgiargs);
void make_request(int fd, char *url, char *host, char *path, char *host_header, char *other_headers, char* newport);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
//void terminate(int param);
void *doit(void *arg);
int external_header(char* browser_request);

cache_LL* cache;

//sem_t accept_mutex;

/*
* Main function
*/
int main(int argc, char **argv)
{

    printf("%s%s%s", user_agent_hdr, accept_hdr, accept_encoding_hdr);

    int listenfd;
    //int  *connfd;
    int clientlen;
    //struct sockaddr_in clientaddr;
    struct clientInfo_t *clientInfo;
    pthread_t tid;

    //Sem_init(&accept_mutex, 0, 1);

    //cache initialization
    cache = (cache_LL*) Calloc(1, sizeof(cache_LL));
    cache->head = NULL;
    cache->size = 0;
    cache_init();

    //signal(SIGPIPE, terminate);
    //signal(SIGPIPE, NULL);

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }


    listenfd = Open_listenfd(argv[1]);

    // while (1)
    // {
    //     clientlen = sizeof(struct sockaddr_in);
    //     connfd = Calloc(1, sizeof(int));
    //     //P(&accept_mutex);
    //     *connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);

    // Pthread_create(&tid, NULL, doit, connfd);

    // }
    while(1)
    {

        clientInfo = Malloc(sizeof(struct clientInfo_t));
        //clientaddr = Malloc(sizeof(struct sockaddr_in));

        clientlen = sizeof(struct sockaddr_in);

        // accept connection from client; populate client sock addr
        //connfd = Accept(listenfd,(SA *)clientaddr,(socklen_t *)&clientlen);
        //threadInfo.connfd = Accept(listenfd,(SA *)clientaddr,(socklen_t *)&clientlen);
        clientInfo -> connfd = Accept(listenfd,(SA *)&clientInfo -> clientaddr,(socklen_t *)&clientlen);
        // thread((void *)threadInfo);
        //process_request(connfd,clientaddr);
        Pthread_create(&tid, NULL, doit, (void *)clientInfo);


    }

    exit(0);
}


void *doit(void *arg)
{
  //P(&accept_mutex);
  //int connfd = *((int *)arg);
  struct clientInfo_t *clientInfo;

  clientInfo = (struct clientInfo_t*)arg;

  int connfd = clientInfo -> connfd;

  //struct sockaddr_in* clientaddrp = &clientInfo -> clientaddr;

  //V(&accept_mutex);

  Pthread_detach(pthread_self());

  Free(arg);
  


  struct sockaddr_in clientaddr = clientInfo -> clientaddr;



  unsigned short client_port = ntohs(clientaddr.sin_port);
  char *client_IP = inet_ntoa(clientaddr.sin_addr);
  printf("proxy request from %s:%hu \n",client_IP,client_port);

  serve(connfd);

  Close(connfd);

  return NULL;
}


/*
* Serves a client's request.
* file_d is the file descriptor
*/
 void serve(int connfd)
 {
    char method[MAXLINE], url[MAXLINE], version[MAXLINE];
    char buf[MAXLINE], host[MAXLINE];

    char path[MAXLINE], cgiargs[MAXLINE];
    char host_header[MAXLINE], other_headers[MAXLINE];


    rio_t rio;
    /* Read in a request from client */
    Rio_readinitb(&rio, connfd); // initialize rio buffer with file descriptor
    Rio_readlineb(&rio, buf, MAXLINE); // read in line

    // move from buffer into string format
    sscanf(buf, "%s %s %s", method, url, version);

    if(strcasecmp(method,"GET"))
    {
        clienterror(connfd,method,"501","Not Implemented","Proxy does not implement this method");       
        return;

    }


    read_headers(&rio, host_header, other_headers);

    // Parse URL out of request


    // make a new string so as not to defile original url
    char url_store[MAXLINE];
    strcpy(url_store, url);


    int port = 80;
    //port = parse_url(url_store, host, path, cgiargs);
    parse_url(url_store, host, &port, path, cgiargs);



    printf("HOST = %s,HOST PORT = %d\n",host,port);
    char newport[MAXLINE];
    sprintf(newport,"%d",port);



    // if (!strncmp(buf, "https", strlen("https")))
    //     printf("\n\n\n EXPECT A DNS ERROR \n\n");


    make_request(connfd, url, host, path, host_header, other_headers, newport);


 }




// void terminate (int param)
// {
//     printf ("SIGPIPE . . . ignoring!\n");
// }



/*
* Reads in and ignores headers of requests.
* host_header and other_headers are merely buffers
* to store the information obtained from reading
* regarding the host headers and other necessary
* headers respectively.
*/
void read_headers(rio_t *rp, char *host_header, char *other_headers)
{
   char buf[MAXLINE];

   strcpy(other_headers, "");



   Rio_readlineb(rp, buf, MAXLINE);
   while(strcmp(buf, "\r\n"))
   {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);

        int prefix = strlen("Host: ");

    if (!strncmp(buf, "Host: ", prefix))
            strcpy(host_header, buf + prefix);
       if (external_header(buf))
       {
            sprintf(other_headers, "%s%s", other_headers, buf);
       }
    /* We added this check in order to ignore garbage headers */
    // if (buf[0] > 90 || buf[0] < 65)
    // {
    //   return;
    // }
   }
   return;
}






/*
* Parses URL from GET request into hostname and path
*/
void parse_url(char *url, char *host,int *port, char *path, char *cgiargs)
{
    char *path_ptr;

    //char temp[MAXLINE];
    //strcpy(temp, url);

    memset(host, 0, sizeof(host));
    memset(path, 0, sizeof(path));

    sscanf(url, "http://%s", url);


    if ((path_ptr = strchr(url, '/')) == NULL)
    {
        strcpy(host, url);
        strcpy(cgiargs, "");
        strcpy(path, "/");
    }
    else
    {
        strcpy(path, path_ptr);
        path_ptr[0] = '\0';
        strcpy(host, url);
    }


    char *port_ptr;
    //aaaaaaa
    //int port;
    //checks if the user requests a certain port
    if ((port_ptr = strchr(host, ':')) != NULL)
    {
    //aaaa
        *port = atoi(port_ptr+1);
        *port_ptr = '\0';
    }

    //else
      //  port = 80;//else uses the default port


    return;
    //return port;

}

/* Make request creates a request using the information such as the port,
 * file descriptor, url, host, path & necessary headers. These are stored
 * in a structure called argstruct (in order to use Pcreate_thread for
 * each new request that needs to be made). If the object associated with the url
 * exists in the cache, we return the stored data from the cache.
 * If not, the complete request with the
 * given information is created and stored in buf which is then read
 * MAXBUF bytes at a time. Information about the size & data from the web object
 * are kept track of and stored in cache_object_size & cache_object so that
 * the web object may be cached later.
 */
void make_request(int fd, char *url, char *host,
    char *path, char *host_header, char *other_headers, char* newport)
{

    web_object* found = checkCache(cache, url);

    //If the object is found, write the data back to the client
    if(found != NULL) {
        Rio_writen(fd, found->data, found->size);
        return;
    }


    int net_fd;
    char buf[MAXBUF], reply[MAXBUF];
    rio_t rio;



    // char newport[MAXLINE];
    // sprintf(newport,"%d",port);




    printf("dbg:: host:%s, newport:%s\n",host,newport);

    net_fd = Open_clientfd(host, newport);

    if (net_fd < -1)
    {
        clienterror(fd, host, "DNS!", "DNS error, this host isn't a host!", "Ah!");
    return;
    }

    /* The following code adds the necessary information to make buf a complete request */
    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    printf("Send request buf: \n%s\n", buf);

    if (!strlen(host_header))
        sprintf(buf, "%sHost: %s\r\n", buf, host);
    else
        sprintf(buf, "%sHost: %s\r\n", buf, host_header);
    sprintf(buf, "%s%s", buf, user_agent_hdr);
    sprintf(buf, "%s%s", buf, accept_hdr);
    sprintf(buf, "%s%s", buf, accept_encoding_hdr);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sProxy-Connection: close\r\n", buf);
    sprintf(buf, "%s%s\r\n", buf, other_headers);

    // dbg_printf("\n   SENDING REQUEST\n");
    // dbg_printf("%s\n", buf);
    // dbg_printf("\n   ENDING  REQUEST\n");


    Rio_writen(net_fd, buf, strlen(buf));


    strcpy(reply, "");
    strcpy(buf, "");
    Rio_readinitb(&rio, net_fd);

    int read_return;

    //cache_object size finds the total size of the data
    //by summing the total number of bytes received from
    //every read.
    char cache_object[MAX_OBJECT_SIZE];
    int cache_object_size = 0;

    // dbg_printf("Entering reading loop\n");
    do
    {
        strcpy(reply, "");

        // dbg_printf("Read \n");
        read_return = Rio_readnb(&rio, reply, MAXBUF);

        // dbg_printf("Read return: %d\n", read_return);
        // dbg_printf("Object size: %d\n", cache_object_size);

        //Add the bytes returned from the read to the total object size
        cache_object_size += read_return;

        //As long as our object size is within the max, continue to add data
        //to the cache_object so that we can add it to the cache later.
        if ( cache_object_size < MAX_OBJECT_SIZE )
        {
            // dbg_printf("Cache . . . \n");
            sprintf(cache_object, "%s%s", cache_object, reply);
        }

    // dbg_printf("Write . . . \n");
    //Write the data back to the client
    rio_writen(fd, reply, read_return);

    // dbg_printf("Loop\n\n");
    } while ( read_return > 0);


    if (cache_object_size < MAX_OBJECT_SIZE)
    {
        // dbg_printf("\nAdding to cache . . . \n");
        addToCache(cache, cache_object, url, cache_object_size);
        // dbg_printf("Done!\n");
    }

    return;
}

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
* Sends error to proxy's client as html file
*/
// void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
// {
//     char buf[MAXLINE], body[MAXLINE];

//     /* Build body */
//     sprintf(body, "<html><title>Web Proxy Error</title>");
//     sprintf(body, "%s<body bgcolor =\"#FF8680\">\r\n", body);
//     sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
//     sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
//     sprintf(body, "%s<hr><em>Alex & Saumya's Web Proxy</em>\r\n", body);

//     /* Print out response */
//     sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
//     Rio_writen(fd, buf, strlen(buf));
//     sprintf(buf, "Content-type: text/html\r\n");
//     Rio_writen(fd, buf, strlen(buf));
//     sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
//     Rio_writen(fd, buf, strlen(buf));
//     Rio_writen(fd, body, strlen(body));
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
// }
