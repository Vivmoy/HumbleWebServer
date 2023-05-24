#ifndef _HTTP_CONN_H
#define _HTTP_CONN_H

#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include "lock.h"

class http_conn
{
public:
    static const int BUF_SIZE = 1024;
public:
    http_conn() {};
    ~http_conn() {};

    void init(int client_sock,const sockaddr_in& addr);
    void close_conn(bool real_close = true);
    void* process();
public:
    static int m_epollfd;
    static int m_user_count;
private:
    int parse_line(int sock,char* buf,int size);
    void unimplemented(int client_sock);
    void not_found(int client_sock);
    void headers(int client_sock);
    void send_file(int client_sock,FILE* resource);
    void serve_file(int client_sock,const char* filename);
    void bad_request(int client_sock);
    void cannot_execute(int client_sock);
    void execute_cgi(int client_sock,const char* path,const char* method,const char* query_string);
private:
    int m_sockfd;
    sockaddr_in m_address;
};
#endif