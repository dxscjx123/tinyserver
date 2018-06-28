#include "log.h"

int log_level = LOG_USER_L;

void setlogprio(int level)
{
	log_level = level;
}

void log(int level, const char *filename, int linenum, const char *format, ... )
{
	if ( level > log_level )
	{
		return ;
	}
	switch ( level )
	{
		case LOG_INFO_L:
		{
			printf( "[LOG_INFO] " );
			break;
		}
		case LOG_DEBUG_L:
		{
			printf( "[LOG_DEBUG] " );
			break;
		}
		default:
		{
			return ;
		}
	}
	printf( "[%s:", filename );
	printf( "%d] ", linenum );

	char buff[100];
	va_list arg_list;
	va_start( arg_list, format );
	vsnprintf( buff, sizeof( buff ), format, arg_list );
	va_end( arg_list );
	printf( "%s\n", buff );
	fflush( stdout );
}