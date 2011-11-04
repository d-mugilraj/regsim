#include <stdbool.h>
#include <pthread.h>

#ifndef __WORKQUEUE_H
#define __WORKQUEUE_H


struct work {
	bool ready;

	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	void *arg;
	void *(*work_cb)(void *arg);
};

#define DECLARE_WORK(_w, _w_cb) \
struct work _w = { \
	.work_cb = _w_cb, \
	.arg = NULL, \
};

extern void *run_work(void *arg);

static inline void init_work(struct work *w)
{
	w->ready = false;

	pthread_mutex_init(&w->mutex, NULL);
	pthread_cond_init(&w->cond, NULL);

	pthread_create(&w->thread, NULL, run_work, (void *) w);
	while(!w->ready);
}

void schedule_work(struct work *w);
void init_work(struct work *w);

#endif /* __WORKQUEUE_H */
