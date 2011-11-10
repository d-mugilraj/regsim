#ifndef __NET_REG_H
#define __NET_REG_H

#include "reglib.h"

void reg_core_test(void);
int regulatory_init(void);
void regulatory_exit(void);
void regdev_update(struct ieee80211_dev_regulatory *reg);

#endif /* __NET_REG_H */
