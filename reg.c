#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <os/mutex.h>
#include <os/spinlock.h>
#include <os/workqueue.h>

#include "reg.h"
#include "testreg.h"
#include "comm.h"

static struct mutex regcore_mutex;
static spinlock_t reg_requests_lock;

void *reg_todo(void *arg);
static DECLARE_WORK(reg_work, reg_todo);

/*
 * This lets us keep regulatory code which is updated on a regulatory
 * basis in userspace.
 */
static int call_crda(const char *alpha2)
{
	if (!reglib_is_world_regdom((char *) alpha2))
		printf("Calling CRDA for country: %c%c\n",
		       alpha2[0], alpha2[1]);
	else
		printf("Calling CRDA to update world regulatory domain\n");

	comm_add_crda_request(alpha2);

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

	spin_lock(&reg_requests_lock);
	request = reglib_next_request();
	spin_unlock(&reg_requests_lock);

	if (!request)
		return;

	reglib_process_hint(request);
}

static void reg_process_pending_hints(void)
{

	mutex_lock(&regcore_mutex);
	reg_process_next_hint();
	mutex_unlock(&regcore_mutex);
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
	spin_lock(&reg_requests_lock);
	reglib_queue_request(request);
	spin_unlock(&reg_requests_lock);

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

void regdev_update(struct ieee80211_dev_regulatory *reg)
{
	reglib_regdev_update(reg, IEEE80211_REGDOM_SET_BY_CORE);
}

int regulatory_init(void)
{
	int r = 0;

	mutex_init(&regcore_mutex);
	spin_lock_init(&reg_requests_lock);

	init_work(&reg_work);

	r = reglib_core_init(&ops);
	if (r)
		return r;

	r = regulatory_hint_core("00");
	if (r)
		return r;

	return r;
}

void regulatory_exit(void)
{
	cancel_work_sync(&reg_work);

	mutex_destroy(&regcore_mutex);
	spin_lock_destroy(&reg_requests_lock);
}

void reg_core_test(void)
{
	mutex_lock(&regcore_mutex);
	test_regdoms();
	mutex_unlock(&regcore_mutex);
}
