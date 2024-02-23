/******************************************************************************
 *
 * Copyright(c) 2017 - 2019 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 ******************************************************************************/

#ifndef _HALMAC_8822E_CFG_H_
#define _HALMAC_8822E_CFG_H_

#include "../../halmac_hw_cfg.h"
#include "../halmac_88xx_cfg.h"

#if HALMAC_8822E_SUPPORT

#define TX_FIFO_SIZE_8822E	262144
#define RX_FIFO_SIZE_8822E	24576
#define TRX_SHARE_SIZE0_8822E	40960
#define TRX_SHARE_SIZE1_8822E	24576
#define TRX_SHARE_SIZE2_8822E	(TRX_SHARE_SIZE0_8822E + TRX_SHARE_SIZE1_8822E)

#define TX_FIFO_SIZE_LA_8822E	(TX_FIFO_SIZE_8822E >>  1)
#define TX_FIFO_SIZE_RX_EXPAND_1BLK_8822E	\
	(TX_FIFO_SIZE_8822E - TRX_SHARE_SIZE0_8822E)
#define RX_FIFO_SIZE_RX_EXPAND_1BLK_8822E	\
	(RX_FIFO_SIZE_8822E + TRX_SHARE_SIZE0_8822E)
#define TX_FIFO_SIZE_RX_EXPAND_2BLK_8822E	\
	(TX_FIFO_SIZE_8822E - TRX_SHARE_SIZE2_8822E)
#define RX_FIFO_SIZE_RX_EXPAND_2BLK_8822E	\
	(RX_FIFO_SIZE_8822E + TRX_SHARE_SIZE2_8822E)
#define TX_FIFO_SIZE_RX_EXPAND_3BLK_8822E	\
	(TX_FIFO_SIZE_8822E - TRX_SHARE_SIZE2_8822E - TRX_SHARE_SIZE0_8822E)
#define RX_FIFO_SIZE_RX_EXPAND_3BLK_8822E	\
	(RX_FIFO_SIZE_8822E + TRX_SHARE_SIZE2_8822E + TRX_SHARE_SIZE0_8822E)
#define TX_FIFO_SIZE_RX_EXPAND_4BLK_8822E	\
	(TX_FIFO_SIZE_8822E - (2 * TRX_SHARE_SIZE2_8822E))
#define RX_FIFO_SIZE_RX_EXPAND_4BLK_8822E	\
	(RX_FIFO_SIZE_8822E + (2 * TRX_SHARE_SIZE2_8822E))

#define SEC_DATA_EFUSE_SIZE_8822E 192
#define WLAN_HIDDEN_EFUSE_SIZE_8822E 128

#define EFUSE_SIZE_8822E	1216
#define EEPROM_SIZE_8822E	2048
#define BT_EFUSE_SIZE_8822E	512
#define BT_LOG_EFUSE_SIZE_8822E 1024
#define PRTCT_EFUSE_SIZE_8822E	(SEC_DATA_EFUSE_SIZE_8822E + \
	WLAN_HIDDEN_EFUSE_SIZE_8822E + BT_EFUSE_SIZE_8822E)

#define SEC_CAM_NUM_8822E	64

#define OQT_ENTRY_AC_8822E	32
#define OQT_ENTRY_NOAC_8822E	32
#define MACID_MAX_8822E		128

#define WLAN_FW_IRAM_MAX_SIZE_8822E	65536
#define WLAN_FW_DRAM_MAX_SIZE_8822E	65536
#define WLAN_FW_ERAM_MAX_SIZE_8822E	131072
#define WLAN_FW_MAX_SIZE_8822E		(WLAN_FW_IRAM_MAX_SIZE_8822E + \
	WLAN_FW_DRAM_MAX_SIZE_8822E + WLAN_FW_ERAM_MAX_SIZE_8822E)

#endif /* HALMAC_8822E_SUPPORT*/

#endif
