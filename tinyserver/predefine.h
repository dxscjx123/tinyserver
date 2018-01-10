#ifndef PREDEFINE_H
#define PREDEFINE_H

#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>

#include "log.h"

static int sig_pipefd[2];

static int setnonblock( int fd )
{
	int old_option = fcntl( fd, F_GETFL );
	int new_option = old_option | O_NONBLOCK;
	fcntl( fd, F_SETFL, new_option );
	return old_option; 
}

static void addfd( int epollfd, int fd )
{
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
	setnonblock( fd );
}

static void removefd( int epollfd, int fd )
{
	epoll_ctl( epollfd, EPOLL_CTL_DEL, fd, 0 );
	close( fd );
}

static void sig_handler( int sig )
{
	if ( sig == SIGTERM )
		log( LOG_DEBUG_L, __FILE__, __LINE__, "sig_handler SIGTERM" );
	if ( sig == SIGCHLD )
		log( LOG_DEBUG_L, __FILE__, __LINE__, "sig_handler SIGCHLD" );
	int save_errno = errno;
	int msg = sig;
	send( sig_pipefd[1], ( char * )&msg, sizeof( msg ), 0 );
	errno = save_errno;
}

static void addsig( int sig, void( handler )(int) )
{
	log( LOG_DEBUG_L, __FILE__, __LINE__, "addsig test begin" );
	if ( sig == SIGTERM )
		log( LOG_DEBUG_L, __FILE__, __LINE__, "addsig signal SIGTERM" );
	struct sigaction sa;
	memset( &sa, '\0', sizeof( sa ) );
	sa.sa_handler = handler;
	sigfillset( &sa.sa_mask);
	assert( sigaction( sig, &sa, NULL ) != -1);
}

#endif