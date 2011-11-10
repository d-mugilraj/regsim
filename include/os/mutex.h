#ifndef __MUTEX_H
#define __MUTEX_H

#include <pthread.h>

struct mutex {
	pthread_mutex_t lock;
};

void mutex_init(struct mutex *lock);
void mutex_lock(struct mutex *lock);
void mutex_unlock(struct mutex *lock);

#endif /* __MUTEX_H */
