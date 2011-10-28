#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "reglib.h"
#include "reg.h"

/* XXX: add locking / scheduling support for this */

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

	/* XXX: implement this */
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

	/*
	 * XXX: impement queing and queue the request instead of
	 * processing immediately
	 */
	//queue_regulatory_request(request);
	reglib_process_hint(request);

	return 0;
}

static struct regcore_ops ops = {
	.call_crda = call_crda,
	.send_reg_change_event = send_reg_change_event,
};

int regulatory_init(void)
{
	int r;

	r = reglib_core_init(&ops);
	if (r)
		return r;
	regulatory_hint_core("00");

	return 0;
}
