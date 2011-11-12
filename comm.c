#include <os/mutex.h>
#include <os/spinlock.h>
#include <os/workqueue.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

#include "list.h"
#include "comm.h"

struct crda_request {
	char alpha2[2];
	struct dl_list list;
};

static struct mutex crda_mutex;
static struct dl_list crda_list;

static void *comm_todo(void *arg);
static DECLARE_WORK(comm_work, comm_todo);

int comm_add_crda_request(const char *alpha2)
{
	struct crda_request *req;

	req = malloc(sizeof(struct crda_request));
	if (!req)
		return -ENOMEM;

	req->alpha2[0] = alpha2[0];
	req->alpha2[1] = alpha2[1];

	mutex_lock(&crda_mutex);
	dl_list_add_tail(&crda_list, &req->list);
	mutex_unlock(&crda_mutex);

	schedule_work(&comm_work);

	return 0;
}

static void comm_process_crda_list(void)
{
	struct crda_request *req, *tmp;

	dl_list_for_each_safe(req, tmp, &crda_list,
			      struct crda_request, list) {
		dl_list_del(&req->list);
		printf("CRDA being run for %c%c\n",
		       req->alpha2[0],
		       req->alpha2[1]);
		/* XXX: actually do the CRDA work */
		sleep(3);
	}
}

static void *comm_todo(void *arg)
{
	mutex_lock(&crda_mutex);
	comm_process_crda_list();
	mutex_unlock(&crda_mutex);

	pthread_exit(NULL);
}

int comm_init(void)
{
	init_work(&comm_work);
	dl_list_init(&crda_list);

	mutex_init(&crda_mutex);

	return 0;
}

void comm_stop(void)
{
	struct crda_request *req, *tmp;

	cancel_work_sync(&comm_work);

	mutex_lock(&crda_mutex);
	if (!dl_list_empty(&crda_list)) {
		dl_list_for_each_safe(req, tmp, &crda_list,
				      struct crda_request, list) {
			dl_list_del(&req->list);
		}
	}
	mutex_unlock(&crda_mutex);

	mutex_destroy(&crda_mutex);
}
