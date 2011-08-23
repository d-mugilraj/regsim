#include <stdio.h>

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

static const struct ieee80211_regdomain *ieee80211_world_regdom =
	&world_regdom;

bool is_world_regdom(const char *alpha2)
{
	if (!alpha2)
		return false;
	if (alpha2[0] == '0' && alpha2[1] == '0')
		return true;
	return false;
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

static void print_regdomain(const struct ieee80211_regdomain *rd)
{
	if (is_world_regdom(rd->alpha2))
		printf("World regulatory domain updated:\n");
	else
		printf("Regulatory domain changed to country: %c%c\n",
		       rd->alpha2[0], rd->alpha2[1]);

	print_rd_rules(rd);
}

int main(void)
{
	print_regdomain(ieee80211_world_regdom);
}
