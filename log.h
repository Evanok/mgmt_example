/**
**

** \file log.h
** \brief define header for logs feature
** \author Arthur LAMBERT
** \date 27/10/2014
**
**/

#ifndef LOG_H_
# define LOG_H_

# include <unistd.h>
# include <stdlib.h>
# include <syslog.h>
# include <stdio.h>
# include <string.h>
# include <errno.h>

# include "log_private.h"

#define MAX_PATH 4096

/*
 Add -DDEBUG in CFLAGS to enable debug logs
*/

/** Print error message and go to error label */
#define PRINT_ERR_GOTO(...) _PRINT_ERR_GOTO(__VA_ARGS__, "")
/** Print error message and return -1*/
#define PRINT_ERR_RET(...) _PRINT_ERR_RET(__VA_ARGS__, "")
/** Print error message*/
#define PRINT_ERR(...) _PRINT_ERR(__VA_ARGS__, "")
/** Print warning message */
#define PRINT_WARN(...) _PRINT_WARN(__VA_ARGS__, "")
/** Print info message */
#define PRINT_INFO(...) _PRINT_INFO(__VA_ARGS__, "")

/** Print debug message */
#define PRINT_DEBUG(...) _PRINT_DEBUG(__VA_ARGS__, "")

/* Use this macro to check return value of function */
/** Check condition on A : print info message and go to error label */
#define CHECK_INF_GOTO(A, ...) if((A)) { _PRINT_INF_GOTO(__VA_ARGS__, "");}
/** Check condition on A : print error message and go to error label */
#define CHECK_ERR_GOTO(A, ...) if((A)) { _PRINT_ERR_GOTO(__VA_ARGS__, "");}
/** Check condition on A : print error message and return -1 */
#define CHECK_ERR_RET(A, ...) if((A)) { _PRINT_ERR_RET(__VA_ARGS__, "");}
/** Check condition on A and print warning message */
#define CHECK_WARN(A, ...) if((A)) { _PRINT_WARN(__VA_ARGS__, "");}
/** Check condition on A and print warning message */
#define CHECK_INFO(A, ...) if((A)) { _PRINT_INFO(__VA_ARGS__, "");}

#endif /* !LOG_H_ */
