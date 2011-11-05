#include "c-hacks.h"
#include <os/spinlock.h>

void spin_lock_init(spinlock_t *lock)
{
	int r;

	r = pthread_spin_init(lock, PTHREAD_PROCESS_SHARED);
	if (r)  
		BUG_ON(r);
}

void spin_lock(spinlock_t *lock)
{
	pthread_spin_lock(lock);
}

void spin_unlock(spinlock_t *lock)
{
	pthread_spin_unlock(lock);
}
