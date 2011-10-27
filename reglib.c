#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#include "regulatory.h"

/* We keep a static world regulatory domain in case of the absence of CRDA */
static const struct ieee80211_regdomain world_regdom = {
	.n_reg_rules = 5,
	.alpha2 =  "00",
	.reg_rules = {
		/* IEEE 802.11b/g, channels 1..11 */
		REG_RULE(2412-10, 2462+10, 40, 6, 20, 0),
		/* IEEE 802.11b/g, channels 12..13. No HT40
		 * channel fits here. */
		REG_RULE(2467-10, 2472+10, 20, 6, 20,
			IEEE80211_RRF_PASSIVE_SCAN |
			IEEE80211_RRF_NO_IR),
		/* IEEE 802.11 channel 14 - Only JP enables
		 * this and for 802.11b only */
		REG_RULE(2484-10, 2484+10, 20, 6, 20,
			IEEE80211_RRF_PASSIVE_SCAN |
			IEEE80211_RRF_NO_IR |
			IEEE80211_RRF_NO_OFDM),
		/* IEEE 802.11a, channel 36..48 */
		REG_RULE(5180-10, 5240+10, 40, 6, 20,
                        IEEE80211_RRF_PASSIVE_SCAN |
                        IEEE80211_RRF_NO_IR),

		/* NB: 5260 MHz - 5700 MHz requies DFS */

		/* IEEE 802.11a, channel 149..165 */
		REG_RULE(5745-10, 5825+10, 40, 6, 20,
			IEEE80211_RRF_PASSIVE_SCAN |
			IEEE80211_RRF_NO_IR),
	}
};

/**
 * struct ieee80211_regcore - the regulatory core
 *
 * This structure provides a unified view of the regulatory data
 * used by an 802.11 subsystem. It is passed to all regulatory library
 * routines.
 *
 * @regd: pointer to the subsystem's currently set regultory domain
 * @world_regd: pointer to the subsystem's world regulatory domain
 * @dev_regd_list: list of known registered 802.11 device's regulatory
 *	data. This is used by the regulatory library when it needs to
 *	iterate over all devices.
 */
struct ieee80211_regcore {
	const struct ieee80211_regdomain *regd;
	const struct ieee80211_regdomain *world_regd;
	struct dl_list dev_regd_list;
};

struct ieee80211_regcore reg_core = {
	.regd = &world_regdom,
	.world_regd = &world_regdom,
};

struct ieee80211_regcore *regcore = &reg_core;

bool is_world_regdom(const char *alpha2)
{
	if (!alpha2)
		return false;
	if (alpha2[0] == '0' && alpha2[1] == '0')
		return true;
	return false;
}

/* Sanity check on a regulatory rule */
static bool is_valid_reg_rule(const struct ieee80211_reg_rule *rule)
{
	const struct ieee80211_freq_range *freq_range = &rule->freq_range;
	uint32_t freq_diff;

	if (freq_range->start_freq_khz <= 0 || freq_range->end_freq_khz <= 0)
		return false;

	if (freq_range->start_freq_khz > freq_range->end_freq_khz)
		return false;

	freq_diff = freq_range->end_freq_khz - freq_range->start_freq_khz;

	if (freq_range->end_freq_khz <= freq_range->start_freq_khz ||
			freq_range->max_bandwidth_khz > freq_diff)
		return false;

	return true;
}

bool reglib_is_valid_rd(const struct ieee80211_regdomain *rd)
{
	const struct ieee80211_reg_rule *reg_rule = NULL;
	unsigned int i;

	if (!rd->n_reg_rules)
		return false;

	/*
	 * we use 32 for now as the maximum under the assumption we
	 * don't want to allocate memory for more. Right now at least
	 * on the Linux kernel we dynamically allocate memory for
	 * a regulatory domain so we decided to limit this to 32.
	 */
	if (rd->n_reg_rules > 32)
		return false;

	for (i = 0; i < rd->n_reg_rules; i++) {
		reg_rule = &rd->reg_rules[i];
		if (!is_valid_reg_rule(reg_rule))
			return false;
	}

	return true;
}

static bool reg_does_bw_fit(const struct ieee80211_freq_range *freq_range,
			    uint32_t center_freq_khz,
			    uint32_t bw_khz)
{
	uint32_t start_freq_khz, end_freq_khz;

	start_freq_khz = center_freq_khz - (bw_khz/2);
	end_freq_khz = center_freq_khz + (bw_khz/2);

	if (start_freq_khz >= freq_range->start_freq_khz &&
	    end_freq_khz <= freq_range->end_freq_khz)
		return true;

	return false;
}

/**
 * freq_in_rule_band - tells us if a frequency is in a frequency band
 * @freq_range: frequency rule we want to query
 * @freq_khz: frequency we are inquiring about
 *
 * This lets us know if a specific frequency rule is or is not relevant to
 * a specific frequency's band. Bands are device specific and artificial
 * definitions (the "2.4 GHz band" and the "5 GHz band"), however it is
 * safe for now to assume that a frequency rule should not be part of a
 * frequency's band if the start freq or end freq are off by more than 2 GHz.
 * This resolution can be lowered and should be considered as we add
 * regulatory rule support for other "bands".
 **/
static bool freq_in_rule_band(const struct ieee80211_freq_range *freq_range,
			      uint32_t freq_khz)
{
#define ONE_GHZ_IN_KHZ	1000000
	if (abs(freq_khz - freq_range->start_freq_khz) <= (2 * ONE_GHZ_IN_KHZ))
		return true;
	if (abs(freq_khz - freq_range->end_freq_khz) <= (2 * ONE_GHZ_IN_KHZ))
		return true;
	return false;
#undef ONE_GHZ_IN_KHZ
}

int reglib_freq_info_regd(uint32_t center_freq,
			  int target_eirp_mbm,
			  uint32_t desired_bw_khz,
			  const struct ieee80211_reg_rule **reg_rule,
			  const struct ieee80211_regdomain *custom_regd)
{
	int i;
	bool band_rule_found = false;
	const struct ieee80211_regdomain *regd;
	bool bw_fits = false;

	if (!desired_bw_khz)
		desired_bw_khz = MHZ_TO_KHZ(20);

	regd = custom_regd ? custom_regd : regcore->world_regd;

	/*
	 * XXX: Follow the device's regulatory domain, if present, unless a
	 * country IE has been processed or a user wants to help complaince
	 * further
	 */

	if (!regd)
		return -EINVAL;

	for (i = 0; i < regd->n_reg_rules; i++) {
		const struct ieee80211_reg_rule *rr;
		const struct ieee80211_freq_range *fr = NULL;
		const struct ieee80211_power_rule *pr = NULL;

		rr = &regd->reg_rules[i];
		fr = &rr->freq_range;
		pr = &rr->power_rule;

		/*
		 * We only need to know if one frequency rule was
		 * was in center_freq's band, that's enough, so lets
		 * not overwrite it once found
		 */
		if (!band_rule_found)
			band_rule_found = freq_in_rule_band(fr, center_freq);

		bw_fits = reg_does_bw_fit(fr,
					  center_freq,
					  desired_bw_khz);

		if (band_rule_found && bw_fits &&
		    target_eirp_mbm <= pr->max_eirp) {
			*reg_rule = rr;
			return 0;
		}
	}

	if (!band_rule_found)
		return -ERANGE;

	return -EINVAL;
}

int reglib_freq_info(uint32_t center_freq,
		     int target_eirp_mbm,
		     uint32_t desired_bw_khz,
		     const struct ieee80211_reg_rule **reg_rule)
{
	return reglib_freq_info_regd(center_freq,
				     target_eirp_mbm,
				     desired_bw_khz,
				     reg_rule,
				     NULL);
}

static void print_rd_rules(const struct ieee80211_regdomain *rd)
{
	unsigned int i;
	const struct ieee80211_reg_rule *reg_rule = NULL;
	const struct ieee80211_freq_range *freq_range = NULL;
	const struct ieee80211_power_rule *power_rule = NULL;

	printf("    (start_freq - end_freq @ bandwidth), (max_antenna_gain, max_eirp)\n");

	for (i = 0; i < rd->n_reg_rules; i++) {
		reg_rule = &rd->reg_rules[i];
		freq_range = &reg_rule->freq_range;
		power_rule = &reg_rule->power_rule;

		/*
		 * There may not be documentation for max antenna gain
		 * in certain regions
		 */
		if (power_rule->max_antenna_gain)
			printf("    (%d KHz - %d KHz @ %d KHz), (%d mBi, %d mBm)\n",
			       freq_range->start_freq_khz,
			       freq_range->end_freq_khz,
			       freq_range->max_bandwidth_khz,
			       power_rule->max_antenna_gain,
			       power_rule->max_eirp);
		else
			printf("    (%d KHz - %d KHz @ %d KHz), (N/A, %d mBm)\n",
			       freq_range->start_freq_khz,
			       freq_range->end_freq_khz,
			       freq_range->max_bandwidth_khz,
			       power_rule->max_eirp);
	}
}

void reglib_print_regdomain(const struct ieee80211_regdomain *rd)
{
	if (is_world_regdom(rd->alpha2))
		printf("World regulatory domain updated:\n");
	else
		printf("Regulatory domain changed to country: %c%c\n",
		       rd->alpha2[0], rd->alpha2[1]);

	print_rd_rules(rd);
}

int reglib_core_init(void)
{
	dl_list_init(&regcore->dev_regd_list);

	return 0;
}

void reglib_regdev_update(struct ieee80211_dev_regulatory *reg,
			  enum ieee80211_reg_initiator initiator)
{
	/* XXX */
}

