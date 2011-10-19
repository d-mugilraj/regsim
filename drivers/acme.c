/* ACME Inc. Wireless Driver */

#include <errno.h>
#include "core.h"

/* We use the hw_value as an index into our private channel structure */

#define CHAN2G(_freq, _idx)  { \
	.band = IEEE80211_BAND_2GHZ, \
	.center_freq = (_freq), \
	.hw_value = (_idx), \
	.max_power = 20, \
}

#define CHAN5G(_freq, _idx) { \
	.band = IEEE80211_BAND_5GHZ, \
	.center_freq = (_freq), \
	.hw_value = (_idx), \
	.max_power = 20, \
}

/*
 * Some 2 GHz radios are actually tunable on 2312-2732
 * on 5 MHz steps, we support the channels which we know
 * we have calibration data for all cards though to make
 * this static
 */
static struct ieee80211_channel acme_2ghz_chantable[] = {
	CHAN2G(2412, 0), /* Channel 1 */
	CHAN2G(2417, 1), /* Channel 2 */
	CHAN2G(2422, 2), /* Channel 3 */
	CHAN2G(2427, 3), /* Channel 4 */
	CHAN2G(2432, 4), /* Channel 5 */
	CHAN2G(2437, 5), /* Channel 6 */
	CHAN2G(2442, 6), /* Channel 7 */
	CHAN2G(2447, 7), /* Channel 8 */
	CHAN2G(2452, 8), /* Channel 9 */
	CHAN2G(2457, 9), /* Channel 10 */
	CHAN2G(2462, 10), /* Channel 11 */
	CHAN2G(2467, 11), /* Channel 12 */
	CHAN2G(2472, 12), /* Channel 13 */
	CHAN2G(2484, 13), /* Channel 14 */
};

/*
 * Some 5 GHz radios are actually tunable on XXXX-YYYY
 * on 5 MHz steps, we support the channels which we know
 * we have calibration data for all cards though to make
 * this static
 */
static struct ieee80211_channel acme_5ghz_chantable[] = {
	/* _We_ call this UNII 1 */
	CHAN5G(5180, 14), /* Channel 36 */
	CHAN5G(5200, 15), /* Channel 40 */
	CHAN5G(5220, 16), /* Channel 44 */
	CHAN5G(5240, 17), /* Channel 48 */
	/* _We_ call this UNII 2 */
	CHAN5G(5260, 18), /* Channel 52 */
	CHAN5G(5280, 19), /* Channel 56 */
	CHAN5G(5300, 20), /* Channel 60 */
	CHAN5G(5320, 21), /* Channel 64 */
	/* _We_ call this "Middle band" */
	CHAN5G(5500, 22), /* Channel 100 */
	CHAN5G(5520, 23), /* Channel 104 */
	CHAN5G(5540, 24), /* Channel 108 */
	CHAN5G(5560, 25), /* Channel 112 */
	CHAN5G(5580, 26), /* Channel 116 */
	CHAN5G(5600, 27), /* Channel 120 */
	CHAN5G(5620, 28), /* Channel 124 */
	CHAN5G(5640, 29), /* Channel 128 */
	CHAN5G(5660, 30), /* Channel 132 */
	CHAN5G(5680, 31), /* Channel 136 */
	CHAN5G(5700, 32), /* Channel 140 */
	/* _We_ call this UNII 3 */
	CHAN5G(5745, 33), /* Channel 149 */
	CHAN5G(5765, 34), /* Channel 153 */
	CHAN5G(5785, 35), /* Channel 157 */
	CHAN5G(5805, 36), /* Channel 161 */
	CHAN5G(5825, 37), /* Channel 165 */
};

struct ieee80211_supported_band acme_sband_2g = {
	.channels = acme_2ghz_chantable,
	.band = IEEE80211_BAND_2GHZ,
	.n_channels = ARRAY_SIZE(acme_2ghz_chantable),
};

struct ieee80211_supported_band acme_sband_5g = {
	.channels = acme_5ghz_chantable,
	.band = IEEE80211_BAND_5GHZ,
	.n_channels = ARRAY_SIZE(acme_5ghz_chantable),
};

static void acme_setup_band(struct wifi_dev *wdev,
			    enum ieee80211_band band)
{
	switch (band) {
	case IEEE80211_BAND_2GHZ:
		wdev->bands[band] = &acme_sband_2g;
		break;
	case IEEE80211_BAND_5GHZ:
		wdev->bands[band] = &acme_sband_5g;
		break;
	default:
		break;
	}
}

static int acme_probe(struct device *dev, unsigned int idx)
{
	struct wifi_dev *wdev;

	wdev = wdev_new();
	if (!wdev)
		return -ENOMEM;

	dev->wdev = wdev;
	wdev->dev = dev;

	wdev->idx = idx;

	acme_setup_band(wdev, IEEE80211_BAND_2GHZ);
	acme_setup_band(wdev, IEEE80211_BAND_5GHZ);

	register_wifi_dev(wdev);

	/* XXX: The ACME driver regulatory hint would go next */
	return 0;
}

static void acme_remove(struct device *dev, unsigned int idx)
{
	struct wifi_dev *wdev = dev->wdev;

	wdev_free(wdev);
	dev->wdev = NULL;
}

struct dev_ops acme_ops = {
	.probe      = acme_probe,
	.remove     = acme_remove,
};

struct device acme = {
	.ops = &acme_ops,
};
