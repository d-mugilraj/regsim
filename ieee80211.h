#ifndef __IEEE80211_H
#define __IEEE80211_H

/**
 * enum ieee80211_initiator - Indicates the initiator of a reg domain request
 * @IEEE80211_REGDOM_SET_BY_CORE: Core queried CRDA for a dynamic world
 * 	regulatory domain.
 * @IEEE80211_REGDOM_SET_BY_USER: User asked the wireless core to set the
 * 	regulatory domain.
 * @IEEE80211_REGDOM_SET_BY_DRIVER: a wireless drivers has hinted to the
 * 	wireless core it thinks its knows the regulatory domain we should be in.
 * @IEEE80211_REGDOM_SET_BY_COUNTRY_IE: the wireless core has received an
 * 	802.11 country information element with regulatory information it
 * 	thinks we should consider. We only processes the country
 *	code from the IE, and rely on the regulatory domain information
 *	structure passed by userspace (CRDA) from our wireless-regdb.
 *	If a channel is enabled but the country code indicates it should
 *	be disabled we disable the channel and re-enable it upon disassociation.
 */
enum ieee80211_reg_initiator {
	IEEE80211_REGDOM_SET_BY_CORE,
	IEEE80211_REGDOM_SET_BY_USER,
	IEEE80211_REGDOM_SET_BY_DRIVER,
	IEEE80211_REGDOM_SET_BY_COUNTRY_IE,
};

/**
 * enum ieee80211_reg_type - specifies the type of regulatory domain
 * @IEEE80211_REGDOM_TYPE_COUNTRY: the regulatory domain set is one that pertains
 *	to a specific country. When this is set you can count on the
 *	ISO / IEC 3166 alpha2 country code being valid.
 * @IEEE80211_REGDOM_TYPE_WORLD: the regulatory set domain is the world regulatory
 * 	domain.
 * @IEEE80211_REGDOM_TYPE_CUSTOM_WORLD: the regulatory domain set is a custom
 * 	driver specific world regulatory domain. These do not apply system-wide
 * 	and are only applicable to the individual devices which have requested
 * 	them to be applied.
 * @IEEE80211_REGDOM_TYPE_INTERSECTION: the regulatory domain set is the product
 *	of an intersection between two regulatory domains -- the previously
 *	set regulatory domain on the system and the last accepted regulatory
 *	domain request to be processed.
 */
enum ieee80211_reg_type {
	IEEE80211_REGDOM_TYPE_COUNTRY,
	IEEE80211_REGDOM_TYPE_WORLD,
	IEEE80211_REGDOM_TYPE_CUSTOM_WORLD,
	IEEE80211_REGDOM_TYPE_INTERSECTION,
};

/**
 * enum ieee80211_reg_rule_flags - regulatory rule flags
 *
 * @IEEE80211_RRF_NO_OFDM: OFDM modulation not allowed
 * @IEEE80211_RRF_NO_CCK: CCK modulation not allowed
 * @IEEE80211_RRF_NO_INDOOR: indoor operation not allowed
 * @IEEE80211_RRF_NO_OUTDOOR: outdoor operation not allowed
 * @IEEE80211_RRF_DFS: DFS support is required to be used
 * @IEEE80211_RRF_PTP_ONLY: this is only for Point To Point links
 * @IEEE80211_RRF_PTMP_ONLY: this is only for Point To Multi Point links
 * @IEEE80211_RRF_PASSIVE_SCAN: passive scan is required
 * @IEEE80211_RRF_NO_IR: initiating radiation on our own (beaconing, or
 *	probe requests) is not allowed. The reason this flag exists is although
 *	some OEMs / ODMs may be aware that a mode of operation that initiates
 *	radiation such as AP or Adhoc is allowed in a channnel they simply do
 *	not want to support it.
 */
enum ieee80211_reg_rule_flags {
	IEEE80211_RRF_NO_OFDM		= 1<<0,
	IEEE80211_RRF_NO_CCK		= 1<<1,
	IEEE80211_RRF_NO_INDOOR		= 1<<2,
	IEEE80211_RRF_NO_OUTDOOR	= 1<<3,
	IEEE80211_RRF_DFS		= 1<<4,
	IEEE80211_RRF_PTP_ONLY		= 1<<5,
	IEEE80211_RRF_PTMP_ONLY		= 1<<6,
	IEEE80211_RRF_PASSIVE_SCAN	= 1<<7,
	IEEE80211_RRF_NO_IR		= 1<<8,
};

#endif /* __IEEE80211_H */
