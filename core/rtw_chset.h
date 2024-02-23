/******************************************************************************
 *
 * Copyright(c) 2007 - 2022 Realtek Corporation.
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
 *****************************************************************************/
#ifndef __RTW_CHSET_H__
#define __RTW_CHSET_H__

enum {
	RTW_CHF_DIS = BIT0,
	RTW_CHF_NO_IR = BIT1,
	RTW_CHF_DFS = BIT2,
	RTW_CHF_NO_HT40U = BIT3,
	RTW_CHF_NO_HT40L = BIT4,
	RTW_CHF_NO_80MHZ = BIT5,
	RTW_CHF_NO_160MHZ = BIT6,
};

#define RTW_CHF_FMT "%s%s%s%s%s%s%s"

#define RTW_CHF_ARG_DIS(flags)			(flags & RTW_CHF_DIS) ? " DIS" : ""
#define RTW_CHF_ARG_NO_IR(flags)		(flags & RTW_CHF_NO_IR) ? " NO_IR" : ""
#define RTW_CHF_ARG_DFS(flags)			(flags & RTW_CHF_DFS) ? " DFS" : ""
#define RTW_CHF_ARG_NO_HT40U(flags)		(flags & RTW_CHF_NO_HT40U) ? " NO_40M+" : ""
#define RTW_CHF_ARG_NO_HT40L(flags)		(flags & RTW_CHF_NO_HT40L) ? " NO_40M-" : ""
#define RTW_CHF_ARG_NO_80MHZ(flags)		(flags & RTW_CHF_NO_80MHZ) ? " NO_80M" : ""
#define RTW_CHF_ARG_NO_160MHZ(flags)	(flags & RTW_CHF_NO_160MHZ) ? " NO_160M" : ""

#define RTW_CHF_ARG(flags) \
	RTW_CHF_ARG_DIS(flags) \
	, RTW_CHF_ARG_NO_IR(flags) \
	, RTW_CHF_ARG_DFS(flags) \
	, RTW_CHF_ARG_NO_HT40U(flags) \
	, RTW_CHF_ARG_NO_HT40L(flags) \
	, RTW_CHF_ARG_NO_80MHZ(flags) \
	, RTW_CHF_ARG_NO_160MHZ(flags)

/* The channel information about this channel including joining, scanning, and power constraints. */
typedef struct _RT_CHANNEL_INFO {
	u8 band; /* enum band_type */
	u8 ChannelNum; /* The channel number. */

	/*
	* Bitmap and its usage:
	* RTW_CHF_DIS, RTW_CHF_NO_IR, RTW_CHF_DFS: is used to check for status
	* RTW_CHF_NO_HT40U, RTW_CHF_NO_HT40L, RTW_CHF_NO_80MHZ, RTW_CHF_NO_160MHZ: extra bandwidth limitation (ex: from regulatory)
	*/
	u8 flags;

#ifdef CONFIG_FIND_BEST_CHANNEL
	u32 rx_count;
#endif

#if CONFIG_IEEE80211_BAND_5GHZ && CONFIG_DFS
	#ifdef CONFIG_DFS_MASTER
	systime non_ocp_end_time;
	#endif
#endif

	u8 hidden_bss_cnt; /* per scan count */

#ifdef CONFIG_IOCTL_CFG80211
	void *os_chan;
#endif
} RT_CHANNEL_INFO, *PRT_CHANNEL_INFO;

struct rtw_chset {
	RT_CHANNEL_INFO chs[MAX_CHANNEL_NUM];
	u8 chs_len;
	RT_CHANNEL_INFO *chs_of_band[BAND_MAX];
	u8 chs_len_of_band[BAND_MAX];
	u8 enable_ch_num;
};

int rtw_chset_init(struct rtw_chset *chset, u8 band_bmp);

RTW_FUNC_2G_5G_ONLY int rtw_chset_search_ch(const struct rtw_chset *chset, u32 ch);
RTW_FUNC_2G_5G_ONLY u8 rtw_chset_is_chbw_valid(const struct rtw_chset *chset, u8 ch, u8 bw, u8 offset
	, bool allow_primary_passive, bool allow_passive);
RTW_FUNC_2G_5G_ONLY void rtw_chset_sync_chbw(const struct rtw_chset *chset, u8 *req_ch, u8 *req_bw, u8 *req_offset
	, u8 *g_ch, u8 *g_bw, u8 *g_offset, bool allow_primary_passive, bool allow_passive);
int rtw_chset_search_bch(const struct rtw_chset *chset, enum band_type band, u32 ch);
RT_CHANNEL_INFO *rtw_chset_get_chinfo_by_bch(struct rtw_chset *chset, enum band_type band, u32 ch, bool include_dis);
u8 rtw_chset_is_bchbw_valid(const struct rtw_chset *chset, enum band_type band, u8 ch, u8 bw, u8 offset
	, bool allow_primary_passive, bool allow_passive);
void rtw_chset_sync_bchbw(const struct rtw_chset *chset, enum band_type *req_band, u8 *req_ch, u8 *req_bw, u8 *req_offset
	, enum band_type *g_band, u8 *g_ch, u8 *g_bw, u8 *g_offset, bool allow_primary_passive, bool allow_passive);

u8 *rtw_chset_set_spt_chs_ie(struct rtw_chset *chset, u8 *buf_pos, uint *buf_len);

#ifdef CONFIG_PROC_DEBUG
void dump_chinfos(void *sel, const RT_CHANNEL_INFO *chinfos, u8 chinfo_num);
#endif

#endif /* __RTW_CHSET_H__ */
