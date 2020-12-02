/**
**

** \file log_private.h
** \brief define private macro function that help to define main macro for logs
** \author Arthur LAMBERT
** \date 10/02/2015
**
**/

#ifndef LOG_PRIVATE_H_
# define LOG_PRIVATE_H_


#define CLEAN_ERRNO() (errno == 0 ? "None" : strerror(errno))

#define DEBUG
#ifdef DEBUG

#define _PRINT_DEBUG(M, ...)							\
	do {									\
		printf("[%s][DEBUG] " M "%s\n", __DIR__, __VA_ARGS__); \
	} while (0)
#else

#define _PRINT_DEBUG(M, ...)

#endif /* NDEBUG */

#define _PRINT_INF_GOTO(M, ...)								\
	do {											\
		printf("[%s][INFO] " M "%s\n", __DIR__, __VA_ARGS__);			\
		errno = 0;									\
		goto error;									\
} while (0)

#define _PRINT_ERR_GOTO(M, ...)								\
	do {											\
		printf("[%s][ERROR] [errno: %s] " M "%s\n", __DIR__, CLEAN_ERRNO(), __VA_ARGS__); \
		errno = 0;									\
		goto error;									\
} while (0)

#define _PRINT_ERR_RET(M, ...)								\
	do {											\
		printf("[%s][ERROR] [errno: %s] " M "%s\n", __DIR__, CLEAN_ERRNO(), __VA_ARGS__); \
		errno = 0;									\
		return -1;									\
	} while (0)

#define _PRINT_ERR(M, ...)								\
	do {											\
		printf("[%s][ERROR] [errno: %s] " M "%s\n", __DIR__, CLEAN_ERRNO(), __VA_ARGS__); \
		errno = 0;									\
} while (0)

#define _PRINT_WARN(M, ...) printf("[%s][WARN] " M "%s\n", __DIR__, __VA_ARGS__)
#define _PRINT_INFO(M, ...) printf("[%s][INFO] " M "%s\n", __DIR__, __VA_ARGS__);

#endif /* !LOG_PRIVATE_H_ */
