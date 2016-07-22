#ifndef _UWIFI_PHY_INFO_H_
#define _UWIFI_PHY_INFO_H_

#include <stdbool.h>

#define PHY_FLAG_SHORTPRE	BIT(0)
#define PHY_FLAG_BADFCS		BIT(1)
#define PHY_FLAG_A		BIT(2)
#define PHY_FLAG_B		BIT(3)
#define PHY_FLAG_G		BIT(4)
#define PHY_FLAG_MODE_MASK	BIT(5)

struct phy_info {
	bool			phy_badfcs;
	bool			phy_injected;
	int			phy_freq;
	int			phy_signal;
	unsigned int		phy_rate;		/* physical rate * 10 (=in 100kbps) */
	unsigned char		phy_rate_idx;	/* MCS index */
	unsigned char		phy_rate_flags;	/* MCS flags */
	unsigned int		phy_flags;		/* A, B, G, shortpre */
};

#endif
