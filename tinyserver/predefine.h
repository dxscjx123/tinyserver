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
#include <errno.h>

#include "log.h"

extern int sig_pipefd[2];

int setnonblock(int fd);
void addfd(int epollfd, int fd);
void removefd(int epollfd, int fd);
void sig_handler(int sig);
void addsig(int sig, void(handler)(int));

#endif