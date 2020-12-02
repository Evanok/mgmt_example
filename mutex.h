/**
**

** \file mutex.h
** \brief define header for code to define api mutex
** \author Arthur LAMBERT
** \date 27/10/2016
**
**/

#ifndef MUTEX_H_
# define MUTEX_H_

# include <unistd.h>
# include <stdlib.h>
# include <pthread.h>
# include <semaphore.h>

# include "log.h"

typedef struct mutex_context
{
	int is_own;		/*!< boolean to know if someone is owning mutex */
	pthread_t id;		/*!< pthread id of owner */
	pthread_mutex_t mutex;	/*!< pthread mutex */
	int state;		/*!< allow to store previous cancel state */
} str_mutex_context;

int mutex_init (str_mutex_context* m);
int mutex_destroy (str_mutex_context* m);
int cond_timedwait_nointr (pthread_cond_t * cond, str_mutex_context* m, const struct timespec * abstime);
int sem_wait_nointr (sem_t *sem);
int sem_timedwait_nointr (sem_t * sem, struct timespec* ts);
int mutex_lock (str_mutex_context* m);
int mutex_unlock (str_mutex_context* m);
int mutex_error (str_mutex_context* m);

#endif /* !MUTEX_H_ */
