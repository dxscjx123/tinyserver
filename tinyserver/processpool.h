#ifndef PROCESSPOOL_H
#define PROCESSPOOL_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/wait.h>

#include "predefine.h"
#include "log.h"

#define DEFAULT_PROCESS_NUMBER 8

class process
{
public:
	process():m_pid( -1 ){}
	pid_t m_pid;
	int m_pipefd[2];
};

template< typename T >
class processpool
{
private:
	processpool( int listenfd, int process_number = DEFAULT_PROCESS_NUMBER );
	void run_parent(void);
	void run_child(void);
	void setup_sig_pipe(void);
public:
	static processpool< T >* create( int listenfd, int process_number = DEFAULT_PROCESS_NUMBER )
	{
		if ( !m_instance )
		{
			m_instance = new processpool< T >( listenfd, process_number);
		}
		return m_instance;
	}
	~processpool()
	{
		delete [] m_sub_process;
	}
	void run();
private:
	static const int MAX_PROCESS_NUMBER = 16;
	static const int USER_PER_PROCESS = 65536;
	static const int MAX_EVENT_NUMBER = 10000;
	int m_process_number;
	int m_epollfd;
	int m_listenfd;
	int m_stop;
	int m_idx;
	process *m_sub_process;
	static processpool< T >* m_instance;
	//static int sig_pipefd[2];
};

template< typename T >
processpool< T >* processpool < T >::m_instance = NULL;

template< typename T >
processpool< T >::processpool( int listenfd, int process_number )
	:m_listenfd( listenfd ), m_process_number( process_number ), m_idx( -1 ), m_stop( false )
{
	assert( ( process_number > 0 ) && ( process_number <= MAX_PROCESS_NUMBER ) );
	m_sub_process = new process[ process_number ];

	for ( int i = 0; i < process_number; i++ )
	{
		int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd );
		assert ( ret == 0 );

		m_sub_process[i].m_pid = fork();
		assert( m_sub_process[i].m_pid >= 0);
		if ( m_sub_process[i].m_pid > 0 )
		{
			close( m_sub_process[i].m_pipefd[1] );
			continue;
		}
		else
		{
			close( m_sub_process[i].m_pipefd[0] );
			m_idx = i;
			break;
		}
	}
}

template< typename T >
void processpool< T >::run()
{
	if ( m_idx != -1 )
	{
		run_child();
		return ;
	}
	log( LOG_INFO_L, __FILE__, __LINE__, "parent pid is %d", getpid() );
	//printf( "parent pid is %d\n", getpid() );
	run_parent();
}

template< typename T >
void processpool< T >::run_parent()
{
	setup_sig_pipe();
	//m_epollfd = epoll_create( 5 );
	addfd( m_epollfd, m_listenfd );

	epoll_event events[ MAX_EVENT_NUMBER ];
	int sub_process_counter = 0;
	int number;
	int new_conn = 1;
	int ret;

	while ( !m_stop )
	{
		number = epoll_wait( m_epollfd, events, MAX_EVENT_NUMBER, -1 );

		if ( ( number < 0 ) && ( errno != EINTR ) )
		{
			log( LOG_INFO_L, __FILE__, __LINE__, "parent epoll fail");
			//printf( "parent epoll fail\n" );
			break;
		}

		for ( int i = 0; i < number; i++ )
		{
			int sockfd = events[i].data.fd;
			if ( sockfd == m_listenfd )
			{
				int i = sub_process_counter;
				do
				{
					//int i = sub_process_counter;
					if ( m_sub_process[i].m_pid != -1 )
					{
						break;
					}
					i = ( i + 1 ) % m_process_number;
				}
				while ( i != sub_process_counter );

				if (m_sub_process[i].m_pid == -1)
				{
					m_stop = true;
					break;
				}
				sub_process_counter = ( i + 1 ) % m_process_number;
				send( m_sub_process[i].m_pipefd[0], ( char * )&new_conn, sizeof( new_conn ), 0 );
				//printf( "new_conn send to child %d\n", i);
				log( LOG_DEBUG_L, __FILE__, __LINE__, "new_conn send to child %d", i);
			}
			else if ( ( sockfd == sig_pipefd[0] ) && ( events[i].events & EPOLLIN ) )
			{
				//printf("recv signal parent\n");
				log( LOG_DEBUG_L, __FILE__, __LINE__, "recv signal parent");
				int signal;
				int ret = recv( sig_pipefd[0], ( char * )&signal, sizeof( signal ), 0 );
				if ( ret < 0 )
				{
					continue;
				}
				else
				{
					switch ( signal )
					{
						case SIGCHLD:
						{
							pid_t pid;
							int stat;
							while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 )
							{
								for ( int i = 0; i < m_process_number; i++ )
								{
									if ( m_sub_process[i].m_pid == pid )
									{
										log( LOG_INFO_L, __FILE__, __LINE__, "child %d will be exit", pid);
										//printf("child %d will be exit\n", pid);
										m_sub_process[i].m_pid = -1;
										close( m_sub_process[i].m_pipefd[0] );
									}
								}
							}
							m_stop = true;
							for ( int i = 0; i < m_process_number; i++ )
							{
								if ( m_sub_process[i].m_pid != -1 )
								{
									m_stop = false;
								}
							}
							break;
						}
						case SIGINT:
						case SIGTERM:
						{
							//printf("all the child will be killed\n");
							log( LOG_INFO_L, __FILE__, __LINE__, "all the child will be killed");
							for ( int i = 0; i < m_process_number; i++ )
							{
								int pid = m_sub_process[i].m_pid;
								kill( pid, SIGTERM );
							}
							m_stop = true;
							break;
						}
					}
				}
			}
			else
			{
				continue;
			}
		}
	}

	log( LOG_DEBUG_L, __FILE__, __LINE__, "test for if goes here");
	removefd( m_epollfd, m_listenfd );
	close( m_epollfd );
	//exit(0);
}

template< typename T >
void processpool< T >::run_child()
{
	//m_epollfd = epoll_create( 5 );
	setup_sig_pipe();
	int pipefd = m_sub_process[m_idx].m_pipefd[1];
	addfd( m_epollfd, pipefd );

	epoll_event events[ MAX_EVENT_NUMBER ];
	T *users = new T[ USER_PER_PROCESS ];
	assert( users );
	int number;
	int ret = -1;

	while ( !m_stop )
	{
		number = epoll_wait( m_epollfd, events, MAX_EVENT_NUMBER, -1 );
		log( LOG_DEBUG_L, __FILE__, __LINE__, "test for child");
		if ( (number < 0) && ( errno != EINTR ) )
		{
			log( LOG_INFO_L, __FILE__, __LINE__, "child epoll fail");
			break;
		}

		for ( int i = 0; i < number; i++ )
		{
			int sockfd = events[i].data.fd;
			log( LOG_DEBUG_L, __FILE__, __LINE__, "processpool.h:sockfd is %d", sockfd);

			if ( ( sockfd == pipefd ) && ( events[i].events & EPOLLIN ) )
			{
				int client = 0;
				ret = recv( sockfd, ( char * )&client, sizeof( client ), 0 );
				if ( ( ret < 0 ) && ( errno != EAGAIN ) || ( ret == 0 ) )
				{
					continue;
				}
				else
				{
					struct sockaddr_in client_address;
					socklen_t client_addrlength = sizeof( client_address );
					int connfd = accept( m_listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
					if ( connfd < 0 )
					{
						log( LOG_INFO_L, __FILE__, __LINE__, "accept error,errno is %d", errno);
						continue;
					}
					addfd( m_epollfd, connfd );
					users[connfd].init( m_epollfd, connfd, client_address );
					//users[connfd].accept_request();
				}
			}
			else if ( ( sockfd == sig_pipefd[0] ) && ( events[i].events & EPOLLIN ) )
			{
				int signal;
				int ret = recv( sig_pipefd[0], ( char * )&signal, sizeof( signal ), 0 );
				if ( ret < 0 )
				{
					continue;
				}
				else
				{
					switch ( signal )
					{
						case SIGCHLD:
						{
							log( LOG_DEBUG_L, __FILE__, __LINE__, "test for signal SIGCHLD" );
							pid_t pid;
							int stat;
							while ( ( pid = waitpid( -1, &stat, WNOHANG ) ) > 0 )
							{
								continue;
							}
							break;
						}
						case SIGINT:
						case SIGTERM:
						{
							log( LOG_INFO_L, __FILE__, __LINE__, "process in pool will be stopped" );
							m_stop = true;
							break;
						}
					}
				}
			}
			else if ( events[i].events & EPOLLIN )
			{
				users[sockfd].accept_request();
			}
			else
				continue;
		}
	}

	delete [] users;
	users = NULL;
	close( pipefd );
	close( m_epollfd );
	//exit(0);
}

template< typename T >
void processpool< T >::setup_sig_pipe()
{
	m_epollfd = epoll_create( 5 );
	assert( m_epollfd != -1 );
	int ret = socketpair( PF_UNIX, SOCK_STREAM, 0, sig_pipefd );
	setnonblock( sig_pipefd[1] );
	addfd( m_epollfd, sig_pipefd[0] );
	addsig( SIGCHLD, sig_handler );
	addsig( SIGINT, sig_handler );
	addsig( SIGTERM, sig_handler );
}

#endif