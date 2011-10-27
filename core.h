#ifndef __CORE_H
#define __CORE_H

#include "wifi-dev.h"
#include "reglib.h"

struct wifi_dev *wdev_new(void);
void wdev_free(struct wifi_dev *wdev);

void register_wifi_dev(struct wifi_dev *wdev);

#endif /* __CORE_H */
