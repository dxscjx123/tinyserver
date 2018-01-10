#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "daemon.h"
#include "log.h"

int daemon_init(void)
{
	pid_t pid;

	pid = fork();
	if (pid < 0)
	{
		log(LOG_INFO_L, __FILE__, __LINE__, "daemon fork error");
		return -1;
	}
	else if (pid)
		_exit(0);

	if (setsid() < 0)
	{
		log(LOG_INFO_L, __FILE__, __LINE__, "daemon setsid error");
		return -1;
	}
	signal(SIGHUP, SIG_IGN);

	pid = fork();
	if (pid < 0)
	{
		log(LOG_INFO_L, __FILE__, __LINE__, "daemon second fork error");
		return -1;
	}
	else if (pid)
		_exit(0);
	for (int i = 0; i < 65536; i++)
	{
		close(i);
	}

	open("/dev/null", O_RDONLY);
	open("dev/null", O_RDWR);
	open("dev/null", O_RDWR);

	return 0;
}