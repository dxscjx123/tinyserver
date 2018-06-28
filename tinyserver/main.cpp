#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <netinet/in.h>

#include "processpool.h"
#include "task.h"
#include "log.h"
#include "daemon.h"

#define DEFAULT_PORT 8001
#define SET_LOG_LEVEL LOG_USER_L

using namespace std;

int main(void)
{
	int listen_fd, reuse, ret;
	struct sockaddr_in servaddr;
	
	listen_fd = socket( AF_INET, SOCK_STREAM, 0);
	if ( listen_fd == -1 )
	{
		perror( "create socket error" );
		exit(0);
	}
	
	if ( setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) ) == -1 )
	{
		perror( "reuse socket error" );
		exit(0);
	}

	memset( &servaddr, 0, sizeof( servaddr) );
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
	servaddr.sin_port = htons( DEFAULT_PORT );

	ret = bind( listen_fd, ( struct sockaddr* )&servaddr, sizeof( servaddr ));
	if ( ret == -1 )
	{
		perror( "bind socket error" );
		exit(0);
	}

	ret = listen( listen_fd, 10 );
	if ( ret == -1 )
	{
		perror(" listen socket error" );
		exit(0);
	}

	setlogprio( SET_LOG_LEVEL );

	//daemon_init();

	processpool< task_conn >* pool = processpool< task_conn >::create( listen_fd );
	if ( pool )
	{
		pool->run();
	}
	delete pool;
	close( listen_fd );
	return 0;
}