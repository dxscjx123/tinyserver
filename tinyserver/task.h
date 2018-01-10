#ifndef TASK_H
#define TASK_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>

#include "processpool.h"
#include "predefine.h"
#include "log.h"

class task_conn
{
public:
	task_conn(){}
	~task_conn(){}
	void init( int epollfd, int sockfd, const sockaddr_in &client_addr )
	{
		m_epollfd = epollfd;
		m_sockfd = sockfd;
		m_address = client_addr;
		memset( m_buf, '\0', BUFFER_SIZE );
		m_read_idx = 0;
	}
	
	void accept_request();
	void bad_request( int client );
	void cat( int client, FILE *resource);
	void cannot_execute( int client );
	void error_die( const char *sc );
	void execute_cgi(int client, const char *path, const char *method, const char *query_string);
	int get_line( int sock, char *buf, int size);
	void headers( int client, const char *filename );
	void not_found( int client );
	void serve_file( int client, const char *filename );
	void unimplemented( int client );

private:
	static const int BUFFER_SIZE = 1024;
	int m_epollfd;
	int m_sockfd;
	sockaddr_in m_address;
	char m_buf[ BUFFER_SIZE ];
	int m_read_idx;
};

#endif