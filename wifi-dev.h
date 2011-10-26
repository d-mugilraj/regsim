#ifndef __WIFI_DEV_H
#define __WIFI_DEV_H

#include <stdint.h>
#include <stdbool.h>

#include "ieee80211.h"

/*
 * Device driver channel definition templates for an 802.11 subsystem.
 */

/**
 * enum ieee80211_channel_flags - channel flags
 *
 * Channel flags set by the regulatory control code.
 *
 * @IEEE80211_CHAN_DISABLED: This channel is disabled.
 * @IEEE80211_CHAN_PASSIVE_SCAN: Only passive scanning is permitted
 *	on this channel.
 * @IEEE80211_CHAN_NO_IBSS: IBSS is not allowed on this channel.
 * @IEEE80211_CHAN_RADAR: Radar detection is required on this channel.
 * @IEEE80211_CHAN_NO_HT40PLUS: extension channel above this channel
 *	is not permitted.
 * @IEEE80211_CHAN_NO_HT40MINUS: extension channel below this channel
 *	is not permitted.
 */
enum ieee80211_channel_flags {
	IEEE80211_CHAN_DISABLED         = 1<<0,
	IEEE80211_CHAN_PASSIVE_SCAN     = 1<<1,
	IEEE80211_CHAN_NO_IBSS          = 1<<2,
	IEEE80211_CHAN_RADAR            = 1<<3,
	IEEE80211_CHAN_NO_HT40PLUS      = 1<<4,
	IEEE80211_CHAN_NO_HT40MINUS     = 1<<5,
};

#define IEEE80211_CHAN_NO_HT40 \
	(IEEE80211_CHAN_NO_HT40PLUS | IEEE80211_CHAN_NO_HT40MINUS)

/**
 * struct ieee80211_channel - channel definition
 *
 * This structure describes a single channel for use
 * with cfg80211.
 *
 * @center_freq: center frequency in MHz
 * @hw_value: hardware-specific value for the channel
 * @flags: channel flags from &enum ieee80211_channel_flags.
 * @orig_flags: channel flags at registration time, used by regulatory
 *	code to support devices with additional restrictions
 * @band: band this channel belongs to.
 * @max_antenna_gain: maximum antenna gain in dBi
 * @max_power: maximum transmission power (in dBm)
 * @beacon_found: helper to regulatory code to indicate when a beacon
 *	has been found on this channel. This is useful only on 5 GHz band.
 * @orig_mag: internal use
 * @orig_mpwr: internal use
 */
struct ieee80211_channel {
	enum ieee80211_band band;
	uint16_t center_freq;
	uint16_t hw_value;
	uint32_t flags;
	int max_antenna_gain;
	int max_power;
	bool beacon_found;
	uint32_t orig_flags;
	int orig_mag, orig_mpwr;
};

/**
 * struct ieee80211_supported_band - frequency band definition
 *
 * This structure describes a frequency band a wiphy
 * is able to operate in.
 *
 * @channels: Array of channels the hardware can operate in
 *	in this band.
 * @band: the band this structure represents
 * @n_channels: Number of channels in @channels
 */
struct ieee80211_supported_band {
        struct ieee80211_channel *channels;
        enum ieee80211_band band;
        int n_channels;
};

struct device;

struct dev_ops {
	int (*probe)(struct device *dev, unsigned int idx);
	void (*remove)(struct device *dev, unsigned int idx);
};

struct wifi_dev {
	struct device *dev;
	unsigned int idx;
	struct ieee80211_supported_band *bands[IEEE80211_NUM_BANDS];
};

struct device {
	bool registered;
	struct dev_ops *ops;
	struct wifi_dev *wdev;
};
#endif /* __WIFI_DEV_H */
