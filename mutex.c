#include "mutex.h"

int mutex_init (str_mutex_context* m)
{
	pthread_mutexattr_t attr;

	memset (&attr, 0, sizeof(attr));
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	m->is_own = 0;
	m->state = -1;
	return pthread_mutex_init(&m->mutex, &attr);
}

int mutex_destroy (str_mutex_context* m)
{
	int retry = 0;

	CHECK_ERR_GOTO (m->is_own == -1, "Mutex was already destroy");
	while (m->is_own == 1 && retry < 3)
	{
		PRINT_WARN ("Must wait unlock to destroy a mutex");
		usleep (500);
		retry++;
	}
	if (retry >= 3)
	{
		PRINT_WARN ("Cannot destroy a locked mutex");
		CHECK_ERR_GOTO (mutex_unlock (m), "Cannot unlock mutex");
	}
	m->is_own = -1;
	return pthread_mutex_destroy(&m->mutex);
error:
	return 1;
}

int mutex_lock (str_mutex_context* m)
{
	pthread_t id;

	CHECK_ERR_RET (m->is_own == -1, "Cannot lock unitialized mutex");
	CHECK_ERR_GOTO (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &m->state) != 0, "Cannot set cancel state");
	id = pthread_self();
	CHECK_ERR_GOTO (pthread_equal (id, m->id) != 0, "Mutex already lock by this thread. dead lock !");
	CHECK_ERR_GOTO (pthread_mutex_lock (&m->mutex) != 0, "Cannot lock mutex");
	m->is_own = 1;
	m->id = id;
	return 0;
error:
	if (m->state != -1)
		CHECK_WARN (pthread_setcancelstate(m->state, NULL) != 0, "Cannot set cancel state");
	return 1;
}

int mutex_unlock (str_mutex_context* m)
{
	m->is_own = 0;
	m->id = 0;
	CHECK_ERR_GOTO (pthread_mutex_unlock (&m->mutex) != 0, "Cannot unlock mutex");
	if (m->state != -1)
		CHECK_ERR_GOTO (pthread_setcancelstate(m->state, NULL) != 0, "Cannot set cancel state");
	return 0;
error:
	CHECK_WARN (pthread_setcancelstate(m->state, NULL) != 0, "Cannot set cancel state");
	return 1;
}


int cond_timedwait_nointr (pthread_cond_t * cond, str_mutex_context* m, const struct timespec * abstime)
{
	// This is used between lock and unlock, so there is no need to touch the cancel state
	m->is_own = 0;
	while (pthread_cond_timedwait(cond, &m->mutex, abstime))
	{
		if (errno == EINTR)
			errno = 0;
		else
			PRINT_ERR_GOTO ("Time out on cond wait");
	}
	m->is_own = 1;
	m->id = pthread_self();
	return 0;
error:
	m->is_own = 1;
	m->id = pthread_self();
	return 1;
}

int sem_wait_nointr (sem_t * sem)
{
	while (sem_wait(sem))
	{
		if (errno == EINTR)
			errno = 0;
		else
			PRINT_ERR_GOTO ("Error on sem wait");
	}
	return 0;
error:
	return 1;
}

int sem_timedwait_nointr (sem_t * sem, struct timespec* ts)
{
	while (sem_timedwait(sem, ts))
	{
		if (errno == EINTR)
			errno = 0;
		else
			return 1;
	}
	return 0;
}


int mutex_error (str_mutex_context* m)
{
	pthread_t my_id = pthread_self();

	// noone is owning the mutex. Nothing to unlock
	if (m->is_own == 0) return 0;

	// thread id are equals ! I am the owner of the mutex !
	if (pthread_equal (my_id, m->id) != 0)
	{
		CHECK_WARN (mutex_unlock (m) != 0, "Cannot unlock mutex");
	}
	return 0;
}
