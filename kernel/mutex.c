#include "c-hacks.h"
#include <os/mutex.h>

void mutex_init(struct mutex *lock)
{
	int r;

	r = pthread_mutex_init(&lock->lock, NULL);
	if (r)
		BUG_ON(r);
}

void mutex_destroy(struct mutex *lock)
{
	pthread_mutex_destroy(&lock->lock);
}

void mutex_lock(struct mutex *lock)
{
	pthread_mutex_lock(&lock->lock);
}

void mutex_unlock(struct mutex *lock)
{
	pthread_mutex_unlock(&lock->lock);
}
