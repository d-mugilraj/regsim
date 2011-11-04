#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <os/workqueue.h>

#include "reglib.h"
#include "reg.h"
#include "testreg.h"

pthread_mutex_t regcore_mutex;
pthread_spinlock_t reg_requests_lock;

void *reg_todo(void *arg);
static DECLARE_WORK(reg_work, reg_todo);

/*
 * This lets us keep regulatory code which is updated on a regulatory
 * basis in userspace.
 */
static int call_crda(const char *alpha2)
{
	pthread_mutex_lock(&regcore_mutex);
	if (!reglib_is_world_regdom((char *) alpha2))
		printf("Calling CRDA for country: %c%c\n",
		       alpha2[0], alpha2[1]);
	else
		printf("Calling CRDA to update world regulatory domain\n");

	/* XXX: implement this */
	pthread_mutex_unlock(&regcore_mutex);
	return 0;
}

static void send_reg_change_event(struct regulatory_request *request)
{
	/*
	 * XXX: do something more fancy here to provide a better example of
	 * what this is supposed to do.
	 */
	printf("Regulatory domain changed\n");
	return;
}

static void reg_process_next_hint(void)
{
	struct regulatory_request *request;

	pthread_spin_lock(&reg_requests_lock);
	request = reglib_next_request();
	pthread_spin_unlock(&reg_requests_lock);

	if (!request)
		pthread_exit(NULL);

	reglib_process_hint(request);
	pthread_exit(NULL);
}

static void reg_process_pending_hints(void)
{

	pthread_mutex_lock(&regcore_mutex);
	reg_process_next_hint();
	pthread_mutex_unlock(&regcore_mutex);
}

static void reg_process_pending_beacon_hints(void)
{
	/* XXX: add support for beacon hints */
}

void *reg_todo(void *arg)
{
	reg_process_pending_hints();
	reg_process_pending_beacon_hints();

	pthread_exit(NULL);
}

static void queue_regulatory_request(struct regulatory_request *request)
{
	if (isalpha(request->alpha2[0]))
		request->alpha2[0] = toupper(request->alpha2[0]);
	if (isalpha(request->alpha2[1]))
		request->alpha2[1] = toupper(request->alpha2[1]);

	/*
	 * We use spin_lock for reg_requests_lock as the core request
	 * cannot hold a mutex as __init work in kernel barfs at that.
	 */
	pthread_spin_lock(&reg_requests_lock);
	reglib_queue_request(request);
	pthread_spin_unlock(&reg_requests_lock);

	schedule_work(&reg_work);
}

/*
 * Core regulatory hint -- happens during cfg80211_init()
 * and when we restore regulatory settings.
 */
static int regulatory_hint_core(const char *alpha2)
{
	struct regulatory_request *request;

	request = malloc(sizeof(struct regulatory_request));
	if (!request)
		return -ENOMEM;
	memset(request, 0, sizeof(struct regulatory_request));

	request->alpha2[0] = alpha2[0];
	request->alpha2[1] = alpha2[1];
	request->initiator = IEEE80211_REGDOM_SET_BY_CORE;

	queue_regulatory_request(request);
	return 0;
}

static struct regcore_ops ops = {
	.call_crda = call_crda,
	.send_reg_change_event = send_reg_change_event,
};

int regulatory_init(void)
{
	int r = 0;

	r = pthread_mutex_init(&regcore_mutex, NULL);
	if (r)
		return r;

	r = pthread_spin_init(&reg_requests_lock, PTHREAD_PROCESS_SHARED);
	if (r)
		goto mutex_out;

	init_work(&reg_work);

	r = reglib_core_init(&ops);
	if (r)
		goto out;

	r = regulatory_hint_core("00");
	if (r)
		goto out;

out:
	pthread_spin_destroy(&reg_requests_lock);
mutex_out:
	pthread_mutex_destroy(&regcore_mutex);

	return r;
}

void reg_core_test(void)
{
	pthread_mutex_lock(&regcore_mutex);
	test_regdoms();
	pthread_mutex_unlock(&regcore_mutex);
}
