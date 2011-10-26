#ifndef __WIFI_DEV_H
#define __WIFI_DEV_H

#include <stdint.h>
#include <stdbool.h>

#include "regulatory.h"

/*
 * Device template for an 802.11 subsystem.
 */

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

void regulatory_update(struct wifi_dev *dev, enum ieee80211_reg_initiator);

#endif /* __WIFI_DEV_H */
