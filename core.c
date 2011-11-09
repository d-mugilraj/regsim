#include <string.h>
#include <stdio.h>

#include "reg.h"
#include "core.h"

extern struct device acme;

struct device *devices[] = {
	&acme,
};

static void remove_wifi_devices(void)
{
	unsigned int i;
	struct device *dev = NULL;

	for (i=0; i < ARRAY_SIZE(devices); i++) {
		dev = devices[i];
		if (!dev)
			break;
		if (!dev->registered)
			continue;
		dev->ops->remove(dev, i);
	}
}

static int probe_wifi_devices(void)
{
	unsigned int i;
	int r;
	struct device *dev = NULL;

	for (i=0; i < ARRAY_SIZE(devices); i++) {
		dev = devices[i];
		if (!dev)
			break;
		r = dev->ops->probe(dev, i);
		if (r)
			goto fail;
		dev->registered = true;
	}
	return 0;
fail:
	remove_wifi_devices();
	return r;
}

struct wifi_dev *wdev_new(void)
{
	struct wifi_dev *wdev;

	wdev = malloc(sizeof(struct wifi_dev));
	if (!wdev)
		return NULL;
	memset(wdev, 0, sizeof(struct wifi_dev));
	return wdev;
}

void wdev_free(struct wifi_dev *wdev)
{
	free(wdev);
}

void register_wifi_dev(struct wifi_dev *wdev)
{
	regdev_update(&wdev->reg);
	printf("wlan%d registered\n", wdev->idx);
}

int main(void)
{
	int r;

	r = regulatory_init();
	if (r)
		return r;

	reg_core_test();

	r = probe_wifi_devices();
	if (r)
		return r;

	remove_wifi_devices();

	return 0;
}
