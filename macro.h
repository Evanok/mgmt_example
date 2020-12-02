/**
**

** \file macro.h
** \brief define all macro for middleware
** \author Arthur LAMBERT
** \date 31/10/2016
**
**/

#ifndef MACRO_H_
# define MACRO_H_

# include <unistd.h>
# include <stdlib.h>
# include <syslog.h>
# include <stdio.h>
# include <string.h>
# include <errno.h>
# include "log.h"

#ifdef __arm__
#define TIME(t) time(t)
#else
#define TIME(t) get_simu_time()
#endif

#define FREE(p)					\
	do {					\
		if (p) free (p);		\
		p = NULL;			\
	} while (0)

#define CLOSE(fd)				\
	do {					\
		if (fd >= 0) close (fd);	\
		fd = -1;			\
	} while (0)

#define FSIZE(file) ({int ret; fseek(file, 0L, SEEK_END); ret = ftell (file); ret;})

#define FCLOSE(file)				\
	do {					\
		if (file) fclose (file);	\
		file = NULL;			\
	} while (0)

#define PCLOSE(file)				\
	do {					\
		if (file) pclose (file);	\
		file = NULL;			\
	} while (0)

#define CLOSEDIR(dir)				\
	do {					\
		if (dir) closedir (dir);	\
		dir = NULL;			\
	} while (0)

// Malloc
#define MALLOC(destination, size)						\
	do {									\
		CHECK_ERR_GOTO(destination != NULL, "Already allocated");	\
		destination = malloc(size);					\
		CHECK_ERR_GOTO(destination == NULL, "Cannot malloc");		\
	} while (0)

// Calloc
#define CALLOC(destination, type, size)						\
	do {									\
		CHECK_ERR_GOTO(destination != NULL, "Already allocated");	\
		destination = (type *) calloc(size, sizeof(type));		\
		CHECK_ERR_GOTO(destination == NULL, "Cannot calloc");		\
	} while (0)


#define FREE_ARRAY(a)\
	if (a != NULL)\
	{\
		if (a[0] != NULL)\
		{\
			free(a[0]);\
			a[0] = NULL;\
		}\
		free(a);\
		a = NULL;\
	}

#define FREE_ARRAY_ARRAY(a) \
	if (a != NULL) \
	{ \
		if (a[0] != NULL) \
		{ \
			if (a[0][0] != NULL) \
			{ \
				free(a[0][0]); \
				a[0][0] = NULL; \
			} \
			free(a[0]); \
			a[0] = NULL; \
		} \
		free(a); \
		a = NULL; \
	}

#define FREE_ARRAY_LOOP(a,n,i) \
	if (a != NULL) \
	{ \
		for (i = 0; i < n; i++) \
		{ \
			if ((a)[i] != NULL) \
			{ \
				free((a)[i]); \
				(a)[i] = NULL; \
			} \
		} \
		free(a); \
		a = NULL; \
	}


#endif /* !MACRO_H_ */
