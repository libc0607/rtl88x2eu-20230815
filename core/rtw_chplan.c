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
#define _RTW_CHPLAN_C_

#include <drv_types.h>

extern struct rtw_regdb_ops regdb_ops;

u8 rtw_chplan_get_default_regd_2g(u8 id)
{
	if (regdb_ops.get_default_regd_2g)
		return regdb_ops.get_default_regd_2g(id);
	return RTW_REGD_NA;
}

#if CONFIG_IEEE80211_BAND_5GHZ
u8 rtw_chplan_get_default_regd_5g(u8 id)
{
	if (regdb_ops.get_default_regd_5g)
		return regdb_ops.get_default_regd_5g(id);
	return RTW_REGD_NA;
}
#endif

bool rtw_is_channel_plan_valid(u8 id)
{
	if (regdb_ops.is_domain_code_valid)
		return regdb_ops.is_domain_code_valid(id);
	return false;
}

bool rtw_regsty_is_excl_chs(struct registry_priv *regsty, u8 ch)
{
	int i;

	for (i = 0; i < MAX_CHANNEL_NUM_2G_5G; i++) {
		if (regsty->excl_chs[i] == 0)
			break;
		if (regsty->excl_chs[i] == ch)
			return _TRUE;
	}
	return _FALSE;
}

/*
 * Search the @param ch in chplan by given @param id
 * @id: the given channel plan id
 * @ch: the given channel number
 *
 * return the index of channel_num in channel_set, -1 if not found
 */
static bool rtw_chplan_get_ch(u8 id, u32 ch, u8 *flags)
{
	if (regdb_ops.domain_get_ch)
		return regdb_ops.domain_get_ch(id, ch, flags);
	return false;
}

#if CONFIG_IEEE80211_BAND_6GHZ
u8 rtw_chplan_get_default_regd_6g(u8 id)
{
	if (regdb_ops.get_default_regd_6g)
		return regdb_ops.get_default_regd_6g(id);
	return RTW_REGD_NA;
}

bool rtw_is_channel_plan_6g_valid(u8 id)
{
	if (regdb_ops.is_domain_code_6g_valid)
		return regdb_ops.is_domain_code_6g_valid(id);
	return false;
}

bool rtw_regsty_is_excl_chs_6g(struct registry_priv *regsty, u8 ch)
{
	int i;

	for (i = 0; i < MAX_CHANNEL_NUM_6G; i++) {
		if (regsty->excl_chs_6g[i] == 0)
			break;
		if (regsty->excl_chs_6g[i] == ch)
			return _TRUE;
	}
	return _FALSE;
}

/*
 * Search the @param ch in chplan by given @param id
 * @id: the given channel plan id
 * @ch: the given channel number
 *
 * return the index of channel_num in channel_set, -1 if not found
 */
static bool rtw_chplan_6g_get_ch(u8 id, u32 ch, u8 *flags)
{
	if (regdb_ops.domain_6g_get_ch)
		return regdb_ops.domain_6g_get_ch(id, ch, flags);
	return false;
}
#endif /* CONFIG_IEEE80211_BAND_6GHZ */

/*
 * Check if the @param ch, bw, offset is valid for the given @param id, id_6g
 * @ch_set: the given channel set
 * @ch: the given channel number
 * @bw: the given bandwidth
 * @offset: the given channel offset
 *
 * return valid (1) or not (0)
 */
u8 rtw_chplan_is_bchbw_valid(u8 id, u8 id_6g, enum band_type band, u8 ch, u8 bw, u8 offset
	, bool allow_primary_passive, bool allow_passive, struct registry_priv *regsty)
{
	u8 cch;
	u8 *op_chs;
	u8 op_ch_num;
	u8 op_ch;
	u8 valid = 0;
	int i;
	int ch_idx;
	u8 flags;

	cch = rtw_get_center_ch_by_band(band, ch, bw, offset);

	if (!rtw_get_op_chs_by_bcch_bw(band, cch, bw, &op_chs, &op_ch_num))
		goto exit;

	for (i = 0; i < op_ch_num; i++) {
		op_ch = *(op_chs + i);
		if (0)
			RTW_INFO("%u,%u,%u - cch:%u, bw:%u, op_ch:%u\n", ch, bw, offset, cch, bw, op_ch);
		#if CONFIG_IEEE80211_BAND_6GHZ
		if (band == BAND_ON_6G) {
			if (!rtw_chplan_6g_get_ch(id_6g, op_ch, &flags)
				|| (regsty && rtw_regsty_is_excl_chs_6g(regsty, op_ch) == _TRUE))
				break;
		} else
		#endif
		{
			if (!rtw_chplan_get_ch(id, op_ch, &flags)
				|| (regsty && rtw_regsty_is_excl_chs(regsty, op_ch) == _TRUE))
				break;
		}
		if (flags & RTW_CHF_NO_IR) {
			if (!allow_passive
				|| (!allow_primary_passive && op_ch == ch))
				break;
		}
	}

	if (op_ch_num != 0 && i == op_ch_num)
		valid = 1;

exit:
	return valid;
}

const char *_regd_src_str[] = {
	[REGD_SRC_RTK_PRIV] = "RTK_PRIV",
	[REGD_SRC_OS] = "OS",
	[REGD_SRC_NUM] = "UNKNOWN",
};

static void rtw_chset_apply_from_rtk_priv(struct rtw_chset *chset, u8 chplan, u8 chplan_6g)
{
	RT_CHANNEL_INFO *chinfo;
	u8 i;
	u8 flags;
	bool chplan_valid = rtw_is_channel_plan_valid(chplan);
#if CONFIG_IEEE80211_BAND_6GHZ
	bool chplan_6g_valid = rtw_is_channel_plan_valid(chplan_6g);
#endif
	bool usable_ch;

	RTW_INFO("%s chplan:0x%02X chplan_6g:0x%02X\n", __func__, chplan, chplan_6g);
	rtw_warn_on(!chplan_valid);
#if CONFIG_IEEE80211_BAND_6GHZ
	rtw_warn_on(!chplan_6g_valid);
#endif

	for (i = 0; i < chset->chs_len; i++) {
		chinfo = &chset->chs[i];
		if (chinfo->flags & RTW_CHF_DIS)
			continue;

		if (chinfo->band == BAND_ON_24G
			#if CONFIG_IEEE80211_BAND_5GHZ
			|| chinfo->band == BAND_ON_5G
			#endif
		) {
			if (!chplan_valid)
				continue;
			usable_ch = rtw_chplan_get_ch(chplan, chinfo->ChannelNum, &flags);
		}
		#if CONFIG_IEEE80211_BAND_6GHZ
		else if (chinfo->band == BAND_ON_6G) {
			if (!chplan_6g_valid)
				continue;
			usable_ch = rtw_chplan_6g_get_ch(chplan_6g, chinfo->ChannelNum, &flags);
		}
		#endif
		else
			usable_ch = false;

		if (usable_ch)
			chinfo->flags |= flags;
		else
			chinfo->flags = RTW_CHF_DIS;
	}
}

static void rtw_rfctl_chset_apply_regd_reqs(struct rf_ctl_t *rfctl)
{
	if (rfctl->regd_src == REGD_SRC_RTK_PRIV) {
		u8 domain_code = rfctl->ChannelPlan;
		u8 domain_code_6g = RTW_CHPLAN_6G_NULL;

		#if CONFIG_IEEE80211_BAND_6GHZ
		domain_code_6g = rfctl->chplan_6g;
		#endif
		rtw_chset_apply_from_rtk_priv(&rfctl->chset, domain_code, domain_code_6g);
	}
	#ifdef CONFIG_REGD_SRC_FROM_OS
	else if (rfctl->regd_src == REGD_SRC_OS)
		rtw_chset_apply_from_os(&rfctl->chset);
	#endif
	else
		rtw_warn_on(1);
}

void rtw_rfctl_chset_apply_regulatory(_adapter *adapter)
{
	struct registry_priv *regsty = adapter_to_regsty(adapter);
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);
	struct rtw_chset *chset = &rfctl->chset;
	RT_CHANNEL_INFO *chinfo;
	int i;

	/* reset flags of all channels */
	for (i = 0; i < chset->chs_len; i++) {
		chinfo = &chset->chs[i];
		if (((chinfo->band == BAND_ON_24G || chinfo->band == BAND_ON_5G)
				&& rtw_regsty_is_excl_chs(regsty, chinfo->ChannelNum) == true)
			#if CONFIG_IEEE80211_BAND_6GHZ
			|| (chinfo->band == BAND_ON_6G
				&& rtw_regsty_is_excl_chs_6g(regsty, chinfo->ChannelNum) == true)
			#endif
		)
			chinfo->flags = RTW_CHF_DIS;
		else
			chinfo->flags = 0;
	}

	rtw_rfctl_chset_apply_regd_reqs(rfctl);

	chset->enable_ch_num = 0;
	for (i = 0; i < chset->chs_len; i++) {
		chinfo = &chset->chs[i];

#ifdef CONFIG_DFS_MASTER
		if (!CH_IS_NON_OCP_STOPPED(chinfo)) {
			/* make it stop right at next watchdog checking */
			chinfo->non_ocp_end_time = rtw_get_current_time();
			if (chinfo->non_ocp_end_time == RTW_NON_OCP_STOPPED)
				chinfo->non_ocp_end_time++;
		}
#endif

		if (chinfo->flags & RTW_CHF_DIS)
			continue;
		chset->enable_ch_num++;

		/* logs for channel with NO_IR but can't be cleared through beacon hint */
		if (chinfo->flags & RTW_CHF_NO_IR) {
			if (!rtw_rfctl_reg_allow_beacon_hint(rfctl) || !rtw_chinfo_allow_beacon_hint(chinfo))
				RTW_INFO("band:%s ch%u is NO_IR%s while beacon hint not allowed\n"
					, band_str(chinfo->band), chinfo->ChannelNum, chinfo->flags & RTW_CHF_DFS ? " DFS" : "");
		}
	}

	if (chset->enable_ch_num)
		RTW_INFO(FUNC_ADPT_FMT" ch num:%d\n", FUNC_ADPT_ARG(adapter), chset->enable_ch_num);
	else
		RTW_WARN(FUNC_ADPT_FMT" final chset has no channel\n", FUNC_ADPT_ARG(adapter));
}

/* domain status specific beacon hint rules */
#ifndef RTW_CHPLAN_BEACON_HINT_SPECIFIC_COUNTRY
#define RTW_CHPLAN_BEACON_HINT_SPECIFIC_COUNTRY 0
#endif

bool rtw_rfctl_reg_allow_beacon_hint(struct rf_ctl_t *rfctl)
{
	return RTW_CHPLAN_BEACON_HINT_SPECIFIC_COUNTRY || RFCTL_REG_WORLDWIDE(rfctl) || RFCTL_REG_ALPHA2_UNSPEC(rfctl);
}

/* channel specific beacon hint rules */
#ifndef RTW_CHPLAN_BEACON_HINT_ON_2G_CH_1_11
#define RTW_CHPLAN_BEACON_HINT_ON_2G_CH_1_11 0
#endif
#ifndef RTW_CHPLAN_BEACON_HINT_ON_DFS_CH
#define RTW_CHPLAN_BEACON_HINT_ON_DFS_CH 0
#endif

bool rtw_chinfo_allow_beacon_hint(struct _RT_CHANNEL_INFO *chinfo)
{
	return (RTW_CHPLAN_BEACON_HINT_ON_2G_CH_1_11 || !(chinfo->band == BAND_ON_24G && chinfo->ChannelNum <= 11))
		&& (RTW_CHPLAN_BEACON_HINT_ON_DFS_CH || !(chinfo->flags & RTW_CHF_DFS));
}

u8 rtw_process_beacon_hint(struct rf_ctl_t *rfctl, WLAN_BSSID_EX *bss)
{
	struct rtw_chset *chset = &rfctl->chset;
	enum band_type band = BSS_EX_OP_BAND(bss);
	u8 ch = BSS_EX_OP_CH(bss);
	RT_CHANNEL_INFO *chinfo = rtw_chset_get_chinfo_by_bch(chset, band, ch, false);
	u8 act_cnt = 0;

	if (!chinfo)
		goto exit;

	if ((chinfo->flags & RTW_CHF_NO_IR)
		&& rtw_rfctl_reg_allow_beacon_hint(rfctl)
		&& rtw_chinfo_allow_beacon_hint(chinfo)
	) {
		RTW_INFO("%s: change band:%s ch:%d to active\n", __func__, band_str(band), ch);
		chinfo->flags &= ~RTW_CHF_NO_IR;
		act_cnt++;
	}

exit:
	return act_cnt;
}

const char *const _regd_inr_str[] = {
	[RTW_REGD_SET_BY_INIT]			= "INIT",
	[RTW_REGD_SET_BY_USER]			= "USER",
	[RTW_REGD_SET_BY_COUNTRY_IE]	= "COUNTRY_IE",
	[RTW_REGD_SET_BY_DRIVER]		= "DRIVER",
	[RTW_REGD_SET_BY_CORE]			= "CORE",
	[RTW_REGD_SET_BY_NUM]			= "UNKNOWN",
};

const char *const _regd_str[] = {
	[RTW_REGD_NA]		= "NA",
	[RTW_REGD_FCC]		= "FCC",
	[RTW_REGD_MKK]		= "MKK",
	[RTW_REGD_ETSI]		= "ETSI",
	[RTW_REGD_IC]		= "IC",
	[RTW_REGD_KCC]		= "KCC",
	[RTW_REGD_NCC]		= "NCC",
	[RTW_REGD_ACMA]		= "ACMA",
	[RTW_REGD_CHILE]	= "CHILE",
	[RTW_REGD_MEX]		= "MEX",
	[RTW_REGD_WW]		= "WW",
};

const char *const _rtw_edcca_mode_str[] = {
	[RTW_EDCCA_NORM]	= "NORMAL",
	[RTW_EDCCA_ADAPT]	= "ADAPT",
	[RTW_EDCCA_CS]		= "CS",
};

const char *const _rtw_dfs_regd_str[] = {
	[RTW_DFS_REGD_NONE]	= "NONE",
	[RTW_DFS_REGD_FCC]	= "FCC",
	[RTW_DFS_REGD_MKK]	= "MKK",
	[RTW_DFS_REGD_ETSI]	= "ETSI",
	[RTW_DFS_REGD_KCC]	= "KCC",
};

const char *const _txpwr_lmt_str[] = {
	[TXPWR_LMT_NONE]	= "NONE",
	[TXPWR_LMT_FCC]		= "FCC",
	[TXPWR_LMT_MKK]		= "MKK",
	[TXPWR_LMT_ETSI]	= "ETSI",
	[TXPWR_LMT_IC]		= "IC",
	[TXPWR_LMT_KCC]		= "KCC",
	[TXPWR_LMT_NCC]		= "NCC",
	[TXPWR_LMT_ACMA]	= "ACMA",
	[TXPWR_LMT_CHILE]	= "CHILE",
	[TXPWR_LMT_UKRAINE]	= "UKRAINE",
	[TXPWR_LMT_MEXICO]	= "MEXICO",
	[TXPWR_LMT_CN]		= "CN",
	[TXPWR_LMT_QATAR]	= "QATAR",
	[TXPWR_LMT_UK]		= "UK",
	[TXPWR_LMT_WW]		= "WW",
	[TXPWR_LMT_NUM]		= NULL,
};

const REGULATION_TXPWR_LMT _txpwr_lmt_alternate[] = {
	[TXPWR_LMT_NONE]	= TXPWR_LMT_NONE,
	[TXPWR_LMT_FCC]		= TXPWR_LMT_FCC,
	[TXPWR_LMT_MKK]		= TXPWR_LMT_MKK,
	[TXPWR_LMT_ETSI]	= TXPWR_LMT_ETSI,
	[TXPWR_LMT_WW]		= TXPWR_LMT_WW,
	[TXPWR_LMT_NUM]		= TXPWR_LMT_NUM,

	[TXPWR_LMT_IC]		= TXPWR_LMT_FCC,
	[TXPWR_LMT_KCC]		= TXPWR_LMT_ETSI,
	[TXPWR_LMT_NCC]		= TXPWR_LMT_FCC,
	[TXPWR_LMT_ACMA]	= TXPWR_LMT_ETSI,
	[TXPWR_LMT_CHILE]	= TXPWR_LMT_FCC,
	[TXPWR_LMT_UKRAINE]	= TXPWR_LMT_ETSI,
	[TXPWR_LMT_MEXICO]	= TXPWR_LMT_FCC,
	[TXPWR_LMT_CN]		= TXPWR_LMT_ETSI,
	[TXPWR_LMT_QATAR]	= TXPWR_LMT_ETSI,
	[TXPWR_LMT_UK]		= TXPWR_LMT_ETSI,
};

const enum rtw_edcca_mode _rtw_regd_to_edcca_mode[RTW_REGD_NUM] = {
	[RTW_REGD_NA] = RTW_EDCCA_MODE_NUM,
	[RTW_REGD_MKK] = RTW_EDCCA_CS,
	[RTW_REGD_ETSI] = RTW_EDCCA_ADAPT,
	[RTW_REGD_WW] = RTW_EDCCA_ADAPT,
};

const REGULATION_TXPWR_LMT _rtw_regd_to_txpwr_lmt[] = {
	[RTW_REGD_NA]		= TXPWR_LMT_NUM,
	[RTW_REGD_FCC]		= TXPWR_LMT_FCC,
	[RTW_REGD_MKK]		= TXPWR_LMT_MKK,
	[RTW_REGD_ETSI]		= TXPWR_LMT_ETSI,
	[RTW_REGD_IC]		= TXPWR_LMT_IC,
	[RTW_REGD_KCC]		= TXPWR_LMT_KCC,
	[RTW_REGD_NCC]		= TXPWR_LMT_NCC,
	[RTW_REGD_ACMA]		= TXPWR_LMT_ACMA,
	[RTW_REGD_CHILE]	= TXPWR_LMT_CHILE,
	[RTW_REGD_MEX]		= TXPWR_LMT_MEXICO,
	[RTW_REGD_WW]		= TXPWR_LMT_WW,
};

char *rtw_get_edcca_modes_str(char *buf, u8 modes[])
{
#define EDCCA_MODE_SEQ_COMPARE(result, operand) (result == RTW_EDCCA_MODE_NUM ? operand : (operand == RTW_EDCCA_MODE_NUM ? result : (result != operand ? -1 : result)))

	int mode = RTW_EDCCA_MODE_NUM;
	int cnt = 0;

	mode = EDCCA_MODE_SEQ_COMPARE(mode, modes[BAND_ON_24G]);
#if CONFIG_IEEE80211_BAND_5GHZ
	mode = EDCCA_MODE_SEQ_COMPARE(mode, modes[BAND_ON_5G]);
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	mode = EDCCA_MODE_SEQ_COMPARE(mode, modes[BAND_ON_6G]);
#endif

	if (mode != -1) { /* all available values are the same */
		cnt += snprintf(buf + cnt, EDCCA_MODES_STR_LEN - cnt - 1, "%s(%u)", rtw_edcca_mode_str(mode), mode);
		if (cnt >= EDCCA_MODES_STR_LEN - 1)
			goto exit;
	} else {
		cnt += snprintf(buf + cnt, EDCCA_MODES_STR_LEN - cnt - 1, "%s(%u) ", rtw_edcca_mode_str(modes[BAND_ON_24G]), modes[BAND_ON_24G]);
		if (cnt >= EDCCA_MODES_STR_LEN - 1)
			goto exit;
		#if CONFIG_IEEE80211_BAND_5GHZ
		cnt += snprintf(buf + cnt, EDCCA_MODES_STR_LEN - cnt - 1, "%s(%u) ", rtw_edcca_mode_str(modes[BAND_ON_5G]), modes[BAND_ON_5G]);
		if (cnt >= EDCCA_MODES_STR_LEN - 1)
			goto exit;
		#endif
		#if CONFIG_IEEE80211_BAND_6GHZ
		cnt += snprintf(buf + cnt, EDCCA_MODES_STR_LEN - cnt - 1, "%s(%u) ", rtw_edcca_mode_str(modes[BAND_ON_6G]), modes[BAND_ON_6G]);
		if (cnt >= EDCCA_MODES_STR_LEN - 1)
			goto exit;
		#endif
		buf[cnt - 1] = 0;
	}

exit:
	return buf;
}

void rtw_edcca_mode_update(struct dvobj_priv *dvobj)
{
	struct registry_priv *regsty = dvobj_to_regsty(dvobj);
	struct rf_ctl_t *rfctl = dvobj_to_rfctl(dvobj);

	if (regsty->adaptivity_en == 0) {
		/* force disable */
		rfctl->edcca_mode_2g = RTW_EDCCA_NORM;
		#if CONFIG_IEEE80211_BAND_5GHZ
		rfctl->edcca_mode_5g = RTW_EDCCA_NORM;
		#endif
		#if CONFIG_IEEE80211_BAND_6GHZ
		rfctl->edcca_mode_6g = RTW_EDCCA_NORM;
		#endif

	} else if (regsty->adaptivity_en == 1) {
		/* force enable */
		if (!regsty->adaptivity_mode) {
			/* adaptivity */
			rfctl->edcca_mode_2g = RTW_EDCCA_ADAPT;
			#if CONFIG_IEEE80211_BAND_5GHZ
			rfctl->edcca_mode_5g = RTW_EDCCA_ADAPT;
			#endif
			#if CONFIG_IEEE80211_BAND_6GHZ
			rfctl->edcca_mode_6g = RTW_EDCCA_ADAPT;
			#endif
		} else {
			/* carrier sense */
			rfctl->edcca_mode_2g = RTW_EDCCA_CS;
			#if CONFIG_IEEE80211_BAND_5GHZ
			rfctl->edcca_mode_5g = RTW_EDCCA_CS;
			#endif
			#if CONFIG_IEEE80211_BAND_6GHZ
			rfctl->edcca_mode_6g = RTW_EDCCA_CS;
			#endif
		}

	} else {
		u8 modes[BAND_MAX];
		char buf[EDCCA_MODES_STR_LEN];

		/* by regulatory setting */
		#ifdef CONFIG_REGD_SRC_FROM_OS
		if (rfctl->regd_src == REGD_SRC_OS
			&& rfctl->ChannelPlan == RTW_CHPLAN_UNSPECIFIED
		) {
			modes[BAND_ON_24G] = rfctl->edcca_mode_2g = RTW_EDCCA_ADAPT;
			#if CONFIG_IEEE80211_BAND_5GHZ
			modes[BAND_ON_5G] = rfctl->edcca_mode_5g = RTW_EDCCA_ADAPT;
			#endif
			#if CONFIG_IEEE80211_BAND_6GHZ
			modes[BAND_ON_6G] = rfctl->edcca_mode_6g = RTW_EDCCA_ADAPT;
			#endif
			RTW_PRINT("mapping %scountry:%c%c to edcca_mode:%s\n"
				, RFCTL_REG_WORLDWIDE(rfctl) ? "" : "unsupported "
				, rfctl->alpha2[0]
				, rfctl->alpha2[1]
				, rtw_get_edcca_modes_str(buf, modes)
			);
		} else
		#endif
		{
			modes[BAND_ON_24G] = rfctl->edcca_mode_2g =
				rfctl->edcca_mode_2g_override != RTW_EDCCA_DEF ? rfctl->edcca_mode_2g_override :
				rtw_regd_to_edcca_mode(rtw_chplan_get_default_regd_2g(rfctl->ChannelPlan));
			#if CONFIG_IEEE80211_BAND_5GHZ
			modes[BAND_ON_5G] = rfctl->edcca_mode_5g =
				rfctl->edcca_mode_5g_override != RTW_EDCCA_DEF ? rfctl->edcca_mode_5g_override :
				rtw_regd_to_edcca_mode(rtw_chplan_get_default_regd_5g(rfctl->ChannelPlan));
			#endif
			#if CONFIG_IEEE80211_BAND_6GHZ
			modes[BAND_ON_6G] = rfctl->edcca_mode_6g =
				rfctl->edcca_mode_6g_override != RTW_EDCCA_DEF ? rfctl->edcca_mode_6g_override :
				rtw_regd_to_edcca_mode(rtw_chplan_get_default_regd_6g(rfctl->chplan_6g));
			#endif
			RTW_PRINT("update edcca_mode:%s\n"
				, rtw_get_edcca_modes_str(buf, modes)
			);
		}
	}
}

u8 rtw_get_edcca_mode(struct dvobj_priv *dvobj, enum band_type band)
{
	struct rf_ctl_t *rfctl = dvobj_to_rfctl(dvobj);
	u8 edcca_mode = RTW_EDCCA_NORM;

	if (band == BAND_ON_24G)
		edcca_mode = rfctl->edcca_mode_2g;
	#if CONFIG_IEEE80211_BAND_5GHZ
	else if (band == BAND_ON_5G)
		edcca_mode = rfctl->edcca_mode_5g;
	#endif
	#if CONFIG_IEEE80211_BAND_6GHZ
	else if (band == BAND_ON_6G)
		edcca_mode = rfctl->edcca_mode_6g;
	#endif

	return edcca_mode;
}

#if CONFIG_TXPWR_LIMIT
char *rtw_get_txpwr_lmt_name_of_bands_str(char *buf, const char *name_of_band[], u8 unknown_bmp)
{
#define NAME_DIFF ((void *)1)
/* input comes form organized database, string with same content will not have different pointer */
#define NAME_SEQ_COMPARE(result, operand) (result == NULL ? operand : (operand == NULL ? result : (result != operand ? NAME_DIFF : result)))

	const char *name = NULL;
	int cnt = 0;

	name = NAME_SEQ_COMPARE(name, name_of_band[BAND_ON_24G]);
#if CONFIG_IEEE80211_BAND_5GHZ
	name = NAME_SEQ_COMPARE(name, name_of_band[BAND_ON_5G]);
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	name = NAME_SEQ_COMPARE(name, name_of_band[BAND_ON_6G]);
#endif

	if (name != NAME_DIFF) { /* all available values are the same */
		cnt += snprintf(buf + cnt, TXPWR_NAME_OF_BANDS_STR_LEN - cnt - 1, "%s%s", (unknown_bmp & BIT(BAND_ON_24G)) ? "?" : "", name);
		if (cnt >= TXPWR_NAME_OF_BANDS_STR_LEN - 1)
			goto exit;
	} else {
		cnt += snprintf(buf + cnt, TXPWR_NAME_OF_BANDS_STR_LEN - cnt - 1, "%s%s ", (unknown_bmp & BIT(BAND_ON_24G)) ? "?" : "", name_of_band[BAND_ON_24G]);
		if (cnt >= TXPWR_NAME_OF_BANDS_STR_LEN - 1)
			goto exit;
		#if CONFIG_IEEE80211_BAND_5GHZ
		cnt += snprintf(buf + cnt, TXPWR_NAME_OF_BANDS_STR_LEN - cnt - 1, "%s%s ", (unknown_bmp & BIT(BAND_ON_5G)) ? "?" : "", name_of_band[BAND_ON_5G]);
		if (cnt >= TXPWR_NAME_OF_BANDS_STR_LEN - 1)
			goto exit;
		#endif
		#if CONFIG_IEEE80211_BAND_6GHZ
		cnt += snprintf(buf + cnt, TXPWR_NAME_OF_BANDS_STR_LEN - cnt - 1, "%s%s ", (unknown_bmp & BIT(BAND_ON_6G)) ? "?" : "", name_of_band[BAND_ON_6G]);
		if (cnt >= TXPWR_NAME_OF_BANDS_STR_LEN - 1)
			goto exit;
		#endif
		buf[cnt - 1] = 0;
	}

exit:
	return buf;
}

static void rtw_txpwr_apply_regd_req_default(struct rf_ctl_t *rfctl, char *names_of_band[], int names_of_band_len[])
{
	struct dvobj_priv *dvobj = rfctl_to_dvobj(rfctl);
	bool country_txpwr_lmt_override = 0;
	u8 txpwr_lmt[BAND_MAX];
	const char *name[BAND_MAX];
	u8 unknown_bmp = 0; /* unknown bitmap */
	char buf[TXPWR_NAME_OF_BANDS_STR_LEN];
	u8 band;
	bool altenate_applied = 0;

	for (band = 0; band < BAND_MAX; band++) {
		txpwr_lmt[band] = TXPWR_LMT_NONE;
		name[band] = NULL;
		unknown_bmp |= BIT(band);
	}

	if (rfctl->txpwr_lmt_override != TXPWR_LMT_DEF) {
		country_txpwr_lmt_override = 1;
		for (band = 0; band < BAND_MAX; band++)
			txpwr_lmt[band] = rfctl->txpwr_lmt_override;
	} else {
		txpwr_lmt[BAND_ON_2_4G] = rtw_regd_to_txpwr_lmt(rtw_chplan_get_default_regd_2g(rfctl->ChannelPlan));
		#if CONFIG_IEEE80211_BAND_5GHZ
		txpwr_lmt[BAND_ON_5G] = rtw_regd_to_txpwr_lmt(rtw_chplan_get_default_regd_5g(rfctl->ChannelPlan));
		#endif
		#if CONFIG_IEEE80211_BAND_6GHZ
		txpwr_lmt[BAND_ON_6G] = rtw_regd_to_txpwr_lmt(rtw_chplan_get_default_regd_6g(rfctl->chplan_6g));
		#endif
	}

	for (band = 0; band < BAND_MAX; band++) {
		name[band] = txpwr_lmt_str(txpwr_lmt[band]);
		if (txpwr_lmt[band] == TXPWR_LMT_NONE || txpwr_lmt[band] == TXPWR_LMT_WW || txpwr_lmt[band] == TXPWR_LMT_NUM
			|| rtw_txpwr_hal_lmt_reg_search(dvobj, name[band]))
			unknown_bmp &= ~BIT(band);
	}

	if (country_txpwr_lmt_override) {
		RTW_PRINT("default mapping country:%c%c to txpwr_lmt:%s\n"
			, rfctl->alpha2[0], rfctl->alpha2[1]
			, rtw_get_txpwr_lmt_name_of_bands_str(buf, name, unknown_bmp)
		);
	} else {
		RTW_PRINT("default mapping domain to txpwr_lmt:%s\n"
			, rtw_get_txpwr_lmt_name_of_bands_str(buf, name, unknown_bmp));
	}
	if (unknown_bmp == 0)
		goto exit;

	for (band = 0; band < BAND_MAX; band++) {
		if (!(unknown_bmp & BIT(band)))
			continue;
		if (TXPWR_LMT_ALTERNATE_DEFINED(txpwr_lmt[band])) {
			/*
			* To support older chips without new predefined txpwr_lmt:
			* - use txpwr_lmt_alternate() to get alternate if the selection is  not found
			*/
			altenate_applied = 1;
			txpwr_lmt[band] = txpwr_lmt_alternate(txpwr_lmt[band]);
			name[band] = txpwr_lmt_str(txpwr_lmt[band]);
			if (rtw_txpwr_hal_lmt_reg_search(dvobj, name[band]))
				unknown_bmp &= ~BIT(band);
		}
	}
	if (altenate_applied) {
		RTW_PRINT("alternate applied txpwr_lmt:%s\n"
			, rtw_get_txpwr_lmt_name_of_bands_str(buf, name, unknown_bmp));
		if (unknown_bmp == 0)
			goto exit;
	}

	for (band = 0; band < BAND_MAX; band++) {
		if (!(unknown_bmp & BIT(band)))
			continue;
		txpwr_lmt[band] = TXPWR_LMT_WW;
		name[band] = txpwr_lmt_str(txpwr_lmt[band]);
		unknown_bmp &= ~BIT(band);
	}
	RTW_PRINT("world wide applied txpwr_lmt:%s\n"
		, rtw_get_txpwr_lmt_name_of_bands_str(buf, name, unknown_bmp));

exit:
	for (band = 0; band < BAND_MAX; band++)
		ustrs_add(&names_of_band[band], &names_of_band_len[band], name[band]);
}

static void rtw_txpwr_apply_regd_req(struct rf_ctl_t *rfctl, char *names_of_band[], int names_of_band_len[])
{
	struct dvobj_priv *dvobj = rfctl_to_dvobj(rfctl);
	const char *name = NULL;
	u8 band;
	enum txpwr_lmt_reg_exc_match exc;

	/* search from exception mapping */
	exc = rtw_txpwr_hal_lmt_reg_exc_search(dvobj, rfctl->alpha2, rfctl->ChannelPlan, &name);
	if (exc) {
		bool exist = true;

		if (strcmp(name, txpwr_lmt_str(TXPWR_LMT_NONE)) != 0
			&& strcmp(name, txpwr_lmt_str(TXPWR_LMT_WW)) != 0
		) {
			if (!rtw_txpwr_hal_lmt_reg_search(dvobj, name))
				exist = false;
		}

		if (exc == TXPWR_LMT_REG_EXC_MATCH_COUNTRY) {
			RTW_PRINT("exception mapping country:"ALPHA2_FMT" to%s txpwr_lmt:%s\n"
				, ALPHA2_ARG(rfctl->alpha2), exist ? "" : " unknown", name);
		} else {
			RTW_PRINT("exception mapping domain:0x%02x to%s txpwr_lmt:%s\n"
				, rfctl->ChannelPlan, exist ? "" : " unknown", name);
		}
		if (!exist)
			name = NULL;

		if (name) {
			for (band = 0; band < BAND_MAX; band++)
				ustrs_add(&names_of_band[band], &names_of_band_len[band], name);
			return;
		}
	}

#ifdef CONFIG_REGD_SRC_FROM_OS
	if (rfctl->regd_src == REGD_SRC_OS) {
		char alpha2[3] = {rfctl->alpha2[0], rfctl->alpha2[1], 0};

		if (IS_ALPHA2_WORLDWIDE(alpha2))
			name = txpwr_lmt_str(TXPWR_LMT_WW);
		else {
			if (rtw_txpwr_hal_lmt_reg_search(dvobj, alpha2))
				name = alpha2;
		}

		if (name) {
			for (band = 0; band < BAND_MAX; band++)
				ustrs_add(&names_of_band[band], &names_of_band_len[band], name);

			RTW_PRINT("mapping country:%c%c to txpwr_lmt:%s\n"
				, rfctl->alpha2[0]
				, rfctl->alpha2[1]
				, name
			);
			return;
		}

		if (rfctl->ChannelPlan == RTW_CHPLAN_UNSPECIFIED) {
			name = txpwr_lmt_str(TXPWR_LMT_WW);
			for (band = 0; band < BAND_MAX; band++)
				ustrs_add(&names_of_band[band], &names_of_band_len[band], name);

			RTW_PRINT("mapping unsupported country:%c%c to txpwr_lmt:%s\n"
				, rfctl->alpha2[0]
				, rfctl->alpha2[1]
				, name
			);
			return;
		}
	}
#endif

	/* follow default channel plan mapping */
	rtw_txpwr_apply_regd_req_default(rfctl, names_of_band, names_of_band_len);
}

void rtw_txpwr_update_cur_lmt_regs(struct dvobj_priv *dvobj)
{
	struct rf_ctl_t *rfctl = dvobj_to_rfctl(dvobj);
	char *names[BAND_MAX];
	int names_len[BAND_MAX];
	u8 band;

	_rtw_memset(names, 0, sizeof(names));
	_rtw_memset(names_len, 0, sizeof(names_len));

	rtw_txpwr_apply_regd_req(rfctl, names, names_len);

	/* set to tx power limit regulations to HAL */
	for (band = 0; band < BAND_MAX; band++)
		rtw_txpwr_hal_set_current_lmt_regs_by_name(dvobj
			, band, names[band], names_len[band]);

	for (band = 0; band < BAND_MAX; band++)
		if (names[band] && names_len[band])
			rtw_mfree(names[band], names_len[band]);
}
#endif /* CONFIG_TXPWR_LIMIT */

static const struct country_chplan world_wide_chplan =
	COUNTRY_CHPLAN_ENT(WORLDWIDE_ALPHA2, RTW_CHPLAN_WORLDWIDE, RTW_CHPLAN_6G_WORLDWIDE, DEF, 1, 1);

#ifdef CONFIG_CUSTOMIZED_COUNTRY_CHPLAN_MAP
#include "../platform/custom_country_chplan.h"
#elif RTW_DEF_MODULE_REGULATORY_CERT
#include "./def_module_country_chplan.h"
#endif

/*
* rtw_get_chplan_worldwide -
* @ent: the buf to copy country_chplan entry content
*/
void rtw_get_chplan_worldwide(struct country_chplan *ent)
{
	_rtw_memcpy(ent, &world_wide_chplan, sizeof(*ent));
}

/*
* rtw_get_chplan_from_country -
* @country_code: string of country code
* @ent: the buf to copy country_chplan entry content
*
* Return _TRUE or _FALSE when unsupported country_code is given
*/
bool rtw_get_chplan_from_country(const char *country_code, struct country_chplan *ent)
{
#if defined(CONFIG_CUSTOMIZED_COUNTRY_CHPLAN_MAP) || RTW_DEF_MODULE_REGULATORY_CERT
	const struct country_chplan *map = NULL;
	u16 map_sz = 0;
	int i;
#endif
	char code[2] = {alpha_to_upper(country_code[0]), alpha_to_upper(country_code[1])};

#if defined(CONFIG_CUSTOMIZED_COUNTRY_CHPLAN_MAP) || RTW_DEF_MODULE_REGULATORY_CERT
	#ifdef CONFIG_CUSTOMIZED_COUNTRY_CHPLAN_MAP
	map = CUSTOMIZED_country_chplan_map;
	map_sz = sizeof(CUSTOMIZED_country_chplan_map) / sizeof(struct country_chplan);
	#else
	map_sz = rtw_def_module_country_chplan_map(&map);
	#endif

	for (i = 0; i < map_sz; i++) {
		if (strncmp(code, map[i].alpha2, 2) == 0) {
			if (ent)
				_rtw_memcpy(ent, &map[i], sizeof(*ent));
			return true;
		}
	}

	return false;
#else
	if (regdb_ops.get_chplan_from_alpha2)
		return regdb_ops.get_chplan_from_alpha2(code, ent);
	return false;
#endif
}

void rtw_chplan_ioctl_input_mapping(u16 *chplan, u16 *chplan_6g)
{
	if (chplan) {
		if (*chplan == RTW_CHPLAN_IOCTL_UNSPECIFIED)
			*chplan = RTW_CHPLAN_UNSPECIFIED;
		else if (*chplan == RTW_CHPLAN_IOCTL_NULL)
			*chplan = RTW_CHPLAN_NULL;
	}

	if (chplan_6g) {
		if (*chplan_6g == RTW_CHPLAN_IOCTL_UNSPECIFIED)
			*chplan_6g = RTW_CHPLAN_6G_UNSPECIFIED;
		else if (*chplan_6g == RTW_CHPLAN_IOCTL_NULL)
			*chplan_6g = RTW_CHPLAN_6G_NULL;
	}
}

bool rtw_chplan_ids_is_world_wide(u8 chplan, u8 chplan_6g)
{
	return !(chplan == RTW_CHPLAN_NULL
				#if CONFIG_IEEE80211_BAND_6GHZ
				&& chplan_6g == RTW_CHPLAN_6G_NULL
				#endif
			)
			&& (chplan == RTW_CHPLAN_WORLDWIDE || chplan == RTW_CHPLAN_NULL)
			#if CONFIG_IEEE80211_BAND_6GHZ
			&& (chplan_6g == RTW_CHPLAN_6G_WORLDWIDE || chplan_6g == RTW_CHPLAN_6G_NULL)
			#endif
			;
}

/*
 * Check if the @param ch, bw, offset is valid for the given @param ent
 * @ent: the given country chplan ent
 * @band: the given band
 * @ch: the given channel number
 * @bw: the given bandwidth
 * @offset: the given channel offset
 *
 * return valid (1) or not (0)
 */
u8 rtw_country_chplan_is_bchbw_valid(struct country_chplan *ent, enum band_type band, u8 ch, u8 bw, u8 offset
	, bool allow_primary_passive, bool allow_passive, struct registry_priv *regsty)
{
	u8 chplan_6g = RTW_CHPLAN_6G_NULL;
	u8 valid = 0;

	if (bw >= CHANNEL_WIDTH_80 && !COUNTRY_CHPLAN_EN_11AC(ent))
		goto exit;

	#if CONFIG_IEEE80211_BAND_6GHZ
	chplan_6g = ent->chplan_6g;
	#endif

	valid = rtw_chplan_is_bchbw_valid(ent->chplan, chplan_6g, band, ch, bw, offset
		, allow_primary_passive, allow_passive, regsty);

exit:
	return valid;
}

static void rtw_country_chplan_get_edcca_modes(const struct country_chplan *ent, u8 modes[])
{
	modes[BAND_ON_24G] =
		ent->edcca_mode_2g_override != RTW_EDCCA_DEF ? ent->edcca_mode_2g_override :
		rtw_regd_to_edcca_mode(rtw_chplan_get_default_regd_2g(ent->chplan));
	#if CONFIG_IEEE80211_BAND_5GHZ
	modes[BAND_ON_5G] =
		ent->edcca_mode_5g_override != RTW_EDCCA_DEF ? ent->edcca_mode_5g_override :
		rtw_regd_to_edcca_mode(rtw_chplan_get_default_regd_5g(ent->chplan));
	#endif
	#if CONFIG_IEEE80211_BAND_6GHZ
	modes[BAND_ON_6G] =
		ent->edcca_mode_6g_override != RTW_EDCCA_DEF ? ent->edcca_mode_6g_override :
		rtw_regd_to_edcca_mode(rtw_chplan_get_default_regd_6g(ent->chplan_6g));
	#endif
}

static void rtw_country_chplan_get_txpwr_lmt_of_bands(const struct country_chplan *ent, u8 txpwr_lmt_of_band[])
{
	txpwr_lmt_of_band[BAND_ON_24G] =
		ent->txpwr_lmt_override != TXPWR_LMT_DEF ? ent->txpwr_lmt_override :
		rtw_regd_to_txpwr_lmt(rtw_chplan_get_default_regd_2g(ent->chplan));
	#if CONFIG_IEEE80211_BAND_5GHZ
	txpwr_lmt_of_band[BAND_ON_5G] =
		ent->txpwr_lmt_override != TXPWR_LMT_DEF ? ent->txpwr_lmt_override :
		rtw_regd_to_txpwr_lmt(rtw_chplan_get_default_regd_5g(ent->chplan));
	#endif
	#if CONFIG_IEEE80211_BAND_6GHZ
	txpwr_lmt_of_band[BAND_ON_6G] =
		ent->txpwr_lmt_override != TXPWR_LMT_DEF ? ent->txpwr_lmt_override :
		rtw_regd_to_txpwr_lmt(rtw_chplan_get_default_regd_6g(ent->chplan_6g));
	#endif
}

#ifdef CONFIG_80211D
const char *const _cis_status_str[] = {
	[COUNTRY_IE_SLAVE_NOCOUNTRY]	= "NOCOUNTRY",
	[COUNTRY_IE_SLAVE_UNKNOWN]		= "UNKNOWN",
	[COUNTRY_IE_SLAVE_OPCH_NOEXIST]	= "OPCH_NOEXIST",
	[COUNTRY_IE_SLAVE_APPLICABLE]	= "APPLICABLE",
	[COUNTRY_IE_SLAVE_STATUS_NUM]	= "INVALID",
};

void dump_country_ie_slave_records(void *sel, struct rf_ctl_t *rfctl, bool skip_noset)
{
	int i, j;

	RTW_PRINT_SEL(sel, "     %-6s %-4s %-4s %s\n", "alpha2", "band", "opch", "status");
	for (i = 0; i < CONFIG_IFACE_NUMBER; i++) {
		for (j = 0; j < RTW_RLINK_MAX; j++) {
			if (skip_noset && strncmp(rfctl->cisr[i][j].alpha2, "\x00\x00", 2) == 0)
				continue;
			RTW_PRINT_SEL(sel, "%c%d %d     "ALPHA2_FMT" %4s %4u %s\n"
				, rfctl->effected_cisr == &rfctl->cisr[i][j] ? '*' : ' ', i, j, ALPHA2_ARG(rfctl->cisr[i][j].alpha2)
				, band_str(rfctl->cisr[i][j].band), rfctl->cisr[i][j].opch, cis_status_str(rfctl->cisr[i][j].status));
		}
	}
}

enum country_ie_slave_status rtw_get_cisr_from_recv_country_ie(struct registry_priv *regsty
		, enum band_type band, u8 opch, const u8 *country_ie
		, struct country_ie_slave_record *cisr, const char *caller_msg)
{
	const char *country_code = country_ie ? country_ie + 2 : NULL;
	u8 chplan_6g = RTW_CHPLAN_6G_NULL;
	struct country_chplan *ent = &cisr->chplan;
	enum country_ie_slave_status ret;

	_rtw_memcpy(cisr->alpha2, country_code ? country_code : "\x00\x00", 2);
	cisr->band = band;
	cisr->opch = opch;

	_rtw_memset(ent, 0, sizeof(*ent));

	if (!country_code || strncmp(country_code, "XX", 2) == 0) {
		if (caller_msg) {
			if (country_code)
				RTW_INFO("%s noncountry \"XX\"\n", caller_msg);
			else
				RTW_INFO("%s no country ie\n", caller_msg);
		}
		ret = COUNTRY_IE_SLAVE_NOCOUNTRY;
		goto exit;
	}

	if (!rtw_get_chplan_from_country(country_code, ent)) {
		if (caller_msg) {
			if (is_alpha(country_code[0]) == _FALSE || is_alpha(country_code[1]) == _FALSE) {
				RTW_INFO("%s country_code {0x%02x, 0x%02x} is not alpha2, use world wide instead\n"
					, caller_msg, country_code[0], country_code[1]);
			} else {
				RTW_INFO("%s unsupported country_code:\"%c%c\", use world wide\n"
					, caller_msg, country_code[0], country_code[1]);
			}
		}
		rtw_get_chplan_worldwide(ent);
		ret = COUNTRY_IE_SLAVE_UNKNOWN;
		goto exit;
	}

	#if CONFIG_IEEE80211_BAND_6GHZ
	chplan_6g = ent->chplan_6g;
	#endif

	if (!rtw_chplan_is_bchbw_valid(ent->chplan, chplan_6g, band, opch
			, CHANNEL_WIDTH_20, CHAN_OFFSET_NO_EXT, 1, 1, regsty)
	) {
		u8 edcca_modes[BAND_MAX];

		if (caller_msg) {
			RTW_INFO("%s \"%c%c\" no band:%s ch:%u, use world wide with ori edcca modes\n"
				, caller_msg, country_code[0], country_code[1], band_str(band), opch);
		}
		rtw_country_chplan_get_edcca_modes(ent, edcca_modes);
		rtw_get_chplan_worldwide(ent);
		ent->edcca_mode_2g_override = edcca_modes[BAND_ON_24G];
		#if CONFIG_IEEE80211_BAND_5GHZ
		ent->edcca_mode_5g_override = edcca_modes[BAND_ON_5G];
		#endif
		#if CONFIG_IEEE80211_BAND_6GHZ
		ent->edcca_mode_6g_override = edcca_modes[BAND_ON_6G];
		#endif
		ret = COUNTRY_IE_SLAVE_OPCH_NOEXIST;
		goto exit;
	}

	if (caller_msg) {
		RTW_INFO("%s country_code:\"%c%c\" is applicapble\n"
			, caller_msg, country_code[0], country_code[1]);
	}
	ret = COUNTRY_IE_SLAVE_APPLICABLE;

exit:
	cisr->status = ret;

	return ret;
}
#endif /* CONFIG_80211D */

void dump_country_chplan(void *sel, const struct country_chplan *ent, bool regd_info)
{
	char buf[16];
	char *pos = buf;

	if (ent->chplan == RTW_CHPLAN_UNSPECIFIED)
		pos += sprintf(pos, "NA");
	else
		pos += sprintf(pos, "0x%02X", ent->chplan);

#if CONFIG_IEEE80211_BAND_6GHZ
	if (ent->chplan_6g == RTW_CHPLAN_6G_UNSPECIFIED)
		pos += sprintf(pos, " NA");
	else
		pos += sprintf(pos, " 0x%02X", ent->chplan_6g);
#endif

	RTW_PRINT_SEL(sel, "\"%c%c\", %s"
		, ent->alpha2[0], ent->alpha2[1], buf);

	if (regd_info) {
		u8 edcca_modes[BAND_MAX];
		u8 txpwr_lmt[BAND_MAX];

		rtw_country_chplan_get_edcca_modes(ent, edcca_modes);
		_RTW_PRINT_SEL(sel, " {%-6s", rtw_edcca_mode_str(edcca_modes[BAND_ON_24G]));
		#if CONFIG_IEEE80211_BAND_5GHZ
		_RTW_PRINT_SEL(sel, " %-6s", rtw_edcca_mode_str(edcca_modes[BAND_ON_5G]));
		#endif
		#if CONFIG_IEEE80211_BAND_6GHZ
		_RTW_PRINT_SEL(sel, " %-6s", rtw_edcca_mode_str(edcca_modes[BAND_ON_6G]));
		#endif
		_RTW_PRINT_SEL(sel, "}");

		rtw_country_chplan_get_txpwr_lmt_of_bands(ent, txpwr_lmt);
		_RTW_PRINT_SEL(sel, " {%-7s", txpwr_lmt_str(txpwr_lmt[BAND_ON_24G]));
		#if CONFIG_IEEE80211_BAND_5GHZ
		_RTW_PRINT_SEL(sel, " %-7s", txpwr_lmt_str(txpwr_lmt[BAND_ON_5G]));
		#endif
		#if CONFIG_IEEE80211_BAND_6GHZ
		_RTW_PRINT_SEL(sel, " %-7s", txpwr_lmt_str(txpwr_lmt[BAND_ON_6G]));
		#endif
		_RTW_PRINT_SEL(sel, "}");
	}

	_RTW_PRINT_SEL(sel, " %s", COUNTRY_CHPLAN_EN_11AX(ent) ? "ax" : "  ");
	_RTW_PRINT_SEL(sel, " %s", COUNTRY_CHPLAN_EN_11AC(ent) ? "ac" : "  ");

	_RTW_PRINT_SEL(sel, "\n");
}

void dump_country_chplan_map(void *sel, bool regd_info)
{
	struct country_chplan ent;
	u8 code[2];

#if RTW_DEF_MODULE_REGULATORY_CERT
	RTW_PRINT_SEL(sel, "RTW_DEF_MODULE_REGULATORY_CERT:0x%x\n", RTW_DEF_MODULE_REGULATORY_CERT);
#endif
#ifdef CONFIG_CUSTOMIZED_COUNTRY_CHPLAN_MAP
	RTW_PRINT_SEL(sel, "CONFIG_CUSTOMIZED_COUNTRY_CHPLAN_MAP\n");
#endif

	rtw_get_chplan_worldwide(&ent);
	dump_country_chplan(sel, &ent, regd_info);

	for (code[0] = 'A'; code[0] <= 'Z'; code[0]++) {
		for (code[1] = 'A'; code[1] <= 'Z'; code[1]++) {
			if (!rtw_get_chplan_from_country(code, &ent))
				continue;

			dump_country_chplan(sel, &ent, regd_info);
		}
	}
}

void dump_country_list(void *sel)
{
	u8 code[2];

	RTW_PRINT_SEL(sel, "%s ", WORLDWIDE_ALPHA2);

	for (code[0] = 'A'; code[0] <= 'Z'; code[0]++) {
		for (code[1] = 'A'; code[1] <= 'Z'; code[1]++) {
			if (!rtw_get_chplan_from_country(code, NULL))
				continue;
			_RTW_PRINT_SEL(sel, "%c%c ", code[0], code[1]);
		}
	}
	_RTW_PRINT_SEL(sel, "\n");
}

void dump_chplan_id_list(void *sel)
{
	u8 id_search_max = 255;
	u8 first = 1;
	int i;

	for (i = 0; i <= id_search_max; i++) {
		if (!rtw_is_channel_plan_valid(i))
			continue;

		if (first) {
			RTW_PRINT_SEL(sel, "0x%02X ", i);
			first = 0;
		} else
			_RTW_PRINT_SEL(sel, "0x%02X ", i);
	}
	if (first == 0)
		_RTW_PRINT_SEL(sel, "\n");
}

void dump_chplan_country_list(void *sel)
{
	u8 id_search_max = 255;
	int i;

	for (i = 0; i <= id_search_max; i++) {
		struct country_chplan ent;
		u8 code[2];
		u8 first;

		if (!rtw_is_channel_plan_valid(i))
			continue;

		first = 1;
		for (code[0] = 'A'; code[0] <= 'Z'; code[0]++) {
			for (code[1] = 'A'; code[1] <= 'Z'; code[1]++) {
				if (!rtw_get_chplan_from_country(code, &ent) || ent.chplan != i)
					continue;

				if (first) {
					RTW_PRINT_SEL(sel, "0x%02X %c%c ", i, code[0], code[1]);
					first = 0;
				} else
					_RTW_PRINT_SEL(sel, "%c%c ", code[0], code[1]);
			}
		}
		if (first == 0)
			_RTW_PRINT_SEL(sel, "\n");
	}
}

#if CONFIG_IEEE80211_BAND_6GHZ
void dump_chplan_6g_id_list(void *sel)
{
	u8 id_search_max = 255;
	u8 first = 1;
	int i;

	for (i = 0; i <= id_search_max; i++) {
		if (!rtw_is_channel_plan_6g_valid(i))
			continue;

		if (first) {
			RTW_PRINT_SEL(sel, "0x%02X ", i);
			first = 0;
		} else
			_RTW_PRINT_SEL(sel, "0x%02X ", i);
	}
	if (first == 0)
		_RTW_PRINT_SEL(sel, "\n");
}

void dump_chplan_6g_country_list(void *sel)
{
	u8 id_search_max = 255;
	int i;

	for (i = 0; i <= id_search_max; i++) {
		struct country_chplan ent;
		u8 code[2];
		u8 first;

		if (!rtw_is_channel_plan_6g_valid(i))
			continue;

		first = 1;
		for (code[0] = 'A'; code[0] <= 'Z'; code[0]++) {
			for (code[1] = 'A'; code[1] <= 'Z'; code[1]++) {
				if (!rtw_get_chplan_from_country(code, &ent) || ent.chplan_6g != i)
					continue;

				if (first) {
					RTW_PRINT_SEL(sel, "0x%02X %c%c ", i, code[0], code[1]);
					first = 0;
				} else
					_RTW_PRINT_SEL(sel, "%c%c ", code[0], code[1]);
			}
		}
		if (first == 0)
			_RTW_PRINT_SEL(sel, "\n");
	}
}
#endif /* CONFIG_IEEE80211_BAND_6GHZ */

#ifdef CONFIG_RTW_DEBUG
void dump_chplan_test(void *sel)
{
	if (regdb_ops.dump_chplan_test)
		regdb_ops.dump_chplan_test(sel);
}
#endif /* CONFIG_RTW_DEBUG */

void dump_chplan_ver(void *sel)
{
	char buf[CHPLAN_VER_STR_BUF_LEN] = {0};

	if (regdb_ops.get_ver_str)
		regdb_ops.get_ver_str(buf, CHPLAN_VER_STR_BUF_LEN);
	RTW_PRINT_SEL(sel, "%s\n", buf);
}

void rtw_chplan_set_regdb_ctx(struct dvobj_priv *dvobj)
{
	if (regdb_ops.set_ctx)
		regdb_ops.set_ctx(dvobj);
}

void rtw_chplan_clear_regdb_ctx(struct dvobj_priv *dvobj)
{
	if (regdb_ops.clear_ctx)
		regdb_ops.clear_ctx(dvobj);
}

static enum channel_width rtw_regd_adjust_linking_bw(struct rf_ctl_t *rfctl, struct registry_priv *regsty
	, enum band_type band, u8 ch, enum channel_width bw, enum chan_offset offset
	, struct country_chplan *cis_chplan, u8 cis_iface_id, u8 cis_link_id)
{
#ifndef DBG_REGD_ADJUST_LINKING_BW
#define DBG_REGD_ADJUST_LINKING_BW 0
#endif

	struct rtw_chset *chset = &rfctl->chset;

	#if DBG_REGD_ADJUST_LINKING_BW
	RTW_INFO("%s %s ch:%u,%u,%u cis_chplan:%p,cis_iface_id:%u, cis_link_id:%u\n"
		, __func__, band_str(band), ch, bw, offset, cis_chplan, cis_iface_id, cis_link_id);
	#endif

	if (bw == CHANNEL_WIDTH_20)
		goto exit;

	for (; bw > CHANNEL_WIDTH_20; bw--) {
		if (rtw_chset_is_bchbw_non_ocp(chset, band, ch, bw, offset)) {
			#if DBG_REGD_ADJUST_LINKING_BW
			RTW_INFO("%s %s ch:%u,%u,%u not allowed by non_ocp\n", __func__, band_str(band), ch, bw, offset);
			#endif
			continue;
		}

#ifdef CONFIG_80211D
		if (cis_chplan && !rtw_country_chplan_is_bchbw_valid(cis_chplan, band, ch, bw, offset, true, true, regsty)) {
			#if DBG_REGD_ADJUST_LINKING_BW
			RTW_INFO("%s %s ch:%u,%u,%u not allowed by cis_chplan\n", __func__, band_str(band), ch, bw, offset);
			#endif
			continue;
		}
#endif

		if (!cis_chplan && !rtw_chset_is_bchbw_valid(chset, band, ch, bw, offset, true, true)) {
			#if DBG_REGD_ADJUST_LINKING_BW
			RTW_INFO("%s %s ch:%u,%u,%u not allowed by chset\n", __func__, band_str(band), ch, bw, offset);
			#endif
			continue;
		}
		break;
	}

	if (bw == CHANNEL_WIDTH_20)
		offset = CHAN_OFFSET_NO_EXT;

exit:
#ifdef CONFIG_80211D
	if (cis_chplan)
		rtw_warn_on(!rtw_country_chplan_is_bchbw_valid(cis_chplan, band, ch, bw, offset, true, true, regsty));
	else
#endif
		rtw_warn_on(!rtw_chset_is_bchbw_valid(chset, band, ch, bw, offset, true, true));
	rtw_warn_on(rtw_chset_is_bchbw_non_ocp(chset, band, ch, bw, offset));

	return bw;
}

enum channel_width alink_adjust_linking_bw_by_regd(struct _ADAPTER_LINK *alink
	, enum band_type band, u8 ch, enum channel_width bw, enum chan_offset offset
	, struct country_chplan *cis_chplan)
{
	_adapter *adapter = alink->adapter;
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);
	struct registry_priv *regsty = adapter_to_regsty(adapter);
	u8 cis_iface_id = adapter->iface_id;
	u8 cis_alink_id = rtw_adapter_link_get_id(alink);

	return rtw_regd_adjust_linking_bw(rfctl, regsty, band, ch, bw, offset, cis_chplan, cis_iface_id, cis_alink_id);
}

enum channel_width adapter_adjust_linking_bw_by_regd(_adapter *adapter
	, enum band_type band, u8 ch, enum channel_width bw, enum chan_offset offset
	, struct country_chplan *cis_chplan)
{
	struct _ADAPTER_LINK *alink = GET_PRIMARY_LINK(adapter);
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);
	struct registry_priv *regsty = adapter_to_regsty(adapter);
	u8 cis_iface_id = adapter->iface_id;
	u8 cis_alink_id = rtw_adapter_link_get_id(alink);

	return rtw_regd_adjust_linking_bw(rfctl, regsty, band, ch, bw, offset, cis_chplan, cis_iface_id, cis_alink_id);
}

/*
 * Description:
 *	Use hardware(efuse), driver parameter(registry) and default channel plan
 *	to decide which one should be used.
 *
 * Parameters:
 *	rfctl				pointer of rfctl
 *	hw_alpha2		country code from HW (efuse/eeprom/mapfile)
 *	hw_chplan		domain code from HW (efuse/eeprom/mapfile)
 *	hw_chplan_6g	6g domain code from HW (efuse/eeprom/mapfile)
 *	hw_force_chplan	if forcing HW channel plan setting (efuse/eeprom/mapfile)
 *					will modified tif HW channel plan setting is invlid, will
 */
void rtw_rfctl_decide_init_chplan(struct rf_ctl_t *rfctl,
	const char *hw_alpha2, u8 hw_chplan, u8 hw_chplan_6g, u8 hw_force_chplan)
{
	struct registry_priv *regsty;
	char *sw_alpha2;
	const struct country_chplan *country_ent = NULL;
	struct country_chplan ent;
	int chplan = -1;
	int chplan_6g = -1;

	u8 sw_chplan;
	u8 def_chplan = RTW_CHPLAN_WORLDWIDE; /* worldwide,  used when HW, SW both invalid */
#if CONFIG_IEEE80211_BAND_6GHZ
	u8 sw_chplan_6g;
	u8 def_chplan_6g = RTW_CHPLAN_6G_WORLDWIDE; /* worldwide,  used when HW, SW both invalid */
#endif

	if (hw_alpha2) {
		if (strlen(hw_alpha2) != 2 || !is_alpha(hw_alpha2[0]) || is_alpha(hw_alpha2[1]))
			RTW_PRINT("%s hw_alpha2 is not valid alpha2\n", __func__);
		else if (rtw_get_chplan_from_country(hw_alpha2, &ent)) {
			/* get chplan from hw country code, by pass hw chplan setting */
			country_ent = &ent;
			chplan = ent.chplan;
			#if CONFIG_IEEE80211_BAND_6GHZ
			chplan_6g = ent.chplan_6g;
			#endif
			goto chk_sw_config;
		} else
			RTW_PRINT("%s unsupported hw_alpha2:\"%c%c\"\n", __func__, hw_alpha2[0], hw_alpha2[1]);
	}

	if (rtw_is_channel_plan_valid(hw_chplan))
		chplan = hw_chplan;
	else if (hw_force_chplan == _TRUE) {
		RTW_PRINT("%s unsupported hw_chplan:0x%02X\n", __func__, hw_chplan);
		/* hw infomaton invalid, refer to sw information */
		hw_force_chplan = _FALSE;
	}

#if CONFIG_IEEE80211_BAND_6GHZ
	if (rtw_is_channel_plan_6g_valid(hw_chplan_6g))
		chplan_6g = hw_chplan_6g;
	else if (hw_force_chplan == _TRUE) {
		RTW_PRINT("%s unsupported hw_chplan_6g:0x%02X\n", __func__, hw_chplan_6g);
		/* hw infomaton invalid, refer to sw information */
		hw_force_chplan = _FALSE;
	}
#endif

chk_sw_config:
	if (hw_force_chplan == _TRUE)
		goto done;

	regsty = dvobj_to_regsty(rfctl_to_dvobj(rfctl));
	sw_alpha2 = regsty->alpha2;
	sw_chplan = regsty->channel_plan;
	#if CONFIG_IEEE80211_BAND_6GHZ
	sw_chplan_6g = regsty->channel_plan_6g;
	#endif

	if (sw_alpha2 && !IS_ALPHA2_UNSPEC(sw_alpha2)) {
		if (IS_ALPHA2_WORLDWIDE(sw_alpha2)
			|| rtw_get_chplan_from_country(sw_alpha2, &ent)
		) {
			/* get chplan from sw country code, by pass sw chplan setting */
			if (IS_ALPHA2_WORLDWIDE(sw_alpha2))
				rtw_get_chplan_worldwide(&ent);
			country_ent = &ent;
			chplan = ent.chplan;
			#if CONFIG_IEEE80211_BAND_6GHZ
			chplan_6g = ent.chplan_6g;
			#endif
			goto done;
		} else
			RTW_PRINT("%s unsupported sw_alpha2:\"%c%c\"\n", __func__, sw_alpha2[0], sw_alpha2[1]);
	}

	if (rtw_is_channel_plan_valid(sw_chplan)) {
		/* cancel hw_alpha2 because chplan is specified by sw_chplan */
		country_ent = NULL;
		chplan = sw_chplan;
	} else if (sw_chplan != RTW_CHPLAN_UNSPECIFIED)
		RTW_PRINT("%s unsupported sw_chplan:0x%02X\n", __func__, sw_chplan);

#if CONFIG_IEEE80211_BAND_6GHZ
	if (rtw_is_channel_plan_6g_valid(sw_chplan_6g)) {
		/* cancel hw_alpha2 because chplan_6g is specified by sw_chplan_6g */
		country_ent = NULL;
		chplan_6g = sw_chplan_6g;
	} else if (sw_chplan_6g != RTW_CHPLAN_6G_UNSPECIFIED)
		RTW_PRINT("%s unsupported sw_chplan_6g:0x%02X\n", __func__, sw_chplan_6g);
#endif

done:
	if (chplan == -1) {
		RTW_PRINT("%s use def_chplan:0x%02X\n", __func__, def_chplan);
		chplan = def_chplan;
	} else
		RTW_PRINT("%s chplan:0x%02X\n", __func__, chplan);

#if CONFIG_IEEE80211_BAND_6GHZ
	if (chplan_6g == -1) {
		RTW_PRINT("%s use def_chplan_6g:0x%02X\n", __func__, def_chplan_6g);
		chplan_6g = def_chplan_6g;
	} else
		RTW_PRINT("%s chplan_6g:0x%02X\n", __func__, chplan_6g);
#endif

	if (!country_ent) {
		if (rtw_chplan_ids_is_world_wide(chplan, chplan_6g))
			rtw_get_chplan_worldwide(&ent);
		else {
			SET_UNSPEC_ALPHA2(ent.alpha2);
			ent.edcca_mode_2g_override = RTW_EDCCA_DEF;
			#if CONFIG_IEEE80211_BAND_5GHZ
			ent.edcca_mode_5g_override = RTW_EDCCA_DEF;
			#endif
			#if CONFIG_IEEE80211_BAND_6GHZ
			ent.edcca_mode_6g_override = RTW_EDCCA_DEF;
			#endif
			ent.txpwr_lmt_override = TXPWR_LMT_DEF;
			#if defined(CONFIG_80211AX_HE) || defined(CONFIG_80211AC_VHT)
			ent.proto_en = CHPLAN_PROTO_EN_ALL;
			#endif
		}
	} else {
		RTW_PRINT("%s country code:\"%c%c\"\n", __func__
			, country_ent->alpha2[0], country_ent->alpha2[1]);
	}

	rfctl->disable_sw_chplan = hw_force_chplan;

	rfctl->regd_inr = RTW_REGD_SET_BY_INIT;
	rfctl->init_alpha2[0] = rfctl->alpha2[0] = ent.alpha2[0];
	rfctl->init_alpha2[1] = rfctl->alpha2[1] = ent.alpha2[1];
	rfctl->init_ChannelPlan = rfctl->ChannelPlan = chplan;
#if CONFIG_IEEE80211_BAND_6GHZ
	rfctl->init_chplan_6g = rfctl->chplan_6g = chplan_6g;
#endif
	rfctl->edcca_mode_2g_override = ent.edcca_mode_2g_override;
#if CONFIG_IEEE80211_BAND_5GHZ
	rfctl->edcca_mode_5g_override = ent.edcca_mode_5g_override;
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	rfctl->edcca_mode_6g_override = ent.edcca_mode_6g_override;
#endif
#if CONFIG_TXPWR_LIMIT
	rfctl->txpwr_lmt_override = ent.txpwr_lmt_override;
#endif
#if defined(CONFIG_80211AX_HE) || defined(CONFIG_80211AC_VHT)
	rfctl->proto_en = ent.proto_en;
#endif
}

bool rtw_rfctl_is_disable_sw_channel_plan(struct dvobj_priv *dvobj)
{
	return dvobj_to_rfctl(dvobj)->disable_sw_chplan;
}

static void get_str_of_set_chplan_keys(char *buf, u8 buf_len, struct SetChannelPlan_param *param)
{
	char *pos = buf;

#ifdef CONFIG_80211D
	if (param->has_cisr) {
		pos += snprintf(pos, buf_len - (pos - buf), "alid:%c alpha2:"ALPHA2_FMT" %s"
			, param->cisr_alink_id >= RTW_RLINK_MAX ? '-' : '0' + param->cisr_alink_id
			, ALPHA2_ARG(param->cisr.alpha2), cis_status_str(param->cisr.status));
	} else
#endif
	if (param->has_country)
		pos += snprintf(pos, buf_len - (pos - buf), "alpha2:"ALPHA2_FMT, ALPHA2_ARG(param->country_ent.alpha2));
	else {
		if (param->channel_plan == RTW_CHPLAN_UNSPECIFIED)
			pos += snprintf(pos, buf_len - (pos - buf), "chplan:NA");
		else
			pos += snprintf(pos, buf_len - (pos - buf), "chplan:0x%02X", param->channel_plan);

		#if CONFIG_IEEE80211_BAND_6GHZ
		if (param->channel_plan_6g == RTW_CHPLAN_6G_UNSPECIFIED)
			pos += snprintf(pos, buf_len - (pos - buf), " chplan_6g:NA");
		else
			pos += snprintf(pos, buf_len - (pos - buf), " chplan_6g:0x%02X", param->channel_plan_6g);
		#endif
	}
}

#ifdef CONFIG_80211D
enum cisr_match {
	CISR_MATCH			= 0, /* identically match */
	CISR_MATCH_CHPLAN	= 1, /* same chplan result */
	CISR_DIFF				 /* different (not above cases) */
};

static enum cisr_match rtw_cisr_compare(struct country_ie_slave_record *a, struct country_ie_slave_record *b)
{
	if (_rtw_memcmp(a, b, sizeof(*a)) == true)
		return CISR_MATCH;
	if (_rtw_memcmp(&a->chplan, &b->chplan, sizeof(a->chplan)) == true)
		return CISR_MATCH_CHPLAN;
	return CISR_DIFF;
}

static bool rtw_chplan_rtk_priv_req_prehdl_country_ie(_adapter *adapter, struct SetChannelPlan_param *param, const char *caller)
{
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);
	struct country_ie_slave_record ori_cisr_cont[RTW_RLINK_MAX];
	struct country_ie_slave_record ori_ecisr_cont;
	struct country_ie_slave_record *ori_ecisr = rfctl->effected_cisr;
	u8 iface_id = adapter->iface_id;
	u8 alink_id_s, alink_id_e, alink_id;
	int i, j;
	bool effected = false;
	char buf[32];

#ifdef CONFIG_RTW_DEBUG
	if (rtw_drv_log_level >= _DRV_DEBUG_) {
		RTW_PRINT("%s cisr before\n", caller);
		dump_country_ie_slave_records(RTW_DBGDUMP, rfctl, 0);
	}
#endif

	if (param->inr == RTW_REGD_SET_BY_USER
		&& rfctl->effected_cisr
	) {
		/* country IE setting is applied, user setting is only recorded but not applied */
		goto exit;
	}

	if (param->inr != RTW_REGD_SET_BY_COUNTRY_IE) {
		effected = true;
		goto exit;
	}

	if (param->cisr_alink_id < RTW_RLINK_MAX) {
		/* specific alink */
		alink_id_s = param->cisr_alink_id;
		alink_id_e = alink_id_s + 1;
	} else if (param->cisr_alink_id >= RTW_RLINK_MAX) {
		/* all alinks of specific iface */
		alink_id_s = 0;
		alink_id_e = RTW_RLINK_MAX;
	}

	/* compare original record with same iface_id & spcified alink_id range */
	for (alink_id = alink_id_s; alink_id < alink_id_e; alink_id++)
		if (rtw_cisr_compare(&rfctl->cisr[iface_id][alink_id], &param->cisr) != CISR_MATCH)
			break;
	if (alink_id >= alink_id_e) {
		/* record no change  */
		goto exit;
	}

	/* backup original content */
	for (alink_id = alink_id_s; alink_id < alink_id_e; alink_id++)
		_rtw_memcpy(&ori_cisr_cont[alink_id], &rfctl->cisr[iface_id][alink_id], sizeof(ori_cisr_cont[alink_id]));

	/* backup original effected content */
	if (ori_ecisr)
		_rtw_memcpy(&ori_ecisr_cont, ori_ecisr, sizeof(ori_ecisr_cont));

	/* update record */
	for (alink_id = alink_id_s; alink_id < alink_id_e; alink_id++)
		_rtw_memcpy(&rfctl->cisr[iface_id][alink_id], &param->cisr, sizeof(param->cisr));

	/* compare original record with same iface_id & spcified alink_id range for chplan change */
	for (alink_id = alink_id_s; alink_id < alink_id_e; alink_id++)
		if (rtw_cisr_compare(&ori_cisr_cont[alink_id], &param->cisr) > CISR_MATCH_CHPLAN)
			break;
	if (alink_id >= alink_id_e) {
		/* will take no effect  */
		goto exit;
	}

	/* select new effected one */
	{
		static const u8 status_score[] = { /* conservative policy */
			[COUNTRY_IE_SLAVE_UNKNOWN] = 3,
			[COUNTRY_IE_SLAVE_OPCH_NOEXIST] = 2,
			[COUNTRY_IE_SLAVE_APPLICABLE] = 1,
		};
		struct country_ie_slave_record *new_ecisr = NULL;

		for (i = 0; i < CONFIG_IFACE_NUMBER; i++) {
			for (j = 0; j < RTW_RLINK_MAX; j++) {
				if (rfctl->cisr[i][j].status == COUNTRY_IE_SLAVE_NOCOUNTRY)
					continue;
				if (!new_ecisr
					/* high score */
					|| status_score[rfctl->cisr[i][j].status] > status_score[new_ecisr->status]
					/* same score, prefer the same alpha2 as current effected(same score) one */
					|| (status_score[rfctl->cisr[i][j].status] == status_score[new_ecisr->status]
						&& ori_ecisr
						&& ori_ecisr_cont.status == new_ecisr->status
						&& _rtw_memcmp(ori_ecisr_cont.alpha2, new_ecisr->alpha2, 2) == _FALSE
						&& _rtw_memcmp(ori_ecisr_cont.alpha2, rfctl->cisr[i][j].alpha2, 2) == _TRUE)
				)
					new_ecisr = &rfctl->cisr[i][j];
			}
		}

		rfctl->effected_cisr = new_ecisr;

		if (rfctl->effected_cisr && ori_ecisr) {
			/* compare with original effected one  */
			if (_rtw_memcmp(&ori_ecisr_cont, new_ecisr, sizeof(ori_ecisr_cont)) == _TRUE) {
				/* same record content, no effect */
				goto exit;
			}
		}
	}

	effected = true;

	if (!rfctl->effected_cisr) {
		/* no country IE setting */
		const char *alpha2;

		if (strncmp(rfctl->user_alpha2, "\x00\x00", 2) != 0) {
			/* restore to user setting */
			param->inr = RTW_REGD_SET_BY_USER;
			alpha2 = rfctl->user_alpha2;
			param->channel_plan = rfctl->user_ChannelPlan;
			#if CONFIG_IEEE80211_BAND_6GHZ
			param->channel_plan_6g = rfctl->user_chplan_6g;
			#endif
		} else {
			/* restore to init setting */
			param->inr = RTW_REGD_SET_BY_INIT;
			alpha2 = rfctl->init_alpha2;
			param->channel_plan = rfctl->init_ChannelPlan;
			#if CONFIG_IEEE80211_BAND_6GHZ
			param->channel_plan_6g = rfctl->init_chplan_6g;
			#endif
		}

		if (IS_ALPHA2_UNSPEC(alpha2) || IS_ALPHA2_WORLDWIDE(alpha2))
			param->has_country = 0;
		else if (rtw_get_chplan_from_country(alpha2, &param->country_ent)) {
			param->channel_plan = param->country_ent.chplan;
			#if CONFIG_IEEE80211_BAND_6GHZ
			param->channel_plan_6g = param->country_ent.chplan_6g;
			#endif
			param->has_country = 1;
		} else {
			RTW_WARN("%s unexpected country_code:\"%c%c\", set to \"00\"\n", caller, alpha2[0], alpha2[1]);
			rtw_warn_on(1);
			rtw_get_chplan_worldwide(&param->country_ent);
			param->channel_plan = param->country_ent.chplan;
			#if CONFIG_IEEE80211_BAND_6GHZ
			param->channel_plan_6g = param->country_ent.chplan_6g;
			#endif
			param->has_country = 1;
		}
		param->has_cisr = 0;

		get_str_of_set_chplan_keys(buf, 32, param);
		RTW_INFO("%s restore inr:%s %s\n", caller, regd_inr_str(param->inr), buf);
	}
	else {
		/* has country IE setting */
		_rtw_memcpy(&param->country_ent, &rfctl->effected_cisr->chplan, sizeof(param->country_ent));
		param->channel_plan = param->country_ent.chplan;
		#if CONFIG_IEEE80211_BAND_6GHZ
		param->channel_plan_6g = param->country_ent.chplan_6g;
		#endif
		param->has_country = 1;
		param->has_cisr = 0;

		get_str_of_set_chplan_keys(buf, 32, param);
		RTW_INFO("%s trigger inr:%s %s\n", caller, regd_inr_str(param->inr), buf);
	}

exit:
#ifdef CONFIG_RTW_DEBUG
	if (rtw_drv_log_level >= _DRV_DEBUG_) {
		RTW_PRINT("%s cisr after\n", caller);
		dump_country_ie_slave_records(RTW_DBGDUMP, rfctl, 0);
	}
#endif

	return effected;
}
#endif /* CONFIG_80211D */

static bool rtw_chplan_rtk_priv_req_prehdl_domain_code(_adapter *adapter, struct SetChannelPlan_param *param, const char *caller)
{
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);

	/* disallow invalid input */
	if ((param->channel_plan != RTW_CHPLAN_UNSPECIFIED
		&& !rtw_is_channel_plan_valid(param->channel_plan))
	) {
		RTW_WARN("%s invalid chplan:0x%02X\n", caller, param->channel_plan);
		return _FAIL;
	}

	#if CONFIG_IEEE80211_BAND_6GHZ
	if (param->channel_plan_6g != RTW_CHPLAN_6G_UNSPECIFIED
		&& !rtw_is_channel_plan_6g_valid(param->channel_plan_6g)
	) {
		RTW_WARN("%s invalid chplan_6g:0x%02X\n", caller, param->channel_plan_6g);
		return _FAIL;
	}
	#endif

	/* use original value when unspecified */
	if (param->channel_plan == RTW_CHPLAN_UNSPECIFIED)
		param->channel_plan = rfctl->ChannelPlan;
	#if CONFIG_IEEE80211_BAND_6GHZ
	if (param->channel_plan_6g == RTW_CHPLAN_6G_UNSPECIFIED)
		param->channel_plan_6g = rfctl->chplan_6g;
	#endif

	return _SUCCESS;
}

static void rtw_chplan_rtk_priv_req_prehdl_country_ent(struct SetChannelPlan_param *param)
{
	if (!param->has_country) {
		u8 chplan_6g = RTW_CHPLAN_6G_NULL;

		#if CONFIG_IEEE80211_BAND_6GHZ
		chplan_6g = param->channel_plan_6g;
		#endif

		if (rtw_chplan_ids_is_world_wide(param->channel_plan, chplan_6g))
			rtw_get_chplan_worldwide(&param->country_ent);
		else {
			SET_UNSPEC_ALPHA2(param->country_ent.alpha2);
			param->country_ent.edcca_mode_2g_override = RTW_EDCCA_DEF;
			#if CONFIG_IEEE80211_BAND_5GHZ
			param->country_ent.edcca_mode_5g_override = RTW_EDCCA_DEF;
			#endif
			#if CONFIG_IEEE80211_BAND_6GHZ
			param->country_ent.edcca_mode_6g_override = RTW_EDCCA_DEF;
			#endif
			param->country_ent.txpwr_lmt_override = TXPWR_LMT_DEF;
			#if defined(CONFIG_80211AX_HE) || defined(CONFIG_80211AC_VHT)
			param->country_ent.proto_en = CHPLAN_PROTO_EN_ALL;
			#endif
		}
		param->has_country = 1;
	}
}

u8 rtw_set_chplan_hdl(_adapter *adapter, u8 *pbuf)
{
	struct SetChannelPlan_param *param;
	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);
#ifdef CONFIG_80211D
	bool effected = 1;
#endif /* CONFIG_80211D */
	char buf[32];
#ifdef CONFIG_IOCTL_CFG80211
	struct get_chplan_resp *chplan;
#endif

	if (!pbuf)
		return H2C_PARAMETERS_ERROR;

	param = (struct SetChannelPlan_param *)pbuf;

	get_str_of_set_chplan_keys(buf, 32, param);
	RTW_INFO("%s iface_id:%u src:%s inr:%s %s\n", __func__, adapter->iface_id
		, regd_src_str(param->regd_src), regd_inr_str(param->inr), buf);

	/* check input parameter */
	if (param->regd_src == REGD_SRC_RTK_PRIV) {
		#ifdef CONFIG_80211D
		effected = rtw_chplan_rtk_priv_req_prehdl_country_ie(adapter, param, __func__);
		#endif

		if (rtw_chplan_rtk_priv_req_prehdl_domain_code(adapter, param, __func__) != _SUCCESS)
			return H2C_PARAMETERS_ERROR;

		rtw_chplan_rtk_priv_req_prehdl_country_ent(param);
	}

	rtw_warn_on(!param->has_country);

	if (param->inr == RTW_REGD_SET_BY_USER) {
		rfctl->user_alpha2[0] = param->country_ent.alpha2[0];
		rfctl->user_alpha2[1] = param->country_ent.alpha2[1];
		rfctl->user_ChannelPlan = param->channel_plan;
		#if CONFIG_IEEE80211_BAND_6GHZ
		rfctl->user_chplan_6g = param->channel_plan_6g;
		#endif
	}

#ifdef CONFIG_80211D
	if (!effected)
		goto exit;
#endif /* CONFIG_80211D */

	rfctl->regd_src = param->regd_src;
	rfctl->regd_inr = param->inr;
	rfctl->alpha2[0] = param->country_ent.alpha2[0];
	rfctl->alpha2[1] = param->country_ent.alpha2[1];
	rfctl->edcca_mode_2g_override = param->country_ent.edcca_mode_2g_override;
#if CONFIG_IEEE80211_BAND_5GHZ
	rfctl->edcca_mode_5g_override = param->country_ent.edcca_mode_5g_override;
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	rfctl->edcca_mode_6g_override = param->country_ent.edcca_mode_6g_override;
#endif
#if CONFIG_TXPWR_LIMIT
	rfctl->txpwr_lmt_override = param->country_ent.txpwr_lmt_override;
#endif
#if defined(CONFIG_80211AX_HE) || defined(CONFIG_80211AC_VHT)
	rfctl->proto_en = param->country_ent.proto_en;
#endif
	rfctl->ChannelPlan = param->channel_plan;
#if CONFIG_IEEE80211_BAND_6GHZ
	rfctl->chplan_6g = param->channel_plan_6g;
#endif

#if CONFIG_TXPWR_LIMIT
	rtw_txpwr_update_cur_lmt_regs(dvobj);
#endif

	rtw_edcca_mode_update(rfctl_to_dvobj(rfctl));
	rtw_odm_adaptivity_update(rfctl_to_dvobj(rfctl));

	rtw_rfctl_chplan_init(adapter);

#ifdef CONFIG_IOCTL_CFG80211
	if (rtw_get_chplan_cmd(adapter, RTW_CMDF_DIRECTLY, &chplan) == _SUCCESS) {
		if (!param->rtnl_lock_needed)
			rtw_regd_change_complete_sync(adapter_to_wiphy(adapter), chplan, 0);
		else
			rtw_warn_on(rtw_regd_change_complete_async(adapter_to_wiphy(adapter), chplan) != _SUCCESS);
	} else
		rtw_warn_on(1);
#endif

	rtw_nlrtw_reg_change_event(adapter);

	#ifdef CONFIG_LPS
	LPS_Leave(adapter, "SET_CHPLAN");
	#endif

	if (rtw_txpwr_hal_get_pwr_lmt_en(dvobj) && rtw_hw_is_init_completed(dvobj))
		rtw_update_txpwr_level(dvobj, HW_BAND_MAX);

#ifdef CONFIG_80211D
exit:
#endif /* CONFIG_80211D */
	return	H2C_SUCCESS;
}

static u8 _rtw_set_chplan_cmd(_adapter *adapter, int flags
	, u8 chplan, u8 chplan_6g, const struct country_chplan *country_ent
	, enum regd_src_t regd_src, enum rtw_regd_inr inr
	, const struct country_ie_slave_record *cisr, u8 cisr_alink_id)
{
	struct cmd_obj *cmdobj;
	struct SetChannelPlan_param *parm;
	struct cmd_priv *pcmdpriv = &adapter->cmdpriv;
	struct submit_ctx sctx;
#ifdef PLATFORM_LINUX
	bool rtnl_lock_needed = rtw_rtnl_lock_needed(adapter_to_dvobj(adapter));
#endif
	u8 res = _SUCCESS;

	/* check if allow software config */
	if (rtw_rfctl_is_disable_sw_channel_plan(adapter_to_dvobj(adapter)) == _TRUE) {
		res = _FAIL;
		goto exit;
	}

	if (country_ent) {
		/* if country_entry is provided, replace chplan */
		chplan = country_ent->chplan;
		#if CONFIG_IEEE80211_BAND_6GHZ
		chplan_6g = country_ent->chplan_6g;
		#endif
	}

	/* prepare cmd parameter */
	parm = (struct SetChannelPlan_param *)rtw_zmalloc(sizeof(*parm));
	if (parm == NULL) {
		res = _FAIL;
		goto exit;
	}
	parm->regd_src = regd_src;
	parm->inr = inr;
	if (country_ent) {
		_rtw_memcpy(&parm->country_ent, country_ent, sizeof(parm->country_ent));
		parm->has_country = 1;
	}
	parm->channel_plan = chplan;
#if CONFIG_IEEE80211_BAND_6GHZ
	parm->channel_plan_6g = chplan_6g;
#endif
#ifdef CONFIG_80211D
	if (cisr) {
		_rtw_memcpy(&parm->cisr, cisr, sizeof(*cisr));
		parm->cisr_alink_id = cisr_alink_id;
		parm->has_cisr = 1;
	}
#endif
#ifdef PLATFORM_LINUX
	if (flags & (RTW_CMDF_DIRECTLY | RTW_CMDF_WAIT_ACK))
		parm->rtnl_lock_needed = rtnl_lock_needed; /* synchronous call, follow caller's */
	else
		parm->rtnl_lock_needed = 1; /* asynchronous call, always needed */
#endif

	if (flags & RTW_CMDF_DIRECTLY) {
		/* no need to enqueue, do the cmd hdl directly and free cmd parameter */
		if (H2C_SUCCESS != rtw_set_chplan_hdl(adapter, (u8 *)parm))
			res = _FAIL;
		rtw_mfree((u8 *)parm, sizeof(*parm));
	} else {
		/* need enqueue, prepare cmd_obj and enqueue */
		cmdobj = (struct cmd_obj *)rtw_zmalloc(sizeof(*cmdobj));
		if (cmdobj == NULL) {
			res = _FAIL;
			rtw_mfree((u8 *)parm, sizeof(*parm));
			goto exit;
		}

		init_h2fwcmd_w_parm_no_rsp(cmdobj, parm, CMD_SET_CHANPLAN);

		if (flags & RTW_CMDF_WAIT_ACK) {
			cmdobj->sctx = &sctx;
			rtw_sctx_init(&sctx, 2000);
		}

		res = rtw_enqueue_cmd(pcmdpriv, cmdobj);

		if (res == _SUCCESS && (flags & RTW_CMDF_WAIT_ACK)) {
			rtw_sctx_wait(&sctx, __func__);
			_rtw_mutex_lock_interruptible(&pcmdpriv->sctx_mutex);
			if (sctx.status == RTW_SCTX_SUBMITTED)
				cmdobj->sctx = NULL;
			_rtw_mutex_unlock(&pcmdpriv->sctx_mutex);
			if (sctx.status != RTW_SCTX_DONE_SUCCESS)
				res = _FAIL;
		}

		/* allow set channel plan when cmd_thread is not running */
		if (res != _SUCCESS && (flags & RTW_CMDF_WAIT_ACK)) {
			parm = (struct SetChannelPlan_param *)rtw_zmalloc(sizeof(*parm));
			if (parm == NULL) {
				res = _FAIL;
				goto exit;
			}
			parm->regd_src = regd_src;
			parm->inr = inr;
			if (country_ent) {
				_rtw_memcpy(&parm->country_ent, country_ent, sizeof(parm->country_ent));
				parm->has_country = 1;
			}
			parm->channel_plan = chplan;
			#if CONFIG_IEEE80211_BAND_6GHZ
			parm->channel_plan_6g = chplan_6g;
			#endif
			#ifdef CONFIG_80211D
			if (cisr) {
				_rtw_memcpy(&parm->cisr, cisr, sizeof(*cisr));
				parm->cisr_alink_id = cisr_alink_id;
				parm->has_cisr = 1;
			}
			#endif
			#ifdef PLATFORM_LINUX
			parm->rtnl_lock_needed = rtnl_lock_needed; /* synchronous call, follow caller's */
			#endif

			if (H2C_SUCCESS != rtw_set_chplan_hdl(adapter, (u8 *)parm))
				res = _FAIL;
			else
				res = _SUCCESS;
			rtw_mfree((u8 *)parm, sizeof(*parm));
		}
	}

exit:
	return res;
}

u8 rtw_set_chplan_cmd(_adapter *adapter, int flags, u8 chplan, u8 chplan_6g, enum rtw_regd_inr inr)
{
	return _rtw_set_chplan_cmd(adapter, flags, chplan, chplan_6g, NULL, REGD_SRC_RTK_PRIV, inr, NULL, RTW_RLINK_MAX);
}

u8 rtw_set_country_cmd(_adapter *adapter, int flags, const char *country_code, enum rtw_regd_inr inr)
{
	struct country_chplan ent;

	if (IS_ALPHA2_WORLDWIDE(country_code)) {
		rtw_get_chplan_worldwide(&ent);
		goto cmd;
	}

	if (is_alpha(country_code[0]) == _FALSE
	    || is_alpha(country_code[1]) == _FALSE
	   ) {
		RTW_PRINT("%s input country_code is not alpha2\n", __func__);
		return _FAIL;
	}

	if (!rtw_get_chplan_from_country(country_code, &ent)) {
		RTW_PRINT("%s unsupported country_code:\"%c%c\"\n", __func__, country_code[0], country_code[1]);
		return _FAIL;
	}

cmd:
	RTW_PRINT("%s country_code:\"%c%c\"\n", __func__, country_code[0], country_code[1]);

	return _rtw_set_chplan_cmd(adapter, flags, RTW_CHPLAN_UNSPECIFIED, RTW_CHPLAN_6G_UNSPECIFIED, &ent, REGD_SRC_RTK_PRIV, inr, NULL, RTW_RLINK_MAX);
}

#ifdef CONFIG_80211D
u8 rtw_alink_apply_recv_country_ie_cmd(struct _ADAPTER_LINK *alink, int flags, enum band_type band,u8 opch, const u8 *country_ie)
{
	struct country_ie_slave_record cisr;

	rtw_get_cisr_from_recv_country_ie(adapter_to_regsty(alink->adapter), band, opch, country_ie, &cisr, NULL);

	return _rtw_set_chplan_cmd(alink->adapter, flags, RTW_CHPLAN_UNSPECIFIED, RTW_CHPLAN_6G_UNSPECIFIED
		, NULL, REGD_SRC_RTK_PRIV, RTW_REGD_SET_BY_COUNTRY_IE, &cisr, rtw_adapter_link_get_id(alink));
}

u8 rtw_apply_recv_country_ie_cmd(_adapter *adapter, int flags, enum band_type band,u8 opch, const u8 *country_ie)
{
	struct country_ie_slave_record cisr;

	rtw_get_cisr_from_recv_country_ie(adapter_to_regsty(adapter), band, opch, country_ie, &cisr, NULL);

	return _rtw_set_chplan_cmd(adapter, flags, RTW_CHPLAN_UNSPECIFIED, RTW_CHPLAN_6G_UNSPECIFIED
		, NULL, REGD_SRC_RTK_PRIV, RTW_REGD_SET_BY_COUNTRY_IE, &cisr, RTW_RLINK_MAX);
}
#endif /* CONFIG_80211D */

#ifdef CONFIG_REGD_SRC_FROM_OS
u8 rtw_sync_os_regd_cmd(_adapter *adapter, int flags, const char *country_code, u8 dfs_region, enum rtw_regd_inr inr)
{
	struct country_chplan ent;
	struct country_chplan rtk_ent;
	bool rtk_ent_exist;

	rtk_ent_exist = rtw_get_chplan_from_country(country_code, &rtk_ent);

	_rtw_memcpy(ent.alpha2, country_code, 2);

	/*
	* Regulation follows OS, the internal txpwr limit selection is searched by alpha2
	*     "00" => WW, others use string mapping
	* When  no matching txpwr limit selection is found, use
	*     1. txpwr lmit selection associated with alpha2 inside driver regulation database
	*     2. WW when driver has no support of this alpha2
	*/

	ent.chplan = rtk_ent_exist ? rtk_ent.chplan : RTW_CHPLAN_UNSPECIFIED;
	#if CONFIG_IEEE80211_BAND_6GHZ
	ent.chplan_6g = rtk_ent_exist ? rtk_ent.chplan_6g : RTW_CHPLAN_6G_UNSPECIFIED;
	#endif
	ent.edcca_mode_2g_override = rtk_ent_exist ? rtk_ent.edcca_mode_2g_override : RTW_EDCCA_DEF;
	#if CONFIG_IEEE80211_BAND_5GHZ
	ent.edcca_mode_5g_override = rtk_ent_exist ? rtk_ent.edcca_mode_5g_override : RTW_EDCCA_DEF;
	#endif
	#if CONFIG_IEEE80211_BAND_6GHZ
	ent.edcca_mode_6g_override = rtk_ent_exist ? rtk_ent.edcca_mode_6g_override : RTW_EDCCA_DEF;
	#endif
	ent.txpwr_lmt_override = rtk_ent_exist ? rtk_ent.txpwr_lmt_override : TXPWR_LMT_DEF;
	#if defined(CONFIG_80211AC_VHT) || defined(CONFIG_80211AX_HE)
	ent.proto_en = CHPLAN_PROTO_EN_ALL;
	#endif

	/* TODO: dfs_region */

	return _rtw_set_chplan_cmd(adapter, flags, RTW_CHPLAN_UNSPECIFIED, RTW_CHPLAN_6G_UNSPECIFIED, &ent, REGD_SRC_OS, inr, NULL, RTW_RLINK_MAX);
}
#endif /* CONFIG_REGD_SRC_FROM_OS */

u8 rtw_get_chplan_hdl(_adapter *adapter, u8 *pbuf)
{
	struct get_channel_plan_param *param;
	struct get_chplan_resp *chplan;
	struct rf_ctl_t *rfctl;
	struct rtw_chset *chset;
#if CONFIG_TXPWR_LIMIT
	char *tl_reg_names[BAND_MAX];
	int tl_reg_names_len[BAND_MAX];
#endif
	int tl_reg_names_len_total = 0;
	int i;

	if (!pbuf)
		return H2C_PARAMETERS_ERROR;

	rfctl = adapter_to_rfctl(adapter);
	chset = adapter_to_chset(adapter);
	param = (struct get_channel_plan_param *)pbuf;

#if CONFIG_TXPWR_LIMIT
	for (i = 0; i < BAND_MAX; i++) {
		rtw_txpwr_hal_get_current_lmt_regs_name(adapter_to_dvobj(adapter), i, &tl_reg_names[i], &tl_reg_names_len[i]);
		tl_reg_names_len_total += tl_reg_names_len[i];
	}
#endif

	chplan = rtw_vmalloc(sizeof(struct get_chplan_resp) + sizeof(RT_CHANNEL_INFO) * chset->chs_len + tl_reg_names_len_total);
	if (!chplan)
		return H2C_CMD_FAIL;

	chplan->regd_src = rfctl->regd_src;
	chplan->regd_inr = rfctl->regd_inr;

	chplan->alpha2[0] = rfctl->alpha2[0];
	chplan->alpha2[1] = rfctl->alpha2[1];

	chplan->channel_plan = rfctl->ChannelPlan;
#if CONFIG_IEEE80211_BAND_6GHZ
	chplan->chplan_6g = rfctl->chplan_6g;
#endif
#if CONFIG_TXPWR_LIMIT
	chplan->txpwr_lmt_names_len_total = tl_reg_names_len_total;
	for (i = 0; i < BAND_MAX; i++) {
		if (i == 0)
			chplan->txpwr_lmt_names[i] = ((u8 *)(chplan->chs)) + sizeof(RT_CHANNEL_INFO) * chset->chs_len;
		else
			chplan->txpwr_lmt_names[i] = chplan->txpwr_lmt_names[i - 1] + chplan->txpwr_lmt_names_len[i - 1];

		if (tl_reg_names[i] && tl_reg_names_len[i]) {
			_rtw_memcpy((void *)chplan->txpwr_lmt_names[i], tl_reg_names[i], tl_reg_names_len[i]);
			chplan->txpwr_lmt_names_len[i] = tl_reg_names_len[i];
			rtw_mfree(tl_reg_names[i], tl_reg_names_len[i]);
		}
	}
#endif
	chplan->edcca_mode_2g = rfctl->edcca_mode_2g;
#if CONFIG_IEEE80211_BAND_5GHZ
	chplan->edcca_mode_5g = rfctl->edcca_mode_5g;
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	chplan->edcca_mode_6g = rfctl->edcca_mode_6g;
#endif
#ifdef CONFIG_DFS_MASTER
	chplan->dfs_domain = rtw_rfctl_get_dfs_domain(rfctl);
#endif

	chplan->proto_en = 0
		#if defined(CONFIG_80211AX_HE) || defined(CONFIG_80211AC_VHT)
		| rfctl->proto_en
		#endif
		;

	chplan->chs_len = chset->chs_len;
	_rtw_memcpy(chplan->chs, chset->chs, sizeof(RT_CHANNEL_INFO) * chset->chs_len);
	*param->chplan = chplan;

	return	H2C_SUCCESS;
}

u8 rtw_get_chplan_cmd(_adapter *adapter, int flags, struct get_chplan_resp **chplan)
{
	struct cmd_obj *cmdobj;
	struct get_channel_plan_param *parm;
	struct cmd_priv *pcmdpriv = &adapter->cmdpriv;
	struct submit_ctx sctx;
	u8 res = _FAIL;

	if (!(flags & (RTW_CMDF_DIRECTLY | RTW_CMDF_WAIT_ACK)))
		goto exit;

	/* prepare cmd parameter */
	parm = rtw_zmalloc(sizeof(*parm));
	if (parm == NULL)
		goto exit;
	parm->chplan = chplan;

	if (flags & RTW_CMDF_DIRECTLY) {
		/* no need to enqueue, do the cmd hdl directly and free cmd parameter */
		if (H2C_SUCCESS == rtw_get_chplan_hdl(adapter, (u8 *)parm))
			res = _SUCCESS;
		rtw_mfree((u8 *)parm, sizeof(*parm));
	} else {
		/* need enqueue, prepare cmd_obj and enqueue */
		cmdobj = (struct cmd_obj *)rtw_zmalloc(sizeof(*cmdobj));
		if (cmdobj == NULL) {
			rtw_mfree((u8 *)parm, sizeof(*parm));
			goto exit;
		}

		init_h2fwcmd_w_parm_no_rsp(cmdobj, parm, CMD_GET_CHANPLAN);

		if (flags & RTW_CMDF_WAIT_ACK) {
			cmdobj->sctx = &sctx;
			rtw_sctx_init(&sctx, 2000);
		}

		res = rtw_enqueue_cmd(pcmdpriv, cmdobj);

		if (res == _SUCCESS && (flags & RTW_CMDF_WAIT_ACK)) {
			rtw_sctx_wait(&sctx, __func__);
			_rtw_mutex_lock_interruptible(&pcmdpriv->sctx_mutex);
			if (sctx.status == RTW_SCTX_SUBMITTED)
				cmdobj->sctx = NULL;
			_rtw_mutex_unlock(&pcmdpriv->sctx_mutex);
			if (sctx.status != RTW_SCTX_DONE_SUCCESS)
				res = _FAIL;
		}

		/* allow get channel plan when cmd_thread is not running */
		if (res != _SUCCESS && (flags & RTW_CMDF_WAIT_ACK)) {
			parm = rtw_zmalloc(sizeof(*parm));
			if (parm == NULL)
				goto exit;
			parm->chplan = chplan;

			if (H2C_SUCCESS == rtw_get_chplan_hdl(adapter, (u8 *)parm))
				res = _SUCCESS;

			rtw_mfree((u8 *)parm, sizeof(*parm));
		}
	}

exit:
	return res;
}

void rtw_free_get_chplan_resp(struct get_chplan_resp *chplan)
{
	size_t sz = sizeof(struct get_chplan_resp) + sizeof(RT_CHANNEL_INFO) * chplan->chs_len
		#if CONFIG_TXPWR_LIMIT
		+ chplan->txpwr_lmt_names_len_total
		#endif
		;

	rtw_vmfree(chplan, sz);
}

#ifdef CONFIG_80211D
static bool rtw_iface_accept_country_ie(_adapter *adapter)
{
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);

	if (!(BIT(adapter->iface_id) & rfctl->cis_en_ifbmp))
		return false;
	if (!MLME_IS_STA(adapter))
		return false;
	if (!MLME_IS_GC(adapter)) {
		if (!(rfctl->cis_en_role & COUNTRY_IE_SLAVE_EN_ROLE_STA))
			return false;
	} else {
		if (!(rfctl->cis_en_role & COUNTRY_IE_SLAVE_EN_ROLE_GC))
			return false;
	}
	return true;
}

/* Return corresponding country_chplan setting  */
bool rtw_alink_joinbss_check_country_ie(struct _ADAPTER_LINK *alink, const WLAN_BSSID_EX *network, struct country_ie_slave_record *cisr, WLAN_BSSID_EX *out_network)
{
	_adapter *adapter = alink->adapter;
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);
	bool ret = 0;

	if (rfctl->regd_src == REGD_SRC_RTK_PRIV
		&& !rtw_rfctl_is_disable_sw_channel_plan(rfctl_to_dvobj(rfctl))
	) {
		u8 iface_id = adapter->iface_id;
		u8 alink_id = rtw_adapter_link_get_id(alink);
		const u8 *country_ie = NULL;
		sint country_ie_len = 0;

		if (rtw_iface_accept_country_ie(adapter)) {
			country_ie = rtw_get_ie(BSS_EX_TLV_IES(network)
				, WLAN_EID_COUNTRY, &country_ie_len, BSS_EX_TLV_IES_LEN(network));
			if (country_ie) {
				if (country_ie_len < 6) {
					country_ie = NULL;
					country_ie_len = 0;
				} else
					country_ie_len += 2;
			}
		}

		if (country_ie) {
			enum country_ie_slave_status status;

			rtw_buf_update(&rfctl->recv_country_ie[iface_id][alink_id]
				, &rfctl->recv_country_ie_len[iface_id][alink_id]
				, country_ie, country_ie_len);

			status = rtw_get_cisr_from_recv_country_ie(adapter_to_regsty(adapter)
				, BSS_EX_OP_BAND(network), BSS_EX_OP_CH(network), country_ie, cisr, __func__);
			if (status != COUNTRY_IE_SLAVE_NOCOUNTRY)
				ret = 1;

			if (out_network) {
				_rtw_memcpy(BSS_EX_IES(out_network) + BSS_EX_IES_LEN(out_network)
					, country_ie, country_ie_len);
				BSS_EX_IES_LEN(out_network) += country_ie_len;
			}
		} else {
			rtw_buf_free(&rfctl->recv_country_ie[iface_id][alink_id]
				, &rfctl->recv_country_ie_len[iface_id][alink_id]);
		}
	}

	return ret;
}

void rtw_alink_joinbss_update_regulatory(struct _ADAPTER_LINK *alink, const WLAN_BSSID_EX *network)
{
	_adapter *adapter = alink->adapter;
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);

	if (rfctl->regd_src == REGD_SRC_RTK_PRIV
		&& !rtw_rfctl_is_disable_sw_channel_plan(rfctl_to_dvobj(rfctl))
	) {
		u8 iface_id = adapter->iface_id;
		u8 alink_id = rtw_adapter_link_get_id(alink);
		const u8 *country_ie = NULL;
		sint country_ie_len = 0;

		if (network) {
			if (rtw_iface_accept_country_ie(adapter)) {
				country_ie = rtw_get_ie(BSS_EX_TLV_IES(network)
					, WLAN_EID_COUNTRY, &country_ie_len, BSS_EX_TLV_IES_LEN(network));
				if (country_ie) {
					if (country_ie_len < 6) {
						country_ie = NULL;
						country_ie_len = 0;
					} else
						country_ie_len += 2;
				}
			}
		}

		if (country_ie) {
			rtw_buf_update(&rfctl->recv_country_ie[iface_id][alink_id]
				, &rfctl->recv_country_ie_len[iface_id][alink_id], country_ie, country_ie_len);
			if (rtw_alink_apply_recv_country_ie_cmd(alink, RTW_CMDF_DIRECTLY
				, BSS_EX_OP_BAND(network), BSS_EX_OP_CH(network), country_ie) != _SUCCESS
			)
				RTW_WARN(FUNC_ADPT_FMT" id:%u rtw_alink_apply_recv_country_ie_cmd() fail\n", FUNC_ADPT_ARG(adapter), alink_id);
		} else {
			rtw_buf_free(&rfctl->recv_country_ie[iface_id][alink_id]
				, &rfctl->recv_country_ie_len[iface_id][alink_id]);
		}
	}
}

static void _rtw_alink_leavebss_update_regulatory(_adapter *adapter, u8 alink_id)
{
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);

	if (rfctl->regd_src == REGD_SRC_RTK_PRIV
		&& !rtw_rfctl_is_disable_sw_channel_plan(rfctl_to_dvobj(rfctl))
	) {
		if (alink_id < RTW_RLINK_MAX) {
			struct _ADAPTER_LINK * alink = GET_LINK(adapter, alink_id);

			if (rtw_alink_apply_recv_country_ie_cmd(alink, RTW_CMDF_DIRECTLY, 0, 0, NULL) != _SUCCESS)
				RTW_WARN(FUNC_ADPT_FMT" id:%u rtw_alink_apply_recv_country_ie_cmd() fail\n", FUNC_ADPT_ARG(adapter), alink_id);
		} else {
			if (rtw_apply_recv_country_ie_cmd(adapter, RTW_CMDF_DIRECTLY, 0, 0, NULL) != _SUCCESS)
				RTW_WARN(FUNC_ADPT_FMT" rtw_apply_recv_country_ie_cmd() fail\n", FUNC_ADPT_ARG(adapter));
		}
	}
}

void rtw_alink_leavebss_update_regulatory(struct _ADAPTER_LINK * alink)
{
	_rtw_alink_leavebss_update_regulatory(alink->adapter, rtw_adapter_link_get_id(alink));
}

void rtw_alink_csa_update_regulatory(struct _ADAPTER_LINK *alink, enum band_type req_band, u8 req_ch)
{
	_adapter *adapter = alink->adapter;
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);

	if (rfctl->regd_src == REGD_SRC_RTK_PRIV
		&& !rtw_rfctl_is_disable_sw_channel_plan(rfctl_to_dvobj(rfctl))
	) {
		u8 iface_id = adapter->iface_id;
		u8 alink_id = rtw_adapter_link_get_id(alink);

		if (rfctl->recv_country_ie[iface_id][alink_id]) {
			if (rtw_alink_apply_recv_country_ie_cmd(alink, RTW_CMDF_DIRECTLY
					, req_band, req_ch, rfctl->recv_country_ie[iface_id][alink_id]) != _SUCCESS)
				RTW_WARN(FUNC_ADPT_FMT" id:%u rtw_alink_apply_recv_country_ie_cmd() fail\n", FUNC_ADPT_ARG(adapter), alink_id);
		}
	}
}

void alink_process_country_ie(struct _ADAPTER_LINK *alink, u8 *ies, uint ies_len)
{
	_adapter *adapter = alink->adapter;
	struct rf_ctl_t *rfctl = adapter_to_rfctl(adapter);

	if (rfctl->regd_src == REGD_SRC_RTK_PRIV
		&& !rtw_rfctl_is_disable_sw_channel_plan(rfctl_to_dvobj(rfctl))
		&& !rfctl->csa_ch /* don't process country ie when under CSA processing */
	) {
		u8 iface_id = adapter->iface_id;
		u8 alink_id = rtw_adapter_link_get_id(alink);
		const u8 *ie = NULL;
		sint ie_len = 0;

		if (rtw_iface_accept_country_ie(adapter)) {
			ie = rtw_get_ie(ies, WLAN_EID_COUNTRY, &ie_len, ies_len);
			if (ie) {
				if (ie_len < 6) {
					ie = NULL;
					ie_len = 0;
				} else
					ie_len += 2;
			}
		}

		if (!rfctl->recv_country_ie[iface_id][alink_id] && !ie)
			return;
		if (rfctl->recv_country_ie_len[iface_id][alink_id] == ie_len
			&& _rtw_memcmp(rfctl->recv_country_ie[iface_id][alink_id], ie, ie_len) == _TRUE)
			return;

		if (!ie) {
			rtw_buf_free(&rfctl->recv_country_ie[iface_id][alink_id]
				, &rfctl->recv_country_ie_len[iface_id][alink_id]);
			rtw_alink_apply_recv_country_ie_cmd(alink, 0, 0, 0, NULL);
		} else {
			char ori_alpha2[2] = {0, 0};

			if (rfctl->recv_country_ie[iface_id][alink_id])
				_rtw_memcpy(ori_alpha2, rfctl->recv_country_ie[iface_id][alink_id] + 2, 2);

			rtw_buf_update(&rfctl->recv_country_ie[iface_id][alink_id]
				, &rfctl->recv_country_ie_len[iface_id][alink_id], ie, ie_len);
			/* for now only country code is used */
			if (_rtw_memcmp(ori_alpha2, rfctl->recv_country_ie[iface_id][alink_id] + 2, 2) == _TRUE)
				return;
			RTW_INFO(FUNC_ADPT_FMT" id:%u country_ie alpha2 changed\n", FUNC_ADPT_ARG(adapter), alink_id);
			rtw_alink_apply_recv_country_ie_cmd(alink, 0
				, ALINK_GET_BAND(alink), ALINK_GET_CH(alink), rfctl->recv_country_ie[iface_id][alink_id]);
		}
	}
}

bool rtw_joinbss_check_country_ie(_adapter *adapter, const WLAN_BSSID_EX *network, struct country_ie_slave_record *cisr, WLAN_BSSID_EX *out_network)
{
	return rtw_alink_joinbss_check_country_ie(GET_PRIMARY_LINK(adapter), network, cisr, out_network);
}

void rtw_joinbss_update_regulatory(_adapter *adapter, const WLAN_BSSID_EX *network)
{
	rtw_alink_joinbss_update_regulatory(GET_PRIMARY_LINK(adapter), network);
}

void rtw_leavebss_update_regulatory(_adapter *adapter)
{
	_rtw_alink_leavebss_update_regulatory(adapter, RTW_RLINK_MAX);
}

void rtw_csa_update_regulatory(_adapter *adapter, enum band_type req_band, u8 req_ch)
{
	rtw_alink_csa_update_regulatory(GET_PRIMARY_LINK(adapter), req_band, req_ch);
}

void process_country_ie(_adapter *adapter, u8 *ies, uint ies_len)
{
	alink_process_country_ie(GET_PRIMARY_LINK(adapter), ies, ies_len);
}

void rtw_rfctl_cis_init(struct rf_ctl_t *rfctl, struct registry_priv *regsty)
{
	rfctl->cis_en_role = regsty->country_ie_slave_en_role;
	rfctl->cis_en_ifbmp = regsty->country_ie_slave_en_ifbmp;
	rfctl->effected_cisr = NULL;
}

void rtw_rfctl_cis_deinit(struct rf_ctl_t *rfctl)
{
	int i, j;

	for (i = 0; i < CONFIG_IFACE_NUMBER; i++)
		for (j = 0; j < RTW_RLINK_MAX; j++)
			rtw_buf_free(&rfctl->recv_country_ie[i][j], &rfctl->recv_country_ie_len[i][j]);
}
#endif /* CONFIG_80211D */

#ifdef CONFIG_PROC_DEBUG
#if CONFIG_TXPWR_LIMIT
static void dump_chplan_txpwr_lmt_regs(void *sel, struct get_chplan_resp *chplan)
{
	int band;
	const char *names, *name;
	int names_len;

	for (band = 0; band < BAND_MAX; band++) {
		names = chplan->txpwr_lmt_names[band];
		names_len = chplan->txpwr_lmt_names_len[band];

		RTW_PRINT_SEL(sel, "txpwr_lmt[%s]:", band_str(band));
		ustrs_for_each_str(names, names_len, name)
			_RTW_PRINT_SEL(sel, "%s%s", name == names ? "" : " ", name);
		_RTW_PRINT_SEL(sel, "\n");
	}
}
#endif

static void dump_chplan_edcca_modes(void *sel, struct get_chplan_resp *chplan)
{
	u8 modes[BAND_MAX];
	char buf[EDCCA_MODES_STR_LEN];

	modes[BAND_ON_24G] = chplan->edcca_mode_2g;
#if CONFIG_IEEE80211_BAND_5GHZ
	modes[BAND_ON_5G] = chplan->edcca_mode_5g;
#endif
#if CONFIG_IEEE80211_BAND_6GHZ
	modes[BAND_ON_6G] = chplan->edcca_mode_6g;
#endif

	RTW_PRINT_SEL(sel, "edcca_mode:%s\n", rtw_get_edcca_modes_str(buf, modes));
}

void dump_cur_country(void *sel, struct rf_ctl_t *rfctl)
{
	struct dvobj_priv *dvobj = rfctl_to_dvobj(rfctl);
	struct get_chplan_resp *chplan;
	int i;

	if (rtw_get_chplan_cmd(dvobj_get_primary_adapter(dvobj), RTW_CMDF_WAIT_ACK, &chplan) == _FAIL)
		return;

	RTW_PRINT_SEL(sel, "%c%c\n", chplan->alpha2[0], chplan->alpha2[1]);

	rtw_free_get_chplan_resp(chplan);
}

void dump_cur_chplan(void *sel, struct rf_ctl_t *rfctl)
{
	struct dvobj_priv *dvobj = rfctl_to_dvobj(rfctl);
	struct registry_priv *regsty = dvobj_to_regsty(dvobj);
	struct get_chplan_resp *chplan;
	int i;

	if (rtw_get_chplan_cmd(dvobj_get_primary_adapter(dvobj), RTW_CMDF_WAIT_ACK, &chplan) == _FAIL)
		return;

	RTW_PRINT_SEL(sel, "regd_src:%s(%d)\n", regd_src_str(chplan->regd_src), chplan->regd_src);
	RTW_PRINT_SEL(sel, "regd_inr:%s\n", regd_inr_str(chplan->regd_inr));

	RTW_PRINT_SEL(sel, "alpha2:%c%c\n", chplan->alpha2[0], chplan->alpha2[1]);

#ifdef CONFIG_80211AX_HE
	RTW_PRINT_SEL(sel, "ax:%d\n", (chplan->proto_en & CHPLAN_PROTO_EN_AX) ? 1 : 0);
#endif
#ifdef CONFIG_80211AC_VHT
	RTW_PRINT_SEL(sel, "ac:%d\n", (chplan->proto_en & CHPLAN_PROTO_EN_AC) ? 1 : 0);
#endif

	if (chplan->channel_plan == RTW_CHPLAN_UNSPECIFIED)
		RTW_PRINT_SEL(sel, "chplan:NA\n");
	else
		RTW_PRINT_SEL(sel, "chplan:0x%02X\n", chplan->channel_plan);

#if CONFIG_IEEE80211_BAND_6GHZ
	if (chplan->chplan_6g == RTW_CHPLAN_6G_UNSPECIFIED)
		RTW_PRINT_SEL(sel, "chplan_6g:NA\n");
	else
		RTW_PRINT_SEL(sel, "chplan_6g:0x%02X\n", chplan->chplan_6g);
#endif

#if CONFIG_TXPWR_LIMIT
	dump_chplan_txpwr_lmt_regs(sel, chplan);
#endif

	dump_chplan_edcca_modes(sel, chplan);

#ifdef CONFIG_DFS_MASTER
	RTW_PRINT_SEL(sel, "dfs_domain:%s(%u)\n", rtw_dfs_regd_str(chplan->dfs_domain), chplan->dfs_domain);
#endif

	for (i = 0; i < MAX_CHANNEL_NUM_2G_5G; i++)
		if (regsty->excl_chs[i] != 0)
			break;

	if (i < MAX_CHANNEL_NUM_2G_5G) {
		RTW_PRINT_SEL(sel, "excl_chs:");
		for (i = 0; i < MAX_CHANNEL_NUM_2G_5G; i++) {
			if (regsty->excl_chs[i] == 0)
				break;
			_RTW_PRINT_SEL(sel, "%u ", regsty->excl_chs[i]);
		}
		_RTW_PRINT_SEL(sel, "\n");
	}

#if CONFIG_IEEE80211_BAND_6GHZ
	for (i = 0; i < MAX_CHANNEL_NUM_6G; i++)
		if (regsty->excl_chs_6g[i] != 0)
			break;

	if (i < MAX_CHANNEL_NUM_6G) {
		RTW_PRINT_SEL(sel, "excl_chs_6g:");
		for (i = 0; i < MAX_CHANNEL_NUM_6G; i++) {
			if (regsty->excl_chs_6g[i] == 0)
				break;
			_RTW_PRINT_SEL(sel, "%u ", regsty->excl_chs_6g[i]);
		}
		_RTW_PRINT_SEL(sel, "\n");
	}
#endif

	dump_chinfos(sel, chplan->chs, chplan->chs_len);

	rtw_free_get_chplan_resp(chplan);
}
#endif /* CONFIG_PROC_DEBUG */
