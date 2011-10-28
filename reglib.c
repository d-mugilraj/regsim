#define KBUILD_MODNAME "reglib"
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#include "reglib.h"

#ifdef CONFIG_REGLIB_DEBUG
#define REG_DBG_PRINT(format, args...)			\
	printf(pr_fmt(format), ##args)
#else
#define REG_DBG_PRINT(args...)
#endif /* CONFIG_REGLIB_DEBUG */

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
	struct regulatory_request *last_request;
	struct dl_list dev_regd_list;
};

struct ieee80211_regcore reg_core = {
	.regd = &world_regdom,
	.world_regd = &world_regdom,
	.last_request = NULL,
};

struct ieee80211_regcore *regcore = &reg_core;

int reglib_frequency_to_channel(int freq)
{
	if (freq == 2484)
		return 14;

	if (freq < 2484)
		return (freq - 2407) / 5;

	/* FIXME: dot11ChannelStartingFactor (802.11-2007 17.3.8.3.2) */
	return freq/5 - 1000;
}

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

int reglib_freq_info_regd(struct ieee80211_dev_regulatory *reg,
			  uint32_t center_freq,
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
	 * Follow the device's regulatory domain, if present, unless a
	 * country IE has been processed or a user wants to help complaince
	 * further.
	 */
	if (!custom_regd &&
	    regcore->last_request->initiator != IEEE80211_REGDOM_SET_BY_COUNTRY_IE &&
	    regcore->last_request->initiator != IEEE80211_REGDOM_SET_BY_USER &&
	    reg->regd)
		regd = reg->regd;

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

int reglib_freq_info(struct ieee80211_dev_regulatory *reg,
		     uint32_t center_freq,
		     int target_eirp_mbm,
		     uint32_t desired_bw_khz,
		     const struct ieee80211_reg_rule **reg_rule)
{
	return reglib_freq_info_regd(reg,
				     center_freq,
				     target_eirp_mbm,
				     desired_bw_khz,
				     reg_rule,
				     NULL);
}

const struct ieee80211_regdomain *reglib_get_regd(void)
{
	return regcore->regd;
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

#ifdef CONFIG_REGLIB_DEBUG
static const char *reglib_initiator_name(enum nl80211_reg_initiator initiator)
{
	switch (initiator) {
	case IEEE80211_REGDOM_SET_BY_CORE:
		return "Set by core";
	case IEEE80211_REGDOM_SET_BY_USER:
		return "Set by user";
	case IEEE80211_REGDOM_SET_BY_DRIVER:
		return "Set by driver";
	case IEEE80211_REGDOM_SET_BY_COUNTRY_IE:
		return "Set by country IE";
	default:
		WARN_ON(1);
		return "Set by bug";
	}
}

static void chan_reg_rule_print_dbg(struct ieee80211_channel *chan,
				    uint32_t desired_bw_khz,
				    const struct ieee80211_reg_rule *reg_rule)
{
	const struct ieee80211_power_rule *power_rule;
	const struct ieee80211_freq_range *freq_range;
	char max_antenna_gain[32];

	power_rule = &reg_rule->power_rule;
	freq_range = &reg_rule->freq_range;

	if (!power_rule->max_antenna_gain)
		snprintf(max_antenna_gain, 32, "N/A");
	else
		snprintf(max_antenna_gain, 32, "%d", power_rule->max_antenna_gain);

	REG_DBG_PRINT("Updating information on frequency %d MHz "
		      "for a %d MHz width channel with regulatory rule:\n",
		      chan->center_freq,
		      KHZ_TO_MHZ(desired_bw_khz));

	REG_DBG_PRINT("%d KHz - %d KHz @ %d KHz), (%s mBi, %d mBm)\n",
		      freq_range->start_freq_khz,
		      freq_range->end_freq_khz,
		      freq_range->max_bandwidth_khz,
		      max_antenna_gain,
		      power_rule->max_eirp);
}
#else
static void chan_reg_rule_print_dbg(struct ieee80211_channel *chan,
				    uint32_t desired_bw_khz,
				    const struct ieee80211_reg_rule *reg_rule)
{
	return;
}
#endif /* CONFIG_REGLIB_DEBUG */

/*
 * Note that right now we assume the desired channel bandwidth
 * is always 20 MHz for each individual channel (HT40 uses 20 MHz
 * per channel, the primary and the extension channel). To support
 * smaller custom bandwidths such as 5 MHz or 10 MHz we'll need a
 * new ieee80211_channel.target_bw and re run the regulatory check
 * on the wiphy with the target_bw specified. Then we can simply use
 * that below for the desired_bw_khz below.
 */
static void reglib_handle_channel(struct ieee80211_dev_regulatory *reg,
				  enum ieee80211_reg_initiator initiator,
				  enum ieee80211_band band,
				  unsigned int chan_idx)
{
	int r;
	uint32_t flags, bw_flags = 0;
	uint32_t desired_bw_khz = MHZ_TO_KHZ(20);
	const struct ieee80211_reg_rule *reg_rule = NULL;
	const struct ieee80211_power_rule *power_rule = NULL;
	const struct ieee80211_freq_range *freq_range = NULL;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	struct ieee80211_dev_regulatory *request_reg= NULL;

	request_reg = regcore->last_request->reg;

	sband = reg->bands[band];
	BUG_ON(chan_idx >= sband->n_channels);
	chan = &sband->channels[chan_idx];

	flags = chan->orig_flags;

	/*
	 * XXX: add support for keeping track of target EIRP and
	 * cache that into the ieee80211_channel data structure.
	 * This will require updating the target EIRP on any power
	 * change if want to optimize for seeking the best rule
	 * depending on both target power and bandwidth. For now use
	 * an arbitrary max high value.
	 */
	r = reglib_freq_info(reg,
			     MHZ_TO_KHZ(chan->center_freq),
			     DBM_TO_MBM(31.5),
			     desired_bw_khz,
			     &reg_rule);

	if (r) {
		/*
		 * We will disable all channels that do not match our
		 * received regulatory rule unless the hint is coming
		 * from a Country IE and the Country IE had no information
		 * about a band. The IEEE 802.11 spec allows for an AP
		 * to send only a subset of the regulatory rules allowed,
		 * so an AP in the US that only supports 2.4 GHz may only send
		 * a country IE with information for the 2.4 GHz band
		 * while 5 GHz is still supported.
		 */
		if (initiator == IEEE80211_REGDOM_SET_BY_COUNTRY_IE &&
		    r == -ERANGE)
			return;

		REG_DBG_PRINT("Disabling freq %d MHz\n", chan->center_freq);
		chan->flags = IEEE80211_CHAN_DISABLED;
		return;
	}

	chan_reg_rule_print_dbg(chan, desired_bw_khz, reg_rule);

	power_rule = &reg_rule->power_rule;
	freq_range = &reg_rule->freq_range;

	if (freq_range->max_bandwidth_khz < MHZ_TO_KHZ(40))
		bw_flags = IEEE80211_CHAN_NO_HT40;

	if (regcore->last_request->initiator == IEEE80211_REGDOM_SET_BY_DRIVER &&
	    request_reg && request_reg == reg &&
	    request_reg->flags & IEEE80211_REGD_STRICT_REGULATORY) {
		/*
		 * This guarantees the driver's requested regulatory domain
		 * will always be used as a base for further regulatory
		 * settings
		 */
		chan->flags = chan->orig_flags = reg_rule->flags | bw_flags;
		chan->max_antenna_gain = chan->orig_mag =
			(int) MBI_TO_DBI(power_rule->max_antenna_gain);
		chan->max_power = chan->orig_mpwr =
			(int) MBM_TO_DBM(power_rule->max_eirp);
		return;
	}

	chan->beacon_found = false;
	chan->flags = flags | bw_flags | reg_rule->flags;
	chan->max_antenna_gain = min(chan->orig_mag,
		(int) MBI_TO_DBI(power_rule->max_antenna_gain));
	if (chan->orig_mpwr)
		chan->max_power = min(chan->orig_mpwr,
			(int) MBM_TO_DBM(power_rule->max_eirp));
	else
		chan->max_power = (int) MBM_TO_DBM(power_rule->max_eirp);
}

static void reglib_handle_band(struct ieee80211_dev_regulatory *reg,
			       enum ieee80211_band band,
			       enum ieee80211_reg_initiator initiator)
{
	unsigned int i;
	struct ieee80211_supported_band *sband;

	BUG_ON(!reg->bands[band]);
	sband = reg->bands[band];

	for (i = 0; i < sband->n_channels; i++)
		reglib_handle_channel(reg, initiator, band, i);
}

static bool reglib_dev_ignores_update(struct ieee80211_dev_regulatory *reg,
				      enum ieee80211_reg_initiator initiator)
{
	if (!regcore->last_request) {
		REG_DBG_PRINT("Ignoring regulatory request %s since "
			      "last_request is not set\n",
			      reglib_initiator_name(initiator));
		return true;
	}

	if (initiator == IEEE80211_REGDOM_SET_BY_CORE &&
	    reg->flags & IEEE80211_REGDOM_TYPE_CUSTOM_WORLD) {
		REG_DBG_PRINT("Ignoring regulatory request %s "
			      "since the driver uses its own custom "
			      "regulatory domain\n",
			      reglib_initiator_name(initiator));
		return true;
	}

	/*
	 * reg->regd will be set once the device has its own
	 * desired regulatory domain set
	 */
	if (reg->flags & IEEE80211_REGD_STRICT_REGULATORY && !reg->regd &&
	    initiator != IEEE80211_REGDOM_SET_BY_COUNTRY_IE &&
	    !is_world_regdom(regcore->last_request->alpha2)) {
		REG_DBG_PRINT("Ignoring regulatory request %s "
			      "since the driver requires its own regulatory "
			      "domain to be set first\n",
			      reglib_initiator_name(initiator));
		return true;
	}
	return false;
}

void reglib_regdev_update(struct ieee80211_dev_regulatory *reg,
			  enum ieee80211_reg_initiator initiator)
{
	enum ieee80211_band band;

	BUG_ON(!regcore->last_request);

	if (reglib_dev_ignores_update(reg, initiator))
		return;

	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		if (reg->bands[band])
			reglib_handle_band(reg, band, initiator);
	}
}

int reglib_core_init(void)
{
	dl_list_init(&regcore->dev_regd_list);

	return 0;
}
