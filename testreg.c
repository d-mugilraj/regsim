#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

#include "regulatory.h"

/*
 * Purpose: test a regulatory domain with overlapping frequency
 * rules. This allows low for different max EIRP values depending
 * on the assumed max bandwidth used.
 */
static const struct ieee80211_regdomain test_regdom_01 = {
	.n_reg_rules = 2,
	.alpha2 =  "01",
	.reg_rules = {
		/* Channels (149 - 161], allows only 20 MHz wide channels */
		REG_RULE(5745-10, 5805+10, 20, 6, 15, 0),
		/* Channels (153 - 165], allows 40 MHz wide channels */
		REG_RULE(5765-10, 5825+10, 40, 6, 20, 0),
	}
};

static int test_freq_khz_on_rd(uint32_t center_freq_khz,
			       int target_eirp_mbm,
			       const struct ieee80211_regdomain *rd)
{
	/*
	 * XXX: Whether or not we support HT40 will depend on HT+ or HT-
	 * so a channel map will need to be built, the same will be required
	 * for new 802.11ac HT80 and so on
	 */
	const uint32_t desired_bws_khz[] = {
		MHZ_TO_KHZ(5),
		MHZ_TO_KHZ(10),
		MHZ_TO_KHZ(20),
		MHZ_TO_KHZ(40),
	};
	uint32_t desired_bw_khz;
	const struct ieee80211_reg_rule *reg_rule = NULL;
	unsigned int x;
	bool one_bw_works = false, last_bw_worked = false;
	int r;

	for (x = 0; x < ARRAY_SIZE(desired_bws_khz); x++) {
		desired_bw_khz = desired_bws_khz[x];
		r = reglib_freq_info_regd(center_freq_khz,
					  target_eirp_mbm,
					  desired_bw_khz,
					  &reg_rule,
					  rd);
		if (r)
			continue;

		if (last_bw_worked)
			printf(" ");

		last_bw_worked = true;

		if (!one_bw_works) {
			uint32_t center_freq_mhz = KHZ_TO_MHZ(center_freq_khz);

			one_bw_works = true;

			printf("%12d\t%15d\t\t%15d\t\t",
			       ieee80211_frequency_to_channel(center_freq_mhz),
			       center_freq_mhz,
			       MBM_TO_DBM(target_eirp_mbm));
		}

		printf("(@%d, %d)",
		       KHZ_TO_MHZ(desired_bw_khz),
		       MBM_TO_DBM(reg_rule->power_rule.max_eirp));
	}

	if (!one_bw_works)
		return -EINVAL;

	return 0;
}

/* Sweep test on all possible combinations */
static void __test_regdom(const struct ieee80211_regdomain *rd)
{
	const uint32_t center_freqs_khz[] = {
		MHZ_TO_KHZ(2412),
		MHZ_TO_KHZ(2417),
		MHZ_TO_KHZ(2422),
		MHZ_TO_KHZ(2427),
		MHZ_TO_KHZ(2432),
		MHZ_TO_KHZ(2437),
		MHZ_TO_KHZ(2442),
		MHZ_TO_KHZ(2447),
		MHZ_TO_KHZ(2452),
		MHZ_TO_KHZ(2457),
		MHZ_TO_KHZ(2462),
		MHZ_TO_KHZ(2467),
		MHZ_TO_KHZ(2472),
		MHZ_TO_KHZ(5180),
		MHZ_TO_KHZ(5200),
		MHZ_TO_KHZ(5220),
		MHZ_TO_KHZ(5240),
		MHZ_TO_KHZ(5260),
		MHZ_TO_KHZ(5280),
		MHZ_TO_KHZ(5300),
		MHZ_TO_KHZ(5320),
		MHZ_TO_KHZ(5500),
		MHZ_TO_KHZ(5520),
		MHZ_TO_KHZ(5540),
		MHZ_TO_KHZ(5560),
		MHZ_TO_KHZ(5580),
		MHZ_TO_KHZ(5600),
		MHZ_TO_KHZ(5620),
		MHZ_TO_KHZ(5640),
		MHZ_TO_KHZ(5660),
		MHZ_TO_KHZ(5680),
		MHZ_TO_KHZ(5700),
		MHZ_TO_KHZ(5745),
		MHZ_TO_KHZ(5765),
		MHZ_TO_KHZ(5785),
		MHZ_TO_KHZ(5805),
		MHZ_TO_KHZ(5825),
	};
	uint32_t center_freq_khz;
	const uint32_t target_eirps_mbm[] = {
		DBM_TO_MBM(5),
		DBM_TO_MBM(20), /* typical */
		/*
		 * 30 dBm happens to be the max in some regulatory domains
		 * under the assumption of 6 dBi antenna gain.
		 */
		DBM_TO_MBM(30),
		DBM_TO_MBM(31.5), /* MAX_RATE_POWER from Atheros hardware */
		DBM_TO_MBM(36),
	};
	int target_eirp_mbm;
	unsigned int i, j;
	int r;

	printf("%12s\t%15s\t\t%15s\t\t%16s\n",
	       "IEEE-Channel",
	       "Center-freq-MHz",
	       "Target EIRP dBm",
	       "(@Bandwidth MHz, Max EIRP dBm)");

	for (i = 0; i < ARRAY_SIZE(center_freqs_khz); i++) {
		center_freq_khz = center_freqs_khz[i];
		for (j = 0; j < ARRAY_SIZE(target_eirps_mbm); j++) {
			target_eirp_mbm = target_eirps_mbm[j];
			r = test_freq_khz_on_rd(center_freq_khz,
						target_eirp_mbm,
						rd);
			if (!r)
				printf("\n");
		}
	}
}

static void test_regdom(const struct ieee80211_regdomain *rd)
{
	printf("=================================================================================\n");
	if (!reglib_is_valid_rd(rd)) {
		printf("Invalid regulatory domain\n");
		return;
	}

	reglib_print_regdomain(rd);
	printf("---------------------------------------------------------------------------------\n");
	__test_regdom(rd);
}

/*
 * Add more regulatory domains as your heart sees fit to test. This is to be
 * used mainly to test the regulatory simulator for possible corner cases and
 * functionality.
 */
void test_regdoms(void)
{
	const struct ieee80211_regdomain *regdoms[] = {
		&test_regdom_01,
	};
	int i;

	test_regdom(reglib_get_regd());

	for (i = 0; i < ARRAY_SIZE(regdoms); i++)
		test_regdom(regdoms[i]);
}
