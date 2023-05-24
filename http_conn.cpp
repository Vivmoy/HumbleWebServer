#include "http_conn.h"

int setnonblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

void setfd(int epollfd,int fd,bool one_shot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET || EPOLLRDHUP;
    if(one_shot)
    {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnonblocking(fd);
}

void removefd(int epollfd,int fd)
{
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void http_conn::init(int client_sock,const sockaddr_in& addr)
{
    m_sockfd = client_sock;
    m_address = addr;
    setfd(m_epollfd,client_sock,true);
    ++m_user_count;
}

void http_conn::close_conn(bool real_close)
{
    if(real_close && (m_sockfd != -1))
    {
        removefd(m_epollfd,m_sockfd);
        m_sockfd = -1;
        --m_user_count;
    }
}

int http_conn::parse_line(int sock,char* buf,int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while((i < size - 1) && (c != '\n'))
    {
        n = recv(sock,&c,1,0);
        if(n > 0)
        {
            if(c == '\r')
            {
                n = recv(sock,&c,1,MSG_PEEK);
                if((n > 0) && (c == '\n'))
                    recv(sock,&c,1,0);
                else
                    c = '\n';
            }
            buf[i] = c;
            ++i;
        }
        else
            c = '\n';
    }
    buf[i] = '\0';
    return i;
}

void http_conn::unimplemented(int client_sock)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "From lzh's server");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "</TITLE></HEAD>\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "<BODY><P>HTTP request method not supported.\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "</BODY></HTML>\r\n");
	send(client_sock, buf, strlen(buf), 0);
}

void http_conn::not_found(int client_sock)
{
    char buf[1024];
	 sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	 send(client_sock, buf, strlen(buf), 0);
	 sprintf(buf, "From lzh's server");
	 send(client_sock, buf, strlen(buf), 0);
	 sprintf(buf, "Content-Type: text/html\r\n");
	 send(client_sock, buf, strlen(buf), 0);
	 sprintf(buf, "\r\n");
	 send(client_sock, buf, strlen(buf), 0);
	 sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
	 send(client_sock, buf, strlen(buf), 0);
	 sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
	 send(client_sock, buf, strlen(buf), 0);
	 sprintf(buf, "your request because the resource specified\r\n");
	 send(client_sock, buf, strlen(buf), 0);
	 sprintf(buf, "is unavailable or nonexistent.\r\n");
	 send(client_sock, buf, strlen(buf), 0);
	 sprintf(buf, "</BODY></HTML>\r\n");
	 send(client_sock, buf, strlen(buf), 0);
}

void http_conn::headers(int client_sock)
{
    char buf[1024];
	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(client_sock, buf, strlen(buf), 0);
	strcpy(buf, "From lzh's server");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client_sock, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client_sock, buf, strlen(buf), 0);
}

void http_conn::send_file(int client_sock,FILE* resource)
{
    char buf[BUF_SIZE];
    fgets(buf,sizeof(buf),resource);
    while(!feof(resource))
    {
        //std::cout << "3" << std::endl;
        send(client_sock,buf,strlen(buf),0);
        fgets(buf,sizeof(buf),resource);
    }
}

void http_conn::serve_file(int client_sock,const char* filename)
{
    FILE* resource = nullptr;
    int numchars = 1;
    char buf[BUF_SIZE];
    buf[0] = 'A';
    buf[1] = '\0';
    while((numchars > 0) && strcmp("\n",buf))
        numchars = parse_line(client_sock,buf,sizeof(buf));
    resource = fopen(filename,"r");
    if(resource == nullptr)
        not_found(client_sock);
    else
    {
        //std::cout << "2" << std::endl;
        headers(client_sock);
        send_file(client_sock,resource);
    }
    fclose(resource);
}

void http_conn::bad_request(int client_sock)
{
    char buf[1024];
	sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
	send(client_sock, buf, sizeof(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client_sock, buf, sizeof(buf), 0);
	sprintf(buf, "\r\n");
	send(client_sock, buf, sizeof(buf), 0);
	sprintf(buf, "<P>Your browser sent a bad request, ");
	send(client_sock, buf, sizeof(buf), 0);
	sprintf(buf, "such as a POST without a Content-Length.\r\n");
	send(client_sock, buf, sizeof(buf), 0);
}

void http_conn::cannot_execute(int client_sock)
{
    char buf[1024];
	sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "Content-type: text/html\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "\r\n");
	send(client_sock, buf, strlen(buf), 0);
	sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
	send(client_sock, buf, strlen(buf), 0);
}

void http_conn::execute_cgi(int client_sock,const char* path,const char* method,const char* query_string)
{
    char buf[BUF_SIZE];
    int cgi_output[2];
    int cgi_input[2];

    pid_t pid;
    //int status;
    int i = 0;
    int numchars = 1;
    int content_length = -1;
    char c;
    
    buf[0] = 'A';
    buf[1] = '\0';
    if(strcasecmp(method,"GET") == 0)
    {
        while((numchars > 0) && strcmp("\n",buf))
            numchars = parse_line(client_sock,buf,sizeof(buf));
    }
    else
    {
        numchars = parse_line(client_sock,buf,sizeof(buf));
        while((numchars > 0) && strcmp("\n",buf))
        {
            buf[15] = '\0';
            if(strcasecmp(buf,"Content-Length:") == 0)
                content_length = atoi(&(buf[16]));
            numchars = parse_line(client_sock,buf,sizeof(buf));
        }
        if(content_length == -1)
        {
            bad_request(client_sock);
            return;
        }
    }
    sprintf(buf,"HTTP/1.0 200 OK\r\n");
    send(client_sock,buf,strlen(buf),0);
    if(pipe(cgi_output) < 0)
    {
        cannot_execute(client_sock);
        return;
    }
    if(pipe(cgi_input) < 0)
    {
        cannot_execute(client_sock);
        return;
    }
    if((pid = fork()) < 0)
    {
        cannot_execute(client_sock);
        return;
    }
    if(pid == 0)
    {
        char meth_env[255];
        char query_env[255];
        char length_env[255];

        dup2(cgi_output[1],STDOUT_FILENO);
        dup2(cgi_input[0],STDIN_FILENO);
        close(cgi_output[0]);
        close(cgi_input[1]);

        sprintf(meth_env,"REQUEST_METHOD=%s",method);
        putenv(meth_env);

        if(strcasecmp(method,"GET") == 0)
        {
            sprintf(query_env,"QUERY_STRING=%s",query_string);
            putenv(query_env);
        }
        else
        {
            sprintf(length_env,"CONTENT_LENGTH=%d",content_length);
            putenv(length_env);
        }
        execl(path,path,nullptr);
        exit(0);
    }
    else
    {
        close(cgi_output[1]);
        close(cgi_input[0]);
        if(strcasecmp(method,"POST") == 0)
        {
            for(i = 0;i < content_length;++i)
            {
                recv(client_sock,&c,1,0);
                write(cgi_input[1],&c,1);
            }
        }
        while(read(cgi_output[0],&c,1) > 0)
            send(client_sock,&c,1,0);
        
        // close(cgi_output[0]);
        // close(cgi_input[1]);
        // waitpid(pid,&status,0);
    }
}

void* http_conn::process()
{
    int client_sock = m_sockfd;
    char buf[BUF_SIZE];
    char method[255];
    char url[255];
    char path[521];
    int numchars = 0;
    size_t i = 0,j = 0;
    bool cgi = false;
    char* query_string = nullptr;
    struct stat st;

    numchars = parse_line(client_sock,buf,sizeof(buf));

    while(buf[j] != ' ' && i < sizeof(method) - 1)
        method[i++] = buf[j++];
    method[i] = '\0';
    
    if(strcasecmp(method,"GET") && strcasecmp(method,"POST"))
    {
        unimplemented(client_sock);
        return nullptr;
    }

    if(strcasecmp(method,"POST") == 0)
        cgi = true;
    
    i = 0;
    while(buf[j] == ' ' && j < sizeof(buf))
        ++j;
    
    while(buf[j] != ' ' && (i < sizeof(url) - 1) && j < sizeof(buf))
        url[i++] = buf[j++];
    url[i] = '\0';

    if(strcasecmp(method,"GET") == 0)
    {
        query_string = url;
        while((*query_string != '?') && (*query_string != '\0'))
            ++query_string;
        if(*query_string == '?')
        {
            cgi = true;
            *query_string = '\0';
            ++query_string;
        }
    }
    sprintf(path,"httpdocs%s",url);

    if(path[strlen(path) - 1] == '/')
        strcat(path,"test.html");
    
    if(stat(path,&st) == -1)
    {
        while((numchars > 0) && strcmp("\n",buf))
            numchars = parse_line(client_sock,buf,sizeof(buf));
        not_found(client_sock);
    }
    else
    {
        if((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path,"/test.html");
        if((st.st_mode & S_IXUSR) ||
           (st.st_mode & S_IXGRP) ||
           (st.st_mode & S_IXOTH))
            cgi = true;
        if(!cgi)
        {
            //std::cout << "1" << std::endl;
            serve_file(client_sock,path);
        }
        else
            execute_cgi(client_sock,path,method,query_string);
    }
    close_conn();
    std::cout << "client " << client_sock << "'s connection closed" << std::endl;
    return nullptr;
}