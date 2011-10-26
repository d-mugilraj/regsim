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
	struct ieee80211_dev_regulatory reg;
};

struct device {
	bool registered;
	struct dev_ops *ops;
	struct wifi_dev *wdev;
};

#endif /* __WIFI_DEV_H */
