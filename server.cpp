#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <string.h>
#include "threadpool.h"
#include "http_conn.h"

#define MAX_EVENT_NUMBER 1024
#define MAX_FD 65536

extern int setfd(int epollfd,int fd,bool one_shot);

void error_die(const std::string& message)
{
    std::cerr << message << std::endl;
    exit(1);
}

int start_server(__u_short* server_port)
{
    int server_sock = 0;
    int option = 1;
    struct sockaddr_in server_addr;
    server_sock = socket(PF_INET,SOCK_STREAM,0);
    if(server_sock == -1)
        error_die("socket() failed");

    socklen_t optlen = sizeof(option);
    setsockopt(server_sock,SOL_SOCKET,SO_REUSEADDR,(void*)option,optlen);

    struct linger tmp = {1,0};
    setsockopt(server_sock,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(*server_port);

    if(bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0)
        error_die("bind() failed");

    if(*server_port == 0)
    {
        socklen_t sockaddr_len = sizeof(server_addr);
        if(getsockname(server_sock,(struct sockaddr*)&server_addr,&sockaddr_len) == -1)
            error_die("getsockname() failed");
        *server_port = ntohs(server_addr.sin_port);
    }

    if(listen(server_sock,5) < 0)
        error_die("listen() failed");
    
    return server_sock;
}

// int parse_line(int sock,char* buf,int size)
// {
//     int i = 0;
//     char c = '\0';
//     int n;
//     while((i < size - 1) && (c != '\n'))
//     {
//         n = recv(sock,&c,1,0);
//         if(n > 0)
//         {
//             if(c == '\r')
//             {
//                 n = recv(sock,&c,1,MSG_PEEK);
//                 if((n > 0) && (c == '\n'))
//                     recv(sock,&c,1,0);
//                 else
//                     c = '\n';
//             }
//             buf[i] = c;
//             ++i;
//         }
//         else
//             c = '\n';
//     }
//     buf[i] = '\0';
//     return i;
// }

// void unimplemented(int client_sock)
// {
//     char buf[1024];

//     sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "From lzh's server");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "Content-Type: text/html\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "</TITLE></HEAD>\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "</BODY></HTML>\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// }

// void not_found(int client_sock)
// {
//     char buf[1024];
// 	 sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
// 	 send(client_sock, buf, strlen(buf), 0);
// 	 sprintf(buf, "From lzh's server");
// 	 send(client_sock, buf, strlen(buf), 0);
// 	 sprintf(buf, "Content-Type: text/html\r\n");
// 	 send(client_sock, buf, strlen(buf), 0);
// 	 sprintf(buf, "\r\n");
// 	 send(client_sock, buf, strlen(buf), 0);
// 	 sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
// 	 send(client_sock, buf, strlen(buf), 0);
// 	 sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
// 	 send(client_sock, buf, strlen(buf), 0);
// 	 sprintf(buf, "your request because the resource specified\r\n");
// 	 send(client_sock, buf, strlen(buf), 0);
// 	 sprintf(buf, "is unavailable or nonexistent.\r\n");
// 	 send(client_sock, buf, strlen(buf), 0);
// 	 sprintf(buf, "</BODY></HTML>\r\n");
// 	 send(client_sock, buf, strlen(buf), 0);
// }

// void headers(int client_sock)
// {
//     char buf[1024];
// 	strcpy(buf, "HTTP/1.0 200 OK\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	strcpy(buf, "From lzh's server");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "Content-Type: text/html\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	strcpy(buf, "\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// }

// void send_file(int client_sock,FILE* resource)
// {
//     char buf[BUF_SIZE];
//     fgets(buf,sizeof(buf),resource);
//     while(!feof(resource))
//     {
//         send(client_sock,buf,strlen(buf),0);
//         fgets(buf,sizeof(buf),resource);
//     }
// }

// void serve_file(int client_sock,const char* filename)
// {
//     FILE* resource = nullptr;
//     int numchars = 1;
//     char buf[BUF_SIZE];
//     buf[0] = 'A';
//     buf[1] = '\0';
//     while((numchars > 0) && strcmp("\n",buf))
//         numchars = parse_line(client_sock,buf,sizeof(buf));
//     resource = fopen(filename,"r");
//     if(resource == nullptr)
//         not_found(client_sock);
//     else
//     {
//         headers(client_sock);
//         send_file(client_sock,resource);
//     }
//     fclose(resource);
// }

// void bad_request(int client_sock)
// {
//     char buf[1024];
// 	sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
// 	send(client_sock, buf, sizeof(buf), 0);
// 	sprintf(buf, "Content-type: text/html\r\n");
// 	send(client_sock, buf, sizeof(buf), 0);
// 	sprintf(buf, "\r\n");
// 	send(client_sock, buf, sizeof(buf), 0);
// 	sprintf(buf, "<P>Your browser sent a bad request, ");
// 	send(client_sock, buf, sizeof(buf), 0);
// 	sprintf(buf, "such as a POST without a Content-Length.\r\n");
// 	send(client_sock, buf, sizeof(buf), 0);
// }

// void cannot_execute(int client_sock)
// {
//     char buf[1024];
// 	sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "Content-type: text/html\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// 	sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
// 	send(client_sock, buf, strlen(buf), 0);
// }

// void execute_cgi(int client_sock,const char* path,const char* method,const char* query_string)
// {
//     char buf[BUF_SIZE];
//     int cgi_output[2];
//     int cgi_input[2];

//     pid_t pid;
//     int status;
//     int i = 0;
//     int numchars = 1;
//     int content_length = -1;
//     char c;
    
//     buf[0] = 'A';
//     buf[1] = '\0';
//     if(strcasecmp(method,"GET") == 0)
//     {
//         while((numchars > 0) && strcmp("\n",buf))
//             numchars = parse_line(client_sock,buf,sizeof(buf));
//     }
//     else
//     {
//         numchars = parse_line(client_sock,buf,sizeof(buf));
//         while((numchars > 0) && strcmp("\n",buf))
//         {
//             buf[15] = '\0';
//             if(strcasecmp(buf,"Content-Length:") == 0)
//                 content_length = atoi(&(buf[16]));
//             numchars = parse_line(client_sock,buf,sizeof(buf));
//         }
//         if(content_length == -1)
//         {
//             bad_request(client_sock);
//             return;
//         }
//     }
//     sprintf(buf,"HTTP/1.0 200 OK\r\n");
//     send(client_sock,buf,strlen(buf),0);
//     if(pipe(cgi_output) < 0)
//     {
//         cannot_execute(client_sock);
//         return;
//     }
//     if(pipe(cgi_input) < 0)
//     {
//         cannot_execute(client_sock);
//         return;
//     }
//     if((pid = fork()) < 0)
//     {
//         cannot_execute(client_sock);
//         return;
//     }
//     if(pid == 0)
//     {
//         char meth_env[255];
//         char query_env[255];
//         char length_env[255];

//         dup2(cgi_output[1],STDOUT_FILENO);
//         dup2(cgi_input[0],STDIN_FILENO);
//         close(cgi_output[0]);
//         close(cgi_input[1]);

//         sprintf(meth_env,"REQUEST_METHOD=%s",method);
//         putenv(meth_env);

//         if(strcasecmp(method,"GET") == 0)
//         {
//             sprintf(query_env,"QUERY_STRING=%s",query_string);
//             putenv(query_env);
//         }
//         else
//         {
//             sprintf(length_env,"CONTENT_LENGTH=%d",content_length);
//             putenv(length_env);
//         }
//         execl(path,path,nullptr);
//         exit(0);
//     }
//     else
//     {
//         close(cgi_output[1]);
//         close(cgi_input[0]);
//         if(strcasecmp(method,"POST") == 0)
//         {
//             for(i = 0;i < content_length;++i)
//             {
//                 recv(client_sock,&c,1,0);
//                 write(cgi_input[1],&c,1);
//             }
//         }
//         while(read(cgi_output[0],&c,1) > 0)
//             send(client_sock,&c,1,0);
        
//         close(cgi_output[0]);
//         close(cgi_input[1]);
//         waitpid(pid,&status,0);
//     }
// }

// void* accept_request(void* arg)
// {
//     int client_sock = *(int*)arg;
//     char buf[BUF_SIZE];
//     char method[255];
//     char url[255];
//     char path[521];
//     int numchars = 0;
//     size_t i = 0,j = 0;
//     bool cgi = false;
//     char* query_string = nullptr;
//     struct stat st;

//     numchars = parse_line(client_sock,buf,sizeof(buf));

//     while(buf[j] != ' ' && i < sizeof(method) - 1)
//         method[i++] = buf[j++];
//     method[i] = '\0';
    
//     if(strcasecmp(method,"GET") && strcasecmp(method,"POST"))
//     {
//         unimplemented(client_sock);
//         return nullptr;
//     }

//     if(strcasecmp(method,"POST") == 0)
//         cgi = true;
    
//     i = 0;
//     while(buf[j] == ' ' && j < sizeof(buf))
//         ++j;
    
//     while(buf[j] != ' ' && (i < sizeof(url) - 1) && j < sizeof(buf))
//         url[i++] = buf[j++];
//     url[i] = '\0';

//     if(strcasecmp(method,"GET") == 0)
//     {
//         query_string = url;
//         while((*query_string != '?') && (*query_string != '\0'))
//             ++query_string;
//         if(*query_string == '?')
//         {
//             cgi = true;
//             *query_string = '\0';
//             ++query_string;
//         }
//     }
//     sprintf(path,"httpdocs%s",url);

//     if(path[strlen(path) - 1] == '/')
//         strcat(path,"test.html");
    
//     if(stat(path,&st) == -1)
//     {
//         while((numchars > 0) && strcmp("\n",buf))
//             numchars = parse_line(client_sock,buf,sizeof(buf));
//         not_found(client_sock);
//     }
//     else
//     {
//         if((st.st_mode & S_IFMT) == S_IFDIR)
//             strcat(path,"/test.html");
//         if((st.st_mode & S_IXUSR) ||
//            (st.st_mode & S_IXGRP) ||
//            (st.st_mode & S_IXOTH))
//             cgi = true;
//         if(!cgi)
//             serve_file(client_sock,path);
//         else
//             execute_cgi(client_sock,path,method,query_string);
//     }
//     close(client_sock);
//     std::cout << "client " << client_sock << "'s connection closed" << std::endl;
//     return nullptr;
// }

void show_error(int client_sock,const char* msg)
{
    std::cout << msg << std::endl;
    send(client_sock,msg,strlen(msg),0);
    close(client_sock);
}

int main()
{
    threadpool<http_conn>* pool;
    try
    {
        pool = new threadpool<http_conn>;
    }
    catch(...)
    {
        error_die("create pool failed");
    }
    http_conn* users = new http_conn[MAX_FD];
    if(!users)
    {
        error_die("create user's http_conn failed");
    }
    int user_count = 0;

    int server_sock = -1;
    __u_short server_port = 9190;

    pthread_t main_thread;
    server_sock = start_server(&server_port);

    std::cout << "my server socket is " << server_sock << std::endl;
    std::cout << "my server is running on port " << server_port << std::endl;

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    if(epollfd == -1)
        error_die("epoll_create() failed");
    setfd(epollfd,server_sock,false);
    http_conn::m_epollfd = epollfd;

    while(1)
    {
        int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if(number < 0)
            error_die("epoll_wait() failed");
        
        for(int i = 0;i < number;++i)
        {
            int sockfd = events[i].data.fd;
            if(sockfd == server_sock)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_sock = accept(server_sock,(struct sockaddr*)&client_addr,&client_addr_len);
                if(client_sock < 0)
                    error_die("accept() failed");
                if(http_conn::m_user_count >= MAX_FD)
                {
                    show_error(client_sock,"server busy");
                    continue;
                }
                std::cout << "new connection-----ip:" << inet_ntoa(client_addr.sin_addr) << std::endl;
                std::cout << "              -----port:" << ntohs(client_addr.sin_port) << std::endl;
                users[client_sock].init(client_sock,client_addr);
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                users[sockfd].close_conn();
            }
            else if(events[i].events & EPOLLIN)
            {
                pool->append((users + sockfd));
            }
            else if(events[i].events & EPOLLOUT)
            {
                users[sockfd].close_conn();
            }
            else
            {}
        }
    }
    close(epollfd);
    close(server_sock);
    delete[] users;
    delete pool;
    return 0;
}