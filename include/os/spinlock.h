#ifndef __SPINLOCK_H
#define __SPINLOCK_H

#include <pthread.h>

#define spinlock_t pthread_spinlock_t

void spin_lock_init(spinlock_t *lock);
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif /* __SPINLOCK_H */
