#ifndef __CORE_H
#define __CORE_H

#include "wifi-dev.h"
#include "regulatory.h"

int probe_wifi_devices(void);
void remove_wifi_devices(void);

struct wifi_dev *wdev_new(void);
void wdev_free(struct wifi_dev *wdev);

void register_wifi_dev(struct wifi_dev *wdev);

#endif /* __CORE_H */
