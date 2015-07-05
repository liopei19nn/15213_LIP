#include <stdio.h>
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

static const char *user_agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_ = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_language ="Accept-Language: en-US,en;q=0.5\r\n";
static const char *accept_encoding = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection = "Connection: close\r\n";
static const char *proxy_connection = "Proxy-Connection: close\r\n";
#define DEBUG

#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

#define IP_ADDR_SIZE 4
#define MAX_DNLEN 253

void read_requesthdrs(rio_t *rp);
void process_request(int connfdp,struct sockaddr_in* clientaddrp);
void clienterror(int fd,char* cause,char* errnum,char* shortmsg,char* longmsg);
void proxy_my_connection(char *host,int port,int connfd,char *proxy_req);
void generate_request(char* request,rio_t *client_rio);
void get_host_port(char *host,int *port,char *uri);
int external_header(char *browser_request);
char *modify_uri(char *uri);

int main(int argc, char **argv)
{
    printf("%s%s%s", user_agent, accept_, accept_encoding);

    int listenfd;

    int connfd;
    int clientlen;

    struct sockaddr_in *clientaddr;

    if(argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    

    
    // create socket; bind and set port as listening port;return fd
    //port = argv[1];
    listenfd = Open_listenfd(argv[1]);

    while(1)
    {

        clientaddr = Malloc(sizeof(struct sockaddr_in));

        clientlen = sizeof(struct sockaddr_in);

        // accept connection from client; populate client sock addr
        connfd = Accept(listenfd,(SA *)clientaddr,(socklen_t *)&clientlen);

        process_request(connfd,clientaddr);

    }

    exit(0);
}

void process_request(int connfd,struct sockaddr_in* clientaddrp)
{

    struct sockaddr_in clientaddr = *clientaddrp;
    free(clientaddrp);
    
    struct hostent *hp;
    unsigned short cport;
    char *haddrp;

    char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
    rio_t rio;

    char proxy_req[MAXLINE];
    char host[MAXLINE];
//    char* host_original;
    int port = 80;

    // get hostent for client from clientaddr->sin_addr->s_addr
    hp = Gethostbyaddr((const char *)&(clientaddr.sin_addr.s_addr),IP_ADDR_SIZE,AF_INET);

    // generate decimal formatted client address string
    haddrp = inet_ntoa(clientaddr.sin_addr);
    cport = ntohs(clientaddr.sin_port);

    printf(" proxy recd REQ from : %s (%s:%hu)\n",hp->h_name,haddrp,cport);


    /* check REQ type ; ignore all except GET*/
    Rio_readinitb(&rio,connfd);
    Rio_readlineb(&rio,buf,MAXLINE);

    sscanf(buf,"%s %s %s",method,uri,version);
    dbg_printf("Received Request : [%s %s %s]\n",method,uri,version);

    if(strcasecmp(method,"GET"))
    {
        dbg_printf("%s : method not implemented by proxy server\n",method);
        clienterror(connfd,method,"501","Not Implemented","Proxy does not implement this method");

        Close(connfd);
        dbg_printf("Server Closed Connection !\n");
    
        return;

    }

    get_host_port(host,&port,uri);
 
    sprintf(proxy_req,"GET %s HTTP/1.0\r\n",modify_uri(uri));

    generate_request(proxy_req,&rio);
      
    proxy_my_connection(host,port,connfd,proxy_req);

    Close(connfd);


    // possible source of memory leak in case of server failure
}




void get_host_port(char *host,int *port,char *uri_)
{

    char *first_break;
    char *second_break;
    char* port_check = NULL;
    char* portch;
    char *host_temp;
    char uri[MAXLINE];
//    char *uri;
    
//    uri = uribuf;
    //uri = Malloc(sizeof(char) * strlen(uri_));
    strcpy(uri,uri_);

    dbg_printf("get_host_port : [uri : %s]\n",uri);

    first_break = strstr(uri,"://");
    host_temp = first_break;

// points to host name  
    host_temp++;
    host_temp++;
    host_temp++;

// points to start of resource requested
    second_break  = strstr(host_temp,"/");

// get the host name   
    *second_break = '\0'; 

// check if port specified
    port_check = strstr(host_temp,":");

//found port appended
    if(port_check)
    {
         portch = port_check + 1;   
         *port = atoi(portch); //get port        
         *port_check = '\0'; // modify host ending
    }

    strcpy(host,host_temp);    
    
    dbg_printf("get_host_port : [FOR URI :%s]\n",uri_);
    dbg_printf("get_host_port : [Host :%s]\n",host);
    dbg_printf("get_host_port : [Port :%d]\n",*port);
//    free(uri);

   
}

void proxy_my_connection(char *host, int port,int connfd, char *proxy_req)
{

    int clientfd;
    char buf[MAXLINE];
    rio_t rio;
    size_t nobytes;

    char *newport;
    sprintf(newport,"%d",port);

    dbg_printf("connection_established : connecting with host : %s | port : %d\n",host,port);
    clientfd = Open_clientfd(host,newport);
    Rio_readinitb(&rio,clientfd);

    dbg_printf("Connection established with : %s (%d)\n",host,port);
    
    dbg_printf("sending request... :\n%s\n\n",proxy_req);

    Rio_writen(clientfd,proxy_req,strlen(proxy_req));
/*
    if(rio_writen(clientfd,proxy_req,strlen(proxy_req)) != strlen(proxy_req)){
        Close(clientfd);
        printf("\nproxy_my_connection : write error 1 ! close clientfd\n");
        return;
    }
*/
    nobytes = Rio_readnb(&rio,buf,MAXLINE);


    while(nobytes > 0)
    {
        dbg_printf("RECEIVED :%lu bytes\nSending ...\n%s\n",nobytes,buf);

/*
        if(rio_writen(clientfd,proxy_req,strlen(proxy_req)) != strlen(proxy_req)){
            printf("\nproxy_my_connection : write error 2 ! close clientfd\n");
            Close(clientfd);
            return;
        }
*/

        Rio_writen(connfd,buf,nobytes);        
        nobytes = Rio_readnb(&rio,buf,MAXLINE);
    
    }
    dbg_printf("proxy_my_connection : Done !\n");
    Close(clientfd);

}

void generate_request(char* request,rio_t *client_rio)
{
    
    size_t n;
    char buf[MAXLINE];

//    Rio_readinitb(&rio,connfd);
    n = Rio_readlineb(client_rio,buf,MAXLINE);

    while(strcmp(buf,"\r\n") && (n != 0))
    {
        dbg_printf("server received : %s bytes received : %lu\n\n",buf,n);
        
        if(external_header(buf))
        sprintf(request,"%s%s",request,buf);
    
        n = Rio_readlineb(client_rio,buf,MAXLINE);
    }

    sprintf(request,"%s%s%s%s%s%s%s\r\n",request,user_agent,accept_,accept_language,accept_encoding,connection,proxy_connection);

    return; 
/*
    sprintf(body,"%s%s: %s\r\n",body,errnum,shortmsg);
    sprintf(body,"%s<p>%s: %s\r\n",body,longmsg,cause); 
    sprintf(body,"%s<hr><em>The Proxy Server</em>\r\n",body);
*/

}

int external_header(char* browser_request)
{

    if(strstr(browser_request,"User-Agent:"))
    return 0;
    else if(strstr(browser_request,"Accept:"))
    return 0;
    else if(strstr(browser_request,"Accept-Language:"))
    return 0;
    else if(strstr(browser_request,"Accept-Encoding:"))
    return 0;
    else if(strstr(browser_request,"Connection:"))
    return 0;
    else if(strstr(browser_request,"Proxy-Connection:"))
    return 0;
    

    return 1;

}

char* modify_uri(char* uri)
{
    char *resource;    

    resource = strpbrk(uri,"//");
    resource++;
    resource++;

    resource = strpbrk(resource,"/");
    dbg_printf("modify_uri : [uri : %s]\n",resource);

    return resource;

}


void clienterror(int fd,char* cause,char* errnum,char* shortmsg,char* longmsg)
{
//buggy code
    char buf[MAXLINE],body[MAXLINE];

    /* Build the HTTP response body */
    sprintf(body,"<html><title>Proxy Error</title>");
    sprintf(body,"%s<body bgcolor=""ffffff"">\r\n",body);
    sprintf(body,"%s%s: %s\r\n",body,errnum,shortmsg);
    sprintf(body,"%s<p>%s: %s\r\n",body,longmsg,cause); 
    sprintf(body,"%s<hr><em>The Proxy Server</em>\r\n",body);

    // Print the HTTP response 
    sprintf(buf,"HTTP/1.0 %s %s\r\n",errnum,shortmsg);
    Rio_writen(fd,buf,strlen(buf));
    sprintf(buf,"Content-type: text/html\r\n");
    Rio_writen(fd,buf,strlen(buf));
    sprintf(buf,"Content-length: %d\r\n\r\n",(int)strlen(body));
    Rio_writen(fd,buf,strlen(buf));
    Rio_writen(fd,body,strlen(body));

}