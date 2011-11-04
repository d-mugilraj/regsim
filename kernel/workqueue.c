#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <os/workqueue.h>

#include "c-hacks.h"

void schedule_work(struct work *w)
{
	pthread_mutex_lock(&w->mutex);
	pthread_cond_signal(&w->cond);
	pthread_mutex_unlock(&w->mutex);
}

void *run_work(void *arg)
{
	struct work *w;
	int r;

	w = (struct work *) arg;

	pthread_mutex_lock(&w->mutex);

	while (true) {
		if (!w->ready)
			w->ready = true;
		r = pthread_cond_wait(&w->cond, &w->mutex);
		if (r != 0) {
			printf("(%s)\n", strerror(r));
			BUG_ON(r);
		}
		w->work_cb(w->arg);
	}

	pthread_mutex_unlock(&w->mutex);

	pthread_exit(NULL);
}
