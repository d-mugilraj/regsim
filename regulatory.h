#ifndef __NET_REGULATORY_H
#define __NET_REGULATORY_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "ieee80211.h"
#include "list.h"
#include "c-hacks.h"

/*
 * regulatory support structures
 *
 * This file contains data which is independent of any infrastructure
 * definitions of a channel or a wireless device.
 *
 * Copyright 2008-2009	Luis R. Rodriguez <mcgrof@qca.qualcomm.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


/**
 * enum environment_cap - Environment parsed from country IE
 * @ENVIRON_ANY: indicates country IE applies to both indoor and
 *	outdoor operation.
 * @ENVIRON_INDOOR: indicates country IE applies only to indoor operation
 * @ENVIRON_OUTDOOR: indicates country IE applies only to outdoor operation
 */
enum environment_cap {
	ENVIRON_ANY,
	ENVIRON_INDOOR,
	ENVIRON_OUTDOOR,
};

/**
 * struct regulatory_request - used to keep track of regulatory requests
 *
 * @wiphy_idx: this is set if this request's initiator is
 * 	%REGDOM_SET_BY_COUNTRY_IE or %REGDOM_SET_BY_DRIVER. This
 * 	can be used by the wireless core to deal with conflicts
 * 	and potentially inform users of which devices specifically
 * 	cased the conflicts.
 * @initiator: indicates who sent this request, could be any of
 * 	of those set in ieee80211_reg_initiator (%NL80211_REGDOM_SET_BY_*)
 * @alpha2: the ISO / IEC 3166 alpha2 country code of the requested
 * 	regulatory domain. We have a few special codes:
 * 	00 - World regulatory domain
 * 	99 - built by driver but a specific alpha2 cannot be determined
 * 	98 - result of an intersection between two regulatory domains
 *	97 - regulatory domain has not yet been configured
 * @intersect: indicates whether the wireless core should intersect
 * 	the requested regulatory domain with the presently set regulatory
 * 	domain.
 * @processed: indicates whether or not this requests has already been
 *	processed. When the last request is processed it means that the
 *	currently regulatory domain set on cfg80211 is updated from
 *	CRDA and can be used by other regulatory requests. When a
 *	the last request is not yet processed we must yield until it
 *	is processed before processing any new requests.
 * @country_ie_checksum: checksum of the last processed and accepted
 * 	country IE
 * @country_ie_env: lets us know if the AP is telling us we are outdoor,
 * 	indoor, or if it doesn't matter
 * @list: used to insert into the reg_requests_list linked list
 */
struct regulatory_request {
	int wiphy_idx;
	enum ieee80211_reg_initiator initiator;
	char alpha2[2];
	bool intersect;
	bool processed;
	enum environment_cap country_ie_env;
	struct dl_list list;
};

struct ieee80211_freq_range {
	uint32_t start_freq_khz;
	uint32_t end_freq_khz;
	uint32_t max_bandwidth_khz;
};

struct ieee80211_power_rule {
	uint32_t max_antenna_gain;
	uint32_t max_eirp;
};

struct ieee80211_reg_rule {
	struct ieee80211_freq_range freq_range;
	struct ieee80211_power_rule power_rule;
	uint32_t flags;
};

struct ieee80211_regdomain {
	uint32_t n_reg_rules;
	char alpha2[2];
	struct ieee80211_reg_rule reg_rules[];
};

#define MHZ_TO_KHZ(freq) ((freq) * 1000)
#define KHZ_TO_MHZ(freq) ((freq) / 1000)
#define DBI_TO_MBI(gain) ((gain) * 100)
#define MBI_TO_DBI(gain) ((gain) / 100)
#define DBM_TO_MBM(gain) ((gain) * 100)
#define MBM_TO_DBM(gain) ((gain) / 100)

#define REG_RULE(start, end, bw, gain, eirp, reg_flags) \
{							\
	.freq_range.start_freq_khz = MHZ_TO_KHZ(start),	\
	.freq_range.end_freq_khz = MHZ_TO_KHZ(end),	\
	.freq_range.max_bandwidth_khz = MHZ_TO_KHZ(bw),	\
	.power_rule.max_antenna_gain = DBI_TO_MBI(gain),\
	.power_rule.max_eirp = DBM_TO_MBM(eirp),	\
	.flags = reg_flags,				\
}

int ieee80211_frequency_to_channel(int freq);

#endif
