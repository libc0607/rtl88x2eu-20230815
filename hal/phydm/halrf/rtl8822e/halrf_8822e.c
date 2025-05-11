/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "mp_precomp.h"
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
#if RT_PLATFORM == PLATFORM_MACOSX
#include "phydm_precomp.h"
#else
#include "../phydm_precomp.h"
#endif
#else
#include "../../phydm_precomp.h"
#endif

#if (RTL8822E_SUPPORT == 1)
void halrf_rf_lna_setting_8822e(struct dm_struct *dm_void,
				enum halrf_lna_set type)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 path = 0x0;

	for (path = 0x0; path < 2; path++)
		if (type == HALRF_LNA_DISABLE) {
			/*S0*/
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(19),
				       0x1);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33,
				       RFREGOFFSETMASK, 0x00003);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3e,
				       RFREGOFFSETMASK, 0x00064);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3f,
				       RFREGOFFSETMASK, 0x0afce);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(19),
				       0x0);
		} else if (type == HALRF_LNA_ENABLE) {
			/*S0*/
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(19),
				       0x1);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x33,
				       RFREGOFFSETMASK, 0x00003);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3e,
				       RFREGOFFSETMASK, 0x00064);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x3f,
				       RFREGOFFSETMASK, 0x1afce);
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(19),
				       0x0);
		}
}

void odm_tx_pwr_track_set_pwr8822e(void *dm_void, enum pwrtrack_method method,
				   u8 rf_path, u8 channel_mapped_index)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &(dm->rf_calibrate_info);
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "pRF->absolute_ofdm_swing_idx=%d   pRF->remnant_ofdm_swing_idx=%d   pRF->absolute_cck_swing_idx=%d   pRF->remnant_cck_swing_idx=%d   rf_path=%d\n",
	       cali_info->absolute_ofdm_swing_idx[rf_path], cali_info->remnant_ofdm_swing_idx[rf_path], cali_info->absolute_cck_swing_idx[rf_path], cali_info->remnant_cck_swing_idx, rf_path);

	if (method == CLEAN_MODE) { /*use for mp driver clean power tracking status*/
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "===> %s method=%d clear power tracking rf_path=%d\n",
		       __func__, method, rf_path);
		tssi->tssi_trk_txagc_offset[rf_path] = 0;

		switch (rf_path) {
		case RF_PATH_A:
			odm_set_bb_reg(dm, R_0x18a0, 0x000000ff, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0xff));
			odm_set_rf_reg(dm, rf_path, RF_0x7f, 0x00002, 0x0);
			odm_set_rf_reg(dm, rf_path, RF_0x7f, 0x00100, 0x0);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x18a0,
			       odm_get_bb_reg(dm, R_0x18a0, 0x000000ff));
			break;
		case RF_PATH_B:
			odm_set_bb_reg(dm, R_0x41a0, 0x000000ff, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0xff));
			odm_set_rf_reg(dm, rf_path, RF_0x7f, 0x00002, 0x0);
			odm_set_rf_reg(dm, rf_path, RF_0x7f, 0x00100, 0x0);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x41a0,
			       odm_get_bb_reg(dm, R_0x41a0, 0x000000ff));
			break;
		default:
			break;
		}
	} else if (method == BBSWING) { /*use for mp driver clean power tracking status*/
		switch (rf_path) {
		case RF_PATH_A:
			odm_set_bb_reg(dm, R_0x18a0, 0x000000ff, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0xff));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x18a0, odm_get_bb_reg(dm, R_0x18a0, 0x000000ff));
			break;
		case RF_PATH_B:
			odm_set_bb_reg(dm, R_0x41a0, 0x000000ff, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0xff));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x41a0, odm_get_bb_reg(dm, R_0x41a0, 0x000000ff));
			break;
		default:
			break;
		}
	} else if (method == MIX_MODE) {
		switch (rf_path) {
		case RF_PATH_A:
			odm_set_bb_reg(dm, R_0x18a0, 0x000000ff, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0xff));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x18a0, odm_get_bb_reg(dm, R_0x18a0, 0x000000ff));
			break;
		case RF_PATH_B:
			odm_set_bb_reg(dm, R_0x41a0, 0x000000ff, (cali_info->absolute_ofdm_swing_idx[rf_path] & 0xff));
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "Path-%d 0x%x=0x%x\n", rf_path, R_0x41a0, odm_get_bb_reg(dm, R_0x41a0, 0x000000ff));
			break;
		default:
			break;
		}	
	}
}

void get_delta_swing_table_8822e(void *dm_void,
	u8 **temperature_up_a,
	u8 **temperature_down_a,
	u8 **temperature_up_b,
	u8 **temperature_down_b)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	u8 channel = *dm->channel;
	u8 tx_rate = phydm_get_tx_rate(dm);

	if (channel >= 1 && channel <= 14) {
		if (IS_CCK_RATE(tx_rate)) {
			*temperature_up_a = cali_info->delta_swing_table_idx_2g_cck_a_p;
			*temperature_down_a = cali_info->delta_swing_table_idx_2g_cck_a_n;
			*temperature_up_b = cali_info->delta_swing_table_idx_2g_cck_b_p;
			*temperature_down_b = cali_info->delta_swing_table_idx_2g_cck_b_n;
		} else {
			*temperature_up_a = cali_info->delta_swing_table_idx_2ga_p;
			*temperature_down_a = cali_info->delta_swing_table_idx_2ga_n;
			*temperature_up_b = cali_info->delta_swing_table_idx_2gb_p;
			*temperature_down_b = cali_info->delta_swing_table_idx_2gb_n;
		}
	} else if (channel >= 36 && channel <= 64) {
		*temperature_up_a = cali_info->delta_swing_table_idx_5ga_p[0];
		*temperature_down_a = cali_info->delta_swing_table_idx_5ga_n[0];
		*temperature_up_b = cali_info->delta_swing_table_idx_5gb_p[0];
		*temperature_down_b = cali_info->delta_swing_table_idx_5gb_n[0];
	} else if (channel >= 100 && channel <= 144) {
		*temperature_up_a = cali_info->delta_swing_table_idx_5ga_p[1];
		*temperature_down_a = cali_info->delta_swing_table_idx_5ga_n[1];
		*temperature_up_b = cali_info->delta_swing_table_idx_5gb_p[1];
		*temperature_down_b = cali_info->delta_swing_table_idx_5gb_n[1];
	} else { /*channel >= 149 && channel <= 177*/
		*temperature_up_a = cali_info->delta_swing_table_idx_5ga_p[2];
		*temperature_down_a = cali_info->delta_swing_table_idx_5ga_n[2];
		*temperature_up_b = cali_info->delta_swing_table_idx_5gb_p[2];
		*temperature_down_b = cali_info->delta_swing_table_idx_5gb_n[2];
	}
}

void _phy_aac_calibrate_8822e(struct dm_struct *dm)
{
	u32 cnt = 0;

	RF_DBG(dm, DBG_RF_LCK, "[AACK]AACK start!!!!!!!\n");
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xca, BIT(0), 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xca, BIT(0), 0x1);
	for (cnt = 0; cnt < 100; cnt++) {
		ODM_delay_ms(1);
		if (odm_get_rf_reg(dm, RF_PATH_A, RF_0xc9, BIT(5)) != 0x1)
			break;
	}
	RF_DBG(dm, DBG_RF_IQK, "[AACK]AACK end!!!!!!!\n");
}

void _phy_rt_calibrate_8822e(struct dm_struct *dm)
{
	u32 cnt = 0;

	RF_DBG(dm, DBG_RF_IQK, "[RTK]RTK start!!!!!!!\n");
	odm_set_rf_reg(dm, RF_PATH_A, 0xcc, BIT(18), 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, 0xcc, BIT(18), 0x1);
	for (cnt = 0; cnt < 100; cnt++) {
		ODM_delay_ms(1);
		if (odm_get_rf_reg(dm, RF_PATH_A, 0xce, BIT(11)) != 0x1)
			break;
	}
	odm_set_rf_reg(dm, RF_PATH_A, 0xcc, BIT(18), 0x0);
	RF_DBG(dm, DBG_RF_IQK, "[RTK]RTK end!!!!!!!\n");
}

void halrf_reload_bp_8822e(struct dm_struct *dm, u32 *bp_reg, u32 *bp)
{
	u32 i;

	for (i = 0; i < DACK_REG_8822E; i++)
		odm_write_4byte(dm, bp_reg[i], bp[i]);
}

void halrf_reload_bprf_8822e(struct dm_struct *dm, u32 *bp_reg, u32 bp[][2])
{
	u32 i;

	for (i = 0; i < DACK_RF_8822E; i++) {
		odm_set_rf_reg(dm, RF_PATH_A, bp_reg[i], MASK20BITS,
			       bp[i][RF_PATH_A]);
		odm_set_rf_reg(dm, RF_PATH_B, bp_reg[i], MASK20BITS,
			       bp[i][RF_PATH_B]);
	}
}

void halrf_bp_8822e(struct dm_struct *dm, u32 *bp_reg, u32 *bp)
{
	u32 i;

	for (i = 0; i < DACK_REG_8822E; i++)
		bp[i] = odm_read_4byte(dm, bp_reg[i]);
}

void halrf_bprf_8822e(struct dm_struct *dm, u32 *bp_reg, u32 bp[][2])
{
	u32 i;

	for (i = 0; i < DACK_RF_8822E; i++) {
		bp[i][RF_PATH_A] =
			odm_get_rf_reg(dm, RF_PATH_A, bp_reg[i], MASK20BITS);
		bp[i][RF_PATH_B] =
			odm_get_rf_reg(dm, RF_PATH_B, bp_reg[i], MASK20BITS);
	}
}


#if 1

#if 1
void halrf_dack_reset_8822e(struct dm_struct *dm)
{
	odm_set_bb_reg(dm, 0x1818, BIT(25), 0);
	odm_set_bb_reg(dm, 0x1818, BIT(25), 1);
	ODM_delay_ms(1);
	odm_set_bb_reg(dm, 0x4118, BIT(25), 0);
	odm_set_bb_reg(dm, 0x4118, BIT(25), 1);
	ODM_delay_ms(1);
}
#endif

boolean halrf_afereg_check_8822e(struct dm_struct *dm, u32 wa)
{
	boolean check_ok = true;

	if (odm_get_bb_reg(dm, wa, MASKDWORD) == 0x0)
		check_ok = false;
	else
		check_ok = true;
	if (!check_ok)
		RF_DBG(dm, DBG_RF_DACK, "[DACK]DAC FIFO reset IO race\n");
	return check_ok;
}

void halrf_write_check_afe_8822e(struct dm_struct *dm, u32 add, u32 data)
{
	u32 step = 1, count = 0;
	u32 wa, wd;

	if ((add == 0x3800) || (add == 0x3900))
		wd = data;
	else
		wd = 0xee32001f;

	if ((add >> 8) == 0x38)
		wa = 0x3800;
	else
		wa = 0x3900;

	while (count < 100) {
		count++;
		odm_set_bb_reg(dm, 0x2dd4, MASKDWORD, 0x0);
		odm_set_bb_reg(dm, add, MASKDWORD, data);
		odm_set_bb_reg(dm, add, MASKDWORD, data);
		odm_set_bb_reg(dm, 0x2dd4, MASKDWORD, 0x0);
		if (halrf_afereg_check_8822e(dm, wa)) {
			return;
		} else {
			odm_set_bb_reg(dm, wa, MASKDWORD, wd);
			odm_set_bb_reg(dm, wa, MASKDWORD, wd);
		}
	}
}

void halrf_dack_soft_rst_8822e(struct dm_struct *dm)
{
	halrf_write_check_afe_8822e(dm, 0x3800, 0xee30001f);
	halrf_write_check_afe_8822e(dm, 0x3800, 0xee32001f);
	halrf_write_check_afe_8822e(dm, 0x382c, 0xee30001f);
	halrf_write_check_afe_8822e(dm, 0x382c, 0xee32001f);
	halrf_write_check_afe_8822e(dm, 0x3900, 0xee30001f);
	halrf_write_check_afe_8822e(dm, 0x3900, 0xee32001f);
	halrf_write_check_afe_8822e(dm, 0x392c, 0xee30001f);
	halrf_write_check_afe_8822e(dm, 0x392c, 0xee32001f);
}

void halrf_dac_fifo_reset_8822e(struct dm_struct *dm)
{
	/*it needs DAC clock*/
	odm_set_bb_reg(dm, 0x3800, BIT(21), 0);
	odm_set_bb_reg(dm, 0x382c, BIT(21), 0);
	odm_set_bb_reg(dm, 0x3800, BIT(21), 1);
	odm_set_bb_reg(dm, 0x382c, BIT(21), 1);
	odm_set_bb_reg(dm, 0x3900, BIT(21), 0);
	odm_set_bb_reg(dm, 0x392c, BIT(21), 0);
	odm_set_bb_reg(dm, 0x3900, BIT(21), 1);
	odm_set_bb_reg(dm, 0x392c, BIT(21), 1);
}

void halrf_dac_fifo_rst_8822e(struct dm_struct *dm, u32 path, u32 iq)
{
	u32 add;

	if ((path == 0) && (iq == 0))
		add = 0x3800;
	else if ((path == 0) && (iq == 1))
		add = 0x382c;
	else if ((path == 1) && (iq == 0))
		add = 0x3900;
	else
		add = 0x392c;

	if (path == 0) {
		odm_set_bb_reg(dm, 0x1830, BIT(30), 0x0);
		odm_set_bb_reg(dm, 0x1860, BIT(30), 0x1);
		odm_set_bb_reg(dm, 0x1860, BIT(27), 0x0);
	} else {
		odm_set_bb_reg(dm, 0x4130, BIT(30), 0x0);
		odm_set_bb_reg(dm, 0x4160, BIT(30), 0x1);
		odm_set_bb_reg(dm, 0x4160, BIT(27), 0x0);
	}

	halrf_write_check_afe_8822e(dm, add, 0xee12001f);
	halrf_write_check_afe_8822e(dm, add, 0xee32001f);

	if (path == 0)
		odm_set_bb_reg(dm, 0x1830, BIT(30), 0x1);
	else
		odm_set_bb_reg(dm, 0x4130, BIT(30), 0x1);
}

void halrf_ex_dac_fifo_rst_8822e(struct dm_struct *dm)
{
#if 0
	u32 i;
	
	RF_DBG(dm, DBG_RF_DACK, "[DACK]DAC FIFO reset\n");

	i = 0;
	while ((i < 100) && (odm_get_bb_reg(dm, 0x3854, 0x3) != 0x0)) {
		halrf_dac_fifo_rst_8822e(dm, 0, 0);
		i++;
	}

	i = 0;
	while ((i < 100) && (odm_get_bb_reg(dm, 0x3884, 0x3) != 0x0)) {
		halrf_dac_fifo_rst_8822e(dm, 0, 1);
		i++;
	}

	i = 0;
	while ((i < 100) && (odm_get_bb_reg(dm, 0x3954, 0x3) != 0x0)) {
		halrf_dac_fifo_rst_8822e(dm, 1, 0);
		i++;
	}

	i = 0;
	while ((i < 100) && (odm_get_bb_reg(dm, 0x3984, 0x3) != 0x0)) {
		halrf_dac_fifo_rst_8822e(dm, 1, 1);
		i++;
	}
#endif
	halrf_dack_soft_rst_8822e(dm);
	
}
void halrf_dack_lps_bk_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u32 i, oft;

	for(i =0; i < 128; i++) {
		oft = i * 4;
		dack->afedig_d[i] = odm_get_bb_reg(dm, 0x3800 + oft, MASKDWORD);
	}
}

void halrf_dack_lps_reload_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u32 i, oft;

	for(i =0; i < 128; i++) {
		oft = i * 4;
		odm_set_bb_reg(dm, 0x3800 + oft, MASKDWORD, dack->afedig_d[i]);
	}
}

#if 0
void halrf_addck_backup_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;

	odm_set_bb_reg(dm, 0xc0f4, 0x300, 0x0);
	dack->addck_d[0][0] = (u16)odm_get_bb_reg(dm, 0xc0fc,0xffc00) ;
	dack->addck_d[0][1] = (u16)odm_get_bb_reg(dm, 0xc0fc,0x003ff) ;

	odm_set_bb_reg(dm, 0xc1f4, 0x300, 0x0);
	dack->addck_d[1][0] = (u16)odm_get_bb_reg(dm, 0xc1fc,0xffc00) ;
	dack->addck_d[1][1] = (u16)odm_get_bb_reg(dm, 0xc1fc,0x003ff) ;
}

void halrf_addck_reload_8822e(struct rf_info *rf)
{
	struct halrf_dack_info *dack = &rf->dack;
	/*S0*/
	halrf_wreg(rf, 0xc0f8, 0x0ffc0000, dack->addck_d[0][0]);
	halrf_wreg(rf, 0xc0f8, 0x0003ff00, dack->addck_d[0][1]);
	/*manual*/
	halrf_wreg(rf, 0xc0f8, 0x30000000, 0x3);
	/*S1*/
	halrf_wreg(rf, 0xc1f8, 0x0ffc0000, dack->addck_d[1][0]);
	halrf_wreg(rf, 0xc1f8, 0x0003ff00, dack->addck_d[1][1]);
	/*manual*/
	halrf_wreg(rf, 0xc1f8, 0x30000000, 0x3);
}
#endif

void halrf_wdack_8822e(struct dm_struct *dm, u32 reg_addr, u32 bit_mask, u32 data)
{
	odm_set_bb_reg(dm, reg_addr, bit_mask, data);
	odm_set_bb_reg(dm, reg_addr, bit_mask, data);
}

void halrf_dack_reload_by_path_8822e(struct dm_struct *dm, u8 path, u8 index)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u32 temp = 0 , temp_offset, temp_reg;
	u8 i;
	u32 idx_offset, path_offset;

	if (index ==0)
		idx_offset = 0;
	else
		idx_offset = 0x14;

	if (path == RF_PATH_A)
		path_offset = 0;
	else
		path_offset = 0x100;

	temp_offset = idx_offset + path_offset;

//	halrf_wreg(rf, 0xc004, BIT(17), 0x1);
//	halrf_wreg(rf, 0xc024, BIT(17), 0x1);
//	halrf_wreg(rf, 0xc104, BIT(17), 0x1);
//	halrf_wreg(rf, 0xc124, BIT(17), 0x1);

	/*new_msbk_d: 15/14/13/12*/
	temp = 0x0;
	for (i = 0; i < 4; i++) {
		temp |= dack->new_msbk_d[path][index][i+12] << (i * 8);
	}
	temp_reg = 0x38c0 + temp_offset;
	halrf_wdack_8822e(dm, temp_reg, MASKDWORD,temp);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x%x=0x%x\n", temp_reg,
		odm_get_bb_reg(dm, temp_reg, MASKDWORD));
	/*new_msbk_d: 11/10/9/8*/
	temp = 0x0;
	for (i = 0; i < 4; i++) {
		temp |= dack->new_msbk_d[path][index][i+8] << (i * 8);
	}
	temp_reg = 0x38c4 + temp_offset;
	halrf_wdack_8822e(dm, temp_reg, MASKDWORD, temp);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x%x=0x%x\n", temp_reg,
		odm_get_bb_reg(dm, temp_reg, MASKDWORD));
	/*new_msbk_d: 7/6/5/4*/
	temp = 0x0;
	for (i = 0; i < 4; i++) {
		temp |= dack->new_msbk_d[path][index][i+4] << (i * 8);
	}
	temp_reg = 0x38c8 + temp_offset;
	halrf_wdack_8822e(dm, temp_reg, MASKDWORD, temp);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x%x=0x%x\n", temp_reg,
		odm_get_bb_reg(dm, temp_reg, MASKDWORD));
	/*new_msbk_d: 3/2/1/0*/
	temp = 0x0;
	for (i = 0; i < 4; i++) {
		temp |= dack->new_msbk_d[path][index][i] << (i * 8);
	}
	temp_reg = 0x38cc + temp_offset;
	halrf_wdack_8822e(dm, temp_reg, MASKDWORD, temp);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x%x=0x%x\n", temp_reg,
		odm_get_bb_reg(dm,temp_reg, MASKDWORD));
	/*dadak_d/new_biask_d*/
	temp = 0x0;
	temp = (dack->new_biask_d[path] << 16) |
		(dack->dadck_d[path][index] << 8);
	temp_reg = 0x38d0 + temp_offset;
	halrf_wdack_8822e(dm, temp_reg, MASKDWORD, temp);
	/*enable DACK result from reg */
//	halrf_wdack_8822e(dm, temp_reg, BIT(0), 0x1);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x%x=0x%x\n", temp_reg,
		odm_get_bb_reg(dm,temp_reg, MASKDWORD));
}


void halrf_dack_reload_8822e(struct dm_struct *dm, enum rf_path path)
{
	u8 i;

	for (i = 0; i < 2; i++)
		halrf_dack_reload_by_path_8822e(dm, path, i);
	/*DACK result  1 : from reg_table ; 0 : from circuit Calibration*/
}

void halrf_dack_backup_s0_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u8 i;
	u32 temp;

//	halrf_wreg(rf, 0x12b8, BIT(30), 0x1);
	/*MSBK*/
	for (i = 0; i < 0x10; i++) {
		halrf_wdack_8822e(dm, 0x3800, 0x1e, i);
		dack->new_msbk_d[0][0][i] = (u8)odm_get_bb_reg(dm, 0x3870, 0xff000000);
		halrf_wdack_8822e(dm, 0x382c, 0x1e, i);
		dack->new_msbk_d[0][1][i] = (u8)odm_get_bb_reg(dm, 0x38a0, 0xff000000);
	}
#if 0
	for (i = 0; i < 0x10; i++) {
		halrf_wdack_8822e(dm, 0x3800, 0x1e, i);
		RF_DBG(dm, DBG_RF_DACK, "[DACK]MSB_S0_MDAC_OFS_SR_I[%d]=0x%x\n", i,
			odm_get_bb_reg(dm, 0x3860, 0x7fc0));
	}
	for (i = 0; i < 0x10; i++) {
		halrf_wdack_8822e(dm, 0x382c, 0x1e, i);
		RF_DBG(dm, DBG_RF_DACK, "[DACK]MSB_S0_MDAC_OFS_SR_Q[%d]=0x%x\n", i,
			odm_get_bb_reg(dm, 0x3890, 0x7fc0));
	}
#endif
	
	/*biasK*/
	dack->new_biask_d[0] = (u16)odm_get_bb_reg(dm, 0x3878, 0xffc00000);
	/*DADCK*/
	dack->dadck_d[0][0] = (u8)odm_get_bb_reg(dm, 0x3874, 0xff000000);
	dack->dadck_d[0][1] = (u8)odm_get_bb_reg(dm, 0x38a4, 0xff000000);
}

void halrf_dack_backup_s1_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u8 i;

//	halrf_wreg(rf, 0x32b8, BIT(30), 0x1);
	/*MSBK*/
	for (i = 0; i < 0x10; i++) {
		halrf_wdack_8822e(dm, 0x3900, 0x1e, i);
		dack->new_msbk_d[1][0][i] = (u8)odm_get_bb_reg(dm, 0x3970, 0xff000000);
		halrf_wdack_8822e(dm, 0x392c, 0x1e, i);
		dack->new_msbk_d[1][1][i] = (u8)odm_get_bb_reg(dm, 0x39a0, 0xff000000);
	}
	/*biasK*/
	dack->new_biask_d[1] = (u16)odm_get_bb_reg(dm, 0x3978, 0xffc00000);
	/*DADCK*/
	dack->dadck_d[1][0] = (u8)odm_get_bb_reg(dm, 0x3974, 0xff000000);
	dack->dadck_d[1][1] = (u8)odm_get_bb_reg(dm, 0x39a4, 0xff000000);
}

#define t_avg 100

#if 1
void halrf_check_addc_8822e(struct dm_struct *dm, enum rf_path path)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u32 c, offset;

	if (path == RF_PATH_A)
		offset = 0;
	else
		offset= 0x100;

	/*SAR ADC AVG SAMPLE*/
	halrf_wdack_8822e(dm, 0x381c+offset, 0x60000, 0x3);
	/*SAR ADC AVG Enable */
	halrf_wdack_8822e(dm, 0x381c+offset, BIT(16), 0x0);
	halrf_wdack_8822e(dm, 0x381c+offset, BIT(16), 0x1);
	/*write AFEDIG must even times*/
	halrf_wdack_8822e(dm, 0x381c+offset, BIT(16), 0x1);
	c = 0x0;
	while ((odm_get_bb_reg(dm, 0x3878+offset, BIT(12)) == 0) || (odm_get_bb_reg(dm, 0x38a8+offset, BIT(12)) == 0)) {
		c++;
		ODM_delay_us(1);
		if (c > 10000) {
			RF_DBG(dm, DBG_RF_DACK, "[DACK]check ADDC timeout\n");
			dack->addck_timeout[path] = true;
			break;
		}
	}
	RF_DBG(dm, DBG_RF_DACK, "[DACK]ADDCK c=%d\n", c);
	dack->addc[path][0] = (u16)odm_get_bb_reg(dm, 0x3878 + offset, 0xfff);
	dack->addc[path][1] = (u16)odm_get_bb_reg(dm, 0x38a8 + offset, 0xfff);
}

void halrf_addck_s0_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u32 ic, qc, temp;

	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 ADDCK\n");
	/*debug mode & clk setting*/
	odm_set_bb_reg(dm, 0x1830, BIT(30), 0x0);
	odm_set_bb_reg(dm, 0x1860, 0xf0000000, 0xf);
	odm_set_bb_reg(dm, 0x1860, BIT(26), 0x0);
	/*ADC input short*/
	odm_set_bb_reg(dm, 0x1860, BIT(12), 0x0);
	odm_set_bb_reg(dm, 0x1810, BIT(19), 0x1);
	halrf_check_addc_8822e(dm, RF_PATH_A);

	ic = 0x800 - dack->addc[0][0];
	qc = 0x800 - dack->addc[0][1];

	dack->addck_d[0][0]= (u16)ic;
	dack->addck_d[0][1]= (u16)qc;

	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 ADDC i=0x%x, q=0x%x\n", dack->addc[0][0], dack->addc[0][1]);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 ADDC ic=0x%x, qc=0x%x\n", dack->addck_d[0][0], dack->addck_d[0][1]);

	temp = (ic & 0x3ff) | ((qc & 0x3ff) << 10);
	odm_write_4byte(dm, 0x1868, temp);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 0x1868=0x%x\n", odm_get_bb_reg(dm, 0x1868, MASKDWORD));
	odm_set_bb_reg(dm, 0x1810, BIT(19), 0x0);
	odm_set_bb_reg(dm, 0x1860, BIT(12), 0x1);
	odm_set_bb_reg(dm, 0x1830, BIT(30), 0x1);
}

void halrf_addck_s1_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u32 ic, qc, temp;

	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 ADDCK\n");
	/*debug mode & clk setting*/
	odm_set_bb_reg(dm, 0x4130, BIT(30), 0x0);
	odm_set_bb_reg(dm, 0x4160, 0xf0000000, 0xf);
	odm_set_bb_reg(dm, 0x4160, BIT(26), 0x0);
	/*ADC input short*/
	odm_set_bb_reg(dm, 0x4160, BIT(12), 0x0);
	odm_set_bb_reg(dm, 0x4110, BIT(19), 0x1);
	halrf_check_addc_8822e(dm, RF_PATH_B);

	ic = 0x800 - dack->addc[1][0];
	qc = 0x800 - dack->addc[1][1];

	dack->addck_d[1][0]= (u16)ic;
	dack->addck_d[1][1]= (u16)qc;
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 ADDC i=0x%x, q=0x%x\n", dack->addc[1][0], dack->addc[1][1]);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 ADDC ic=0x%x, qc=0x%x\n", dack->addck_d[1][0], dack->addck_d[1][1]);

	temp = (ic & 0x3ff) | ((qc & 0x3ff) << 10);
	odm_write_4byte(dm, 0x4168, temp);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 0x4168=0x%x\n", odm_get_bb_reg(dm, 0x4168, MASKDWORD));
	odm_set_bb_reg(dm, 0x4110, BIT(19), 0x0);
	odm_set_bb_reg(dm, 0x4160, BIT(12), 0x1);
	odm_set_bb_reg(dm, 0x4130, BIT(30), 0x1);
}
#endif


#if 0
void halrf_check_dadc_8822e(struct rf_info *rf, enum rf_path path)
{
	halrf_wreg(rf, 0x032c, BIT(30), 0x0);
	halrf_wreg(rf, 0x030c, 0x0f000000, 0xf);
	halrf_wreg(rf, 0x030c, 0x0f000000, 0x3);
	halrf_wreg(rf, 0x032c, BIT(16), 0x0);
	if (path == RF_PATH_A) {
		halrf_wreg(rf, 0x12dc, BIT(0), 0x1);
		halrf_wreg(rf, 0x12e8, BIT(2), 0x1);
		halrf_wrf(rf, RF_PATH_A, 0x8f, BIT(13), 0x1);
	} else {
		halrf_wreg(rf, 0x32dc, BIT(0), 0x1);
		halrf_wreg(rf, 0x32e8, BIT(2), 0x1);
		halrf_wrf(rf, RF_PATH_B, 0x8f, BIT(13), 0x1);
	}
	halrf_check_addc_8822e(rf, path);
	if (path == RF_PATH_A) {
		halrf_wreg(rf, 0x12dc, BIT(0), 0x0);
		halrf_wreg(rf, 0x12e8, BIT(2), 0x0);
		halrf_wrf(rf, RF_PATH_A, 0x8f, BIT(13), 0x0);
	} else {
		halrf_wreg(rf, 0x32dc, BIT(0), 0x0);
		halrf_wreg(rf, 0x32e8, BIT(2), 0x0);
		halrf_wrf(rf, RF_PATH_B, 0x8f, BIT(13), 0x0);
	}
	halrf_wreg(rf, 0x032c, BIT(16), 0x1);
}
#endif
void halrf_dack_s0_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u32 c = 0, temp;

	temp = odm_get_bb_reg(dm, 0x9b4, MASKDWORD);
	/*step 1: Set clk to 160MHz for calibration  */
//	halrf_txck_force_8822e(rf, RF_PATH_A, true, DAC_160M);
	odm_set_bb_reg(dm, 0x9b4, 0x1ff00, 0xdb);

	/*step 2: DAC & clk enable */
	odm_set_bb_reg(dm, 0x1830, BIT(30), 0x0);
	odm_set_bb_reg(dm, 0x1860, BIT(30), 0x1);
	odm_set_bb_reg(dm, 0x1860, BIT(27), 0x0);
	/*step 3: enable comparator*/
	odm_set_bb_reg(dm, 0x1810, BIT(15), 0x1);
	/*step 4: set DAC gain for calibration mode */
	odm_set_bb_reg(dm, 0x1818, 0x0c000000, 0x3);
	/*step 6:Set MSB bias initial value */
	halrf_wdack_8822e(dm, 0x3804, 0x3ff00000, 0x58);
	halrf_wdack_8822e(dm, 0x3830, 0x3ff00000, 0x58);
	halrf_dac_fifo_reset_8822e(dm);
	/*step 8:Hold normal mode offset calibration for waiting DAC setting ready*/
	halrf_wdack_8822e(dm, 0x380c, BIT(1), 0x0);
	/*step 9:set auto mode*/
	halrf_wdack_8822e(dm, 0x3804, BIT(0), 0x1);

//	halrf_dack_reset_8822e(rf, RF_PATH_A);
	/*step10:Wait MSB calibration done */
	ODM_delay_us(1);
	c = 0x0;
	while ((odm_get_bb_reg(dm, 0x385c, BIT(1)) == 0) || (odm_get_bb_reg(dm, 0x388c, BIT(1)) == 0)) {
		c++;
		ODM_delay_us(1);
		if (c > 10000) {
			RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 MSBK timeout! i_ready=%d, q_ready=%d\n",
				odm_get_bb_reg(dm, 0x385c, BIT(1)),
				odm_get_bb_reg(dm, 0x388c, BIT(1)));
			dack->msbk_timeout[0] = true;
			break;
		}
	}
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 MSBK c = %d\n", c);
	/*step11: set DAC gain for normal mode*/
	odm_set_bb_reg(dm, 0x1818, 0x0c000000, 0x0);
	/*step12: Enable dc offset calibration*/
	halrf_wdack_8822e(dm, 0x380c, BIT(1), 0x1);
	/*step13: Wait offset calibration done*/
	ODM_delay_us(1);
	c = 0x0;
	while ((odm_get_bb_reg(dm, 0x3870, BIT(2)) == 0) || (odm_get_bb_reg(dm, 0x38a0, BIT(2)) == 0)) {
		c++;
		ODM_delay_us(1);
		if (c > 10000) {
			RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 DCK timeout! i_ready=%d, q_ready=%d\n",
				odm_get_bb_reg(dm, 0x3870, BIT(2)),
				odm_get_bb_reg(dm, 0x38a0, BIT(2)));
			dack->dadck_timeout[0] = true;
			break;
		}
	}
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 DCK c = %d\n", c);
	/*step 14: disable auto mode*/
	halrf_wdack_8822e(dm, 0x3804, BIT(0), 0x0);
	/*step 15: disable comparator*/
	odm_set_bb_reg(dm, 0x1810, BIT(15), 0x0);
	/*backup here*/
	halrf_dack_backup_s0_8822e(dm);
	halrf_dack_reload_8822e(dm, RF_PATH_A);


	/*step 16: Set clk to 960MHz for normal mode */
//	halrf_txck_force_8822e(rf, RF_PATH_A, false, DAC_960M);
	odm_set_bb_reg(dm, 0x9b4, MASKDWORD, temp);
	halrf_dac_fifo_reset_8822e(dm);
//	RF_DBG(dm, DBG_RF_DACK, "[DACK]after S0 DADCK\n");
	/*halrf_check_dadc_8822e(rf, RF_PATH_A);*/

	/*step 17: Set DAC & clk to normal mode */
	odm_set_bb_reg(dm, 0x1830, BIT(30), 0x1);
}

void halrf_dack_s1_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u32 c = 0, temp;;

	temp = odm_get_bb_reg(dm, 0x9b4, MASKDWORD);
	/*step 1: Set clk to 160MHz for calibration  */
//	halrf_txck_force_8822e(rf, RF_PATH_A, true, DAC_160M);
	odm_set_bb_reg(dm, 0x9b4, 0x1ff00, 0xdb);

	/*step 2: DAC & clk enable */
	odm_set_bb_reg(dm, 0x4130, BIT(30), 0x0);
	odm_set_bb_reg(dm, 0x4160, BIT(30), 0x1);
	odm_set_bb_reg(dm, 0x4160, BIT(27), 0x0);
	/*step 3: enable comparator*/
	odm_set_bb_reg(dm, 0x4110, BIT(15), 0x1);
	/*step 4: set DAC gain for calibration mode */
	odm_set_bb_reg(dm, 0x4118, 0x0c000000, 0x3);
	/*step 6:Set MSB bias initial value */
	halrf_wdack_8822e(dm, 0x3904, 0x3ff00000, 0x58);
	halrf_wdack_8822e(dm, 0x3930, 0x3ff00000, 0x58);
	halrf_dac_fifo_reset_8822e(dm);
	/*step 8:Hold normal mode offset calibration for waiting DAC setting ready*/
	halrf_wdack_8822e(dm, 0x390c, BIT(1), 0x0);
	/*step 9:set auto mode*/
	halrf_wdack_8822e(dm, 0x3904, BIT(0), 0x1);

//	halrf_dack_reset_8822e(rf, RF_PATH_A);
	/*step10:Wait MSB calibration done */
	ODM_delay_us(1);
	c = 0x0;
	while ((odm_get_bb_reg(dm, 0x395c, BIT(1)) == 0) || (odm_get_bb_reg(dm, 0x398c, BIT(1)) == 0)) {
		c++;
		ODM_delay_us(1);
		if (c > 10000) {
			RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 MSBK timeout! i_ready=%d, q_ready=%d\n",
				odm_get_bb_reg(dm, 0x395c, BIT(1)),
				odm_get_bb_reg(dm, 0x398c, BIT(1)));
			dack->msbk_timeout[1] = true;
			break;
		}
	}
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 MSBK c = %d\n", c);
	/*step11: set DAC gain for normal mode*/
	odm_set_bb_reg(dm, 0x4118, 0x0c000000, 0x0);
	/*step12: Enable dc offset calibration*/
	halrf_wdack_8822e(dm, 0x390c, BIT(1), 0x1);
	/*step13: Wait offset calibration done*/
	ODM_delay_us(1);
	c = 0x0;
	while ((odm_get_bb_reg(dm, 0x3970, BIT(2)) == 0) || (odm_get_bb_reg(dm, 0x39a0, BIT(2)) == 0)) {
		c++;
		ODM_delay_us(1);
		if (c > 10000) {
			RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 DCK timeout! i_ready=%d, q_ready=%d\n",
				odm_get_bb_reg(dm, 0x3970, BIT(2)),
				odm_get_bb_reg(dm, 0x39a0, BIT(2)));
			dack->dadck_timeout[1] = true;
			break;
		}
	}
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 DCK c = %d\n", c);
	/*step 14: disable auto mode*/
	halrf_wdack_8822e(dm, 0x3904, BIT(0), 0x0);
	/*step 15: disable comparator*/
	odm_set_bb_reg(dm, 0x4110, BIT(15), 0x0);
	/*backup here*/
	halrf_dack_backup_s1_8822e(dm);
	halrf_dack_reload_8822e(dm, RF_PATH_B);


	/*step 16: Set clk to 960MHz for normal mode */
//	halrf_txck_force_8822e(rf, RF_PATH_A, false, DAC_960M);
	odm_set_bb_reg(dm, 0x9b4, MASKDWORD, temp);
	halrf_dac_fifo_reset_8822e(dm);
//	RF_DBG(dm, DBG_RF_DACK, "[DACK]after S0 DADCK\n");
	/*halrf_check_dadc_8822e(rf, RF_PATH_A);*/

	/*step 17: Set DAC & clk to normal mode */
	odm_set_bb_reg(dm, 0x4130, BIT(30), 0x1);
}

void halrf_dack_8822e(struct dm_struct *dm)
{
	halrf_dack_s0_8822e(dm);
	halrf_dack_s1_8822e(dm);
}

void halrf_addck_8822e(struct dm_struct *dm)
{
	halrf_addck_s0_8822e(dm);
	halrf_addck_s1_8822e(dm);
}

void dack_info_by_8822e(
	void *dm_void,
	u32 *_used,
	char *output,
	u32 *_out_len)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dack_info *dack = &dm->dack_info;
	u8 i;
	u8 t;
	u32 used = *_used;
	u32 out_len = *_out_len;

	PDM_SNPF(out_len, used, output + used, out_len - used, "[DACK]S0 ADC_DCK ic = 0x%x, qc = 0x%x\n",
					dack->addck_d[0][0], dack->addck_d[0][1] );
	PDM_SNPF(out_len, used, output + used, out_len - used,"[DACK]S1 ADC_DCK ic = 0x%x, qc = 0x%x\n",
					dack->addck_d[1][0], dack->addck_d[1][1] );
	PDM_SNPF(out_len, used, output + used, out_len - used,  "[DACK]S0 DAC_DCK ic = 0x%x, qc = 0x%x\n",
		dack->dadck_d[0][0], dack->dadck_d[0][1] );
	PDM_SNPF(out_len, used, output + used, out_len - used,"[DACK]S1 DAC_DCK ic = 0x%x, qc = 0x%x\n",
		dack->dadck_d[1][0], dack->dadck_d[1][1] );

	PDM_SNPF(out_len, used, output + used, out_len - used, "[DACK]S0 biask = 0x%x\n",
	   dack->new_biask_d[0]);
	PDM_SNPF(out_len, used, output + used, out_len - used,"[DACK]S1 biask = 0x%x\n",
	   dack->new_biask_d[1]);

	PDM_SNPF(out_len, used, output + used, out_len - used,"[DACK]S0 MSBK ic:\n");
	for (i = 0; i < 0x10; i++) {
		t = dack->new_msbk_d[0][0][i];
		PDM_SNPF(out_len, used, output + used, out_len - used, "[DACK]0x%x\n", t);
	}
	PDM_SNPF(out_len, used, output + used, out_len - used,"[DACK]S0 MSBK qc:\n");
	for (i = 0; i < 0x10; i++) {
		t = dack->new_msbk_d[0][1][i];
	PDM_SNPF(out_len, used, output + used, out_len - used,"[DACK]0x%x\n", t);
	}
	PDM_SNPF(out_len, used, output + used, out_len - used,"[DACK]S1 MSBK ic:\n");
	for (i = 0; i < 0x10; i++) {
		t = dack->new_msbk_d[1][0][i];
	PDM_SNPF(out_len, used, output + used, out_len - used,  "[DACK]0x%x\n", t);
	}
	PDM_SNPF(out_len, used, output + used, out_len - used, "[DACK]S1 MSBK qc:\n");
	for (i = 0; i < 0x10; i++) {
		t = dack->new_msbk_d[1][1][i];
	PDM_SNPF(out_len, used, output + used, out_len - used, "[DACK]0x%x\n", t);
	}

	*_used = used;
	*_out_len = out_len;
}


void halrf_dack_dump_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	u8 i;
	u8 t;

	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 ADC_DCK ic = 0x%x, qc = 0x%x\n",
	                    dack->addck_d[0][0], dack->addck_d[0][1] );
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 ADC_DCK ic = 0x%x, qc = 0x%x\n",
	                    dack->addck_d[1][0], dack->addck_d[1][1] );
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 DAC_DCK ic = 0x%x, qc = 0x%x\n",
	       dack->dadck_d[0][0], dack->dadck_d[0][1] );
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 DAC_DCK ic = 0x%x, qc = 0x%x\n",
	       dack->dadck_d[1][0], dack->dadck_d[1][1] );

	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 biask = 0x%x\n",
	       dack->new_biask_d[0]);
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 biask = 0x%x\n",
	       dack->new_biask_d[1]);

	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 MSBK ic:\n");
	for (i = 0; i < 0x10; i++) {
		t = dack->new_msbk_d[0][0][i];
		RF_DBG(dm, DBG_RF_DACK, "[DACK]0x%x\n", t);
	}
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S0 MSBK qc:\n");
	for (i = 0; i < 0x10; i++) {
		t = dack->new_msbk_d[0][1][i];
		RF_DBG(dm, DBG_RF_DACK, "[DACK]0x%x\n", t);
	}
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 MSBK ic:\n");
	for (i = 0; i < 0x10; i++) {
		t = dack->new_msbk_d[1][0][i];
		RF_DBG(dm, DBG_RF_DACK, "[DACK]0x%x\n", t);
	}
	RF_DBG(dm, DBG_RF_DACK, "[DACK]S1 MSBK qc:\n");
	for (i = 0; i < 0x10; i++) {
		t = dack->new_msbk_d[1][1][i];
		RF_DBG(dm, DBG_RF_DACK, "[DACK]0x%x\n", t);
	}
}

void halrf_dack_val_8822e(struct dm_struct *dm, boolean from_reg)
{
	u32 kv;
	
	if (from_reg)
		kv = 0x1;
	else
		kv = 0x0;
	halrf_wdack_8822e(dm, 0x38d0, BIT(0), kv);
	halrf_wdack_8822e(dm, 0x38e4, BIT(0), kv);
	halrf_wdack_8822e(dm, 0x39d0, BIT(0), kv);
	halrf_wdack_8822e(dm, 0x39e4, BIT(0), kv);
}


boolean halrf_dack_checkfail_8822e(struct dm_struct *dm)
{
	boolean fail = false;
	
	if (odm_get_bb_reg(dm, 0x3800, BIT(17)) == 0)
		fail = true;
	if (odm_get_bb_reg(dm, 0x3900, BIT(17)) == 0)
		fail = true;	
	return fail;
}

#ifdef  HALRF_DZ_LOG
void halrf_dack_dz_rpt_8822e(struct dm_struct *dm)
{
	struct dm_dack_info *dack = &dm->dack_info;
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct halrf_rfk_dz_rpt *rfk_dz = &(rf->rfk_dz_rpt);

	if (dack->addck_timeout[0])
		rfk_dz->dack_dz_code |= DZ_ADDCK0_TIMEOUT;
	if (dack->addck_timeout[1])
		rfk_dz->dack_dz_code |= DZ_ADDCK1_TIMEOUT;	
	if (dack->dadck_timeout[0])
		rfk_dz->dack_dz_code |= DZ_DADCK0_TIMEOUT;
	if (dack->dadck_timeout[1])
		rfk_dz->dack_dz_code |= DZ_DADCK1_TIMEOUT;
	if (dack->msbk_timeout[0])
		rfk_dz->dack_dz_code |= DZ_MSBK0_TIMEOUT;
	if (dack->msbk_timeout[1])
		rfk_dz->dack_dz_code |= DZ_MSBK1_TIMEOUT;
}
#endif

void halrf_dac_cal_8822e(void *dm_void, boolean force)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dack_info *dack = &dm->dack_info;
	u8 i=0;
	
	dack->dack_done = false;
	RF_DBG(dm, DBG_RF_DACK, "[DACK]DACK 0x1\n");
	RF_DBG(dm, DBG_RF_DACK, "[DACK]DACK start!!!\n");	

#if 1
	halrf_addck_8822e(dm);
//	halrf_addck_backup_8822e(dm);
//	halrf_addck_reload_8822e(dm);

	halrf_dack_reset_8822e(dm);
	halrf_dack_val_8822e(dm, false);
	halrf_dack_8822e(dm);

	while (i < 10) {
		i++;
		if (halrf_dack_checkfail_8822e(dm)) {
			halrf_dack_reset_8822e(dm);
			halrf_dack_val_8822e(dm, false);
			halrf_dack_8822e(dm);
		} else {
			break;
		}
	}
	RF_DBG(dm, DBG_RF_DACK, "[DACK]i=%d\n", i);
	halrf_dack_val_8822e(dm, true);
	halrf_dack_dump_8822e(dm);
	halrf_dack_dz_rpt_8822e(dm);
//	halrf_dack_lps_bk_8822e(dm);
//	halrf_dack_reset_8822e(dm);
//	halrf_dack_lps_reload_8822e(dm);
	dack->dack_done = true;
#endif
	dack->dack_cnt++;
	RF_DBG(dm, DBG_RF_DACK, "[DACK]DACK finish!!!\n");
}

#if 0
void halrf_dack_dbg_8822e(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 i;
	u32 temp1, temp2, temp3;

	temp1 = odm_get_bb_reg(dm, 0x1860, MASKDWORD);
	temp2 = odm_get_bb_reg(dm, 0x4160, MASKDWORD);
	temp3 = odm_get_bb_reg(dm, 0x9b4, MASKDWORD);

	odm_set_bb_reg(dm, 0x9b4, MASKDWORD, 0xdb66db00);

	RF_DBG(dm, DBG_RF_DACK, "[DACK]MSBK result\n");
	RF_DBG(dm, DBG_RF_DACK, "[DACK]PATH A\n");	
	//pathA
	odm_set_bb_reg(dm, 0x1830, BIT(30), 0x0);
	odm_set_bb_reg(dm, 0x1860, 0xfc000000, 0x3c);
	//i
	for (i = 0; i < 0xf; i++) {
		odm_set_bb_reg(dm, 0x18b0, 0xf0000000, i);
		RF_DBG(dm, DBG_RF_DACK, "[DACK]msbk_d[0][0][%d]=0x%x\n", i,
		       odm_get_bb_reg(dm,0x2810,0x7fc0000));
	}
	//q
	for (i = 0; i < 0xf; i++) {
		odm_set_bb_reg(dm, 0x18cc, 0xf0000000, i);
		RF_DBG(dm, DBG_RF_DACK, "[DACK]msbk_d[0][1][%d]=0x%x\n", i,
		       odm_get_bb_reg(dm,0x283c,0x7fc0000));
	}
	//pathB
	RF_DBG(dm, DBG_RF_DACK, "[DACK]PATH A\n");
	odm_set_bb_reg(dm, 0x4130, BIT(30), 0x0);
	odm_set_bb_reg(dm, 0x4160, 0xfc000000, 0x3c);
	//i
	for (i = 0; i < 0xf; i++) {
		odm_set_bb_reg(dm, 0x41b0, 0xf0000000, i);
		RF_DBG(dm, DBG_RF_DACK, "[DACK]msbk_d[1][0][%d]=0x%x\n", i,
		       odm_get_bb_reg(dm,0x4510,0x7fc0000));
	}
	//q
	for (i = 0; i < 0xf; i++) {
		odm_set_bb_reg(dm, 0x41cc, 0xf0000000, i);
		RF_DBG(dm, DBG_RF_DACK, "[DACK]msbk_d[1][1][%d]=0x%x\n", i,
		       odm_get_bb_reg(dm,0x453c,0x7fc0000));
	}

	RF_DBG(dm, DBG_RF_DACK, "[DACK]DCK result\n");
	RF_DBG(dm, DBG_RF_DACK, "[DACK]PATH A\n");
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x18bc[31:28]=0x%x\n",
		       odm_get_bb_reg(dm,0x18bc,0xf0000000));
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x18c0[3:0]=0x%x\n",
		       odm_get_bb_reg(dm,0x18c0,0xf));
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x18d8[31:28]=0x%x\n",
		       odm_get_bb_reg(dm,0x18d8,0xf0000000));
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x18dc[3:0]=0x%x\n",
		       odm_get_bb_reg(dm,0x18dc,0xf));
	RF_DBG(dm, DBG_RF_DACK, "[DACK]PATH B\n");
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x41bc[31:28]=0x%x\n",
		       odm_get_bb_reg(dm,0x41bc,0xf0000000));
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x41c0[3:0]=0x%x\n",
		       odm_get_bb_reg(dm,0x41c0,0xf));
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x41d8[31:28]=0x%x\n",
		       odm_get_bb_reg(dm,0x41d8,0xf0000000));
	RF_DBG(dm, DBG_RF_DACK, "[DACK]0x41dc[3:0]=0x%x\n",
		       odm_get_bb_reg(dm,0x41dc,0xf));


	//restore to normal
	odm_set_bb_reg(dm, 0x1830, BIT(30), 0x1);
	odm_set_bb_reg(dm, 0x4130, BIT(30), 0x1);
	odm_set_bb_reg(dm, 0x1860, MASKDWORD, temp1);
	odm_set_bb_reg(dm, 0x4160, MASKDWORD, temp2);
	odm_set_bb_reg(dm, 0x9b4, MASKDWORD, temp3);
}
#endif
#endif

void halrf_tx_pause_8822e(struct dm_struct *dm)
{
	u8 reg_rf0_a, reg_rf0_b;
	u16 count = 0;

	odm_write_1byte(dm, R_0x522, 0xff);
	odm_set_bb_reg(dm, R_0x1e70, 0x0000000f, 0x2); /*hw tx stop*/

	reg_rf0_a = (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000);
	reg_rf0_b = (u8)odm_get_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000);

	while (((reg_rf0_a == 2) || (reg_rf0_b == 2)) && count < 2500) {
		reg_rf0_a = (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000);
		reg_rf0_b = (u8)odm_get_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000);
		ODM_delay_us(2);
		count++;
	}

	RF_DBG(dm, DBG_RF_RFK, "[RFK] Tx pause!!\n");
}

void halrf_set_rx_dck_8822e(struct dm_struct *dm, u8 path)
{
	u8 i;

	RF_DBG(dm, DBG_RF_RXDCK, "[RX_DCK] S%d RXDCK\n", path);

	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x93, BIT(6), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x1);

	for (i = 0; i < 75; i++)
		ODM_delay_us(20); /*delay 1.5ms*/

	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x93, BIT(6), 0x1);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x1);

	for (i = 0; i < 75; i++)
		ODM_delay_us(20); /*delay 1.5ms*/

	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x0);
}

void _rx_dck_information_8822e(struct dm_struct *dm)
{
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_rxdck_info *rx_dck = &(rf->halrf_rxdck_info);

	u32  reg_rf18;

	reg_rf18 = odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK);
	
	rx_dck->rxdck_band = (u8)((reg_rf18 & BIT(16)) >> 16); /*0/1:G/A*/
	rx_dck->rxdck_ch = (u8)(reg_rf18 & 0xff);
	rx_dck->rxdck_bw = (u8)((reg_rf18 & 0x3000) >> 12); /*3/2/1 :20/40/80*/

	RF_DBG(dm, DBG_RF_RXDCK, "[RX_DCK] Band/ CH/ BW = %s / %d / %s\n",
	       rx_dck->rxdck_band == 0 ? "2G" : "5G", rx_dck->rxdck_ch,
	       rx_dck->rxdck_bw == 3 ? "20M" : (rx_dck->rxdck_bw == 2 ? "40M" :
	       (rx_dck->rxdck_bw == 1 ? "80M" : "other BW")));
}

void halrf_rx_dck_8822e(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 path, reg_522;
	u32 reg_1e70, reg_180c, reg_410c, i;

	RF_DBG(dm, DBG_RF_RXDCK, "[RX_DCK] ****** RXDCK Start (Ver: %s, Cv: %d) ******\n",
		RXDCK_VER_8822E, dm->cut_version);

	_rx_dck_information_8822e(dm);

	reg_522 = odm_read_1byte(dm, R_0x522);
	reg_1e70 = odm_get_bb_reg(dm, R_0x1e70, MASKDWORD);
	reg_180c = odm_get_bb_reg(dm, R_0x180c, MASKDWORD);
	reg_410c = odm_get_bb_reg(dm, R_0x410c, MASKDWORD);

	halrf_tx_pause_8822e(dm);

	odm_set_bb_reg(dm, R_0x180c, 0x3, 0x0);
	odm_set_bb_reg(dm, R_0x410c, 0x3, 0x0);	

	btc_set_gnt_wl_bt_8822e(dm, true);

	for (path = 0; path < 2; path++) {
		odm_set_rf_reg(dm, (enum rf_path)path, 0x0, RFREG_MASK, 0x30000);

		halrf_set_rx_dck_8822e(dm, path);
	}

	btc_set_gnt_wl_bt_8822e(dm, false);

	odm_set_bb_reg(dm, R_0x180c, MASKDWORD, reg_180c );
	odm_set_bb_reg(dm, R_0x410c, MASKDWORD, reg_410c);
	/*toggle IGI*/
	odm_write_4byte(dm, R_0x1d70, 0x50505050);
	odm_write_4byte(dm, R_0x1d70, 0x20202020);
	odm_write_1byte(dm, R_0x522, reg_522); /*TMAC Tx restore*/
	odm_set_bb_reg(dm, R_0x1e70, MASKDWORD, reg_1e70); /*PMAC Tx restore*/
}

void halrf_rx_dck_enable_disable_8822e(void *dm_void, boolean is_enable)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (is_enable) {
		odm_set_rf_reg(dm, RF_PATH_A, 0xde, BIT(7) | BIT(6), 0x0);
		odm_set_rf_reg(dm, RF_PATH_A, 0xde, BIT(7) | BIT(6), 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, 0xde, BIT(7) | BIT(6), 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, 0xde, BIT(7) | BIT(6), 0x0);
	} else {
		odm_set_rf_reg(dm, RF_PATH_A, 0xde, BIT(7) | BIT(6), 0x3);
		odm_set_rf_reg(dm, RF_PATH_A, 0xde, BIT(7) | BIT(6), 0x3);
		odm_set_rf_reg(dm, RF_PATH_B, 0xde, BIT(7) | BIT(6), 0x3);
		odm_set_rf_reg(dm, RF_PATH_B, 0xde, BIT(7) | BIT(6), 0x3);
		odm_set_rf_reg(dm, RF_PATH_A, 0x92, 0xFFF80, 0x1090); /*RXBB_I [19:7]*/
		odm_set_rf_reg(dm, RF_PATH_A, 0x93, 0xFFF80, 0x1090); /*RXBB_Q [19:7]*/
		odm_set_rf_reg(dm, RF_PATH_B, 0x92, 0xFFF80, 0x1090); /*RXBB_I [19:7]*/
		odm_set_rf_reg(dm, RF_PATH_B, 0x93, 0xFFF80, 0x1090); /*RXBB_Q [19:7]*/
	}
	RF_DBG(dm, DBG_RF_RXDCK, "[RX_DCK] RXDCK is %s\n", is_enable ? "enabled" : "disabled");
}

void halrf_rx_dck_dbg_info_8822e(void *dm_void, u32 *_used, char *output, u32 *_out_len)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	
	u32 used = *_used;
	u32 out_len = *_out_len;
	u32 reg_180c, reg_410c, rx_gain;
	u8 path, rxbb;

	reg_180c = odm_get_bb_reg(dm, R_0x180c, MASKDWORD);
	reg_410c = odm_get_bb_reg(dm, R_0x410c, MASKDWORD);

	odm_set_bb_reg(dm, R_0x180c, 0x3, 0x0);
	odm_set_bb_reg(dm, R_0x410c, 0x3, 0x0);
		
	for (path = 0; path < 2; path++) {

		PDM_SNPF(out_len, used, output + used, out_len - used,"\n---------------[ S%d DCK Value ]---------------\n", path);
		PDM_SNPF(out_len, used, output + used, out_len - used,
			 " RF0x00 | S1_I / S2_I / S3_I / S1_Q / S2_Q / S3_Q\n");

			for (rxbb = 0; rxbb < 32; rxbb++) {
				rx_gain = 0x30000 | (rxbb << 5);
				odm_set_rf_reg(dm, (enum rf_path)path, 0x00, RFREG_MASK, rx_gain);

				PDM_SNPF(out_len, used, output + used, out_len - used,
				    "0x%05x | 0x%02x / 0x%02x / 0x%02x / 0x%02x / 0x%02x / 0x%02x\n", rx_gain,
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x92, 0xF8000),  /*[19:15]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x92, 0x07000),  /*[14:12]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x92, 0x00F80),  /*[11:7]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x93, 0xF8000),  /*[19:15]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x93, 0x07000),  /*[14:12]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x93, 0x00F80));  /*[11:7]*/
			}
	}
	odm_set_bb_reg(dm, R_0x180c, MASKDWORD, reg_180c );
	odm_set_bb_reg(dm, R_0x410c, MASKDWORD, reg_410c);
}

void _phy_x2_calibrate_8822e(struct dm_struct *dm)
{
	RF_DBG(dm, DBG_RF_IQK, "[X2K]X2K start!!!!!!!\n");
	/*X2K*/
	//Path A
	odm_set_rf_reg(dm, RF_PATH_A, 0x18, RFREGOFFSETMASK, 0x13108);
	ODM_delay_ms(1);
	odm_set_rf_reg(dm, RF_PATH_A, 0xb8, RFREGOFFSETMASK, 0xC0440);	
	odm_set_rf_reg(dm, RF_PATH_A, 0xba, RFREGOFFSETMASK, 0xE840D);
	ODM_delay_ms(1);
	odm_set_rf_reg(dm, RF_PATH_A, 0x18, RFREGOFFSETMASK, 0x13124);
	//Path B
	// SYN is in the path A
	RF_DBG(dm, DBG_RF_IQK, "[X2K]X2K end!!!!!!!\n");
}

#ifdef HALRF_DZ_LOG
void halrf_ex_rx_dck_dbg_info_8822e(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 reg_180c, reg_410c, rx_gain;
	u8 path, rxbb;

	reg_180c = odm_get_bb_reg(dm, R_0x180c, MASKDWORD);
	reg_410c = odm_get_bb_reg(dm, R_0x410c, MASKDWORD);

	odm_set_bb_reg(dm, R_0x180c, 0x3, 0x0);
	odm_set_bb_reg(dm, R_0x410c, 0x3, 0x0);
#if 1
	for (path = 0; path < 2; path++) {

	RF_DBG(dm, DBG_RF_DZ_LOG,
			 "\n---------------[ S%d DCK Value ]---------------\n", path);
		

		RF_DBG(dm, DBG_RF_DZ_LOG,
			 " RF0x00 | S1_I / S2_I / S3_I / S1_Q / S2_Q / S3_Q\n");

			for (rxbb = 0; rxbb < 32; rxbb++) {
				rx_gain = 0x30000 | (rxbb << 5);
				odm_set_rf_reg(dm, (enum rf_path)path, 0x00, RFREG_MASK, rx_gain);

				RF_DBG(dm, DBG_RF_DZ_LOG,
				    "0x%05x | 0x%02x / 0x%02x / 0x%02x / 0x%02x / 0x%02x / 0x%02x\n", rx_gain,
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x92, 0xF8000),  /*[19:15]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x92, 0x07000),  /*[14:12]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x92, 0x00F80),  /*[11:7]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x93, 0xF8000),  /*[19:15]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x93, 0x07000),  /*[14:12]*/
				    odm_get_rf_reg(dm, (enum rf_path)path, 0x93, 0x00F80));  /*[11:7]*/
			}
	}
	odm_set_bb_reg(dm, R_0x180c, MASKDWORD, reg_180c );
	odm_set_bb_reg(dm, R_0x410c, MASKDWORD, reg_410c);
#endif
}
#endif

void phy_x2_check_8822e(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 X2K_BUSY;

	RF_DBG(dm, DBG_RF_IQK, "[X2K]X2K check start!!!!!!!\n");
	/*X2K*/
	//Path A
	ODM_delay_ms(1);
	X2K_BUSY = (u8) odm_get_rf_reg(dm, RF_PATH_A, 0xb8, BIT(15));
	if (X2K_BUSY == 1) {
		odm_set_rf_reg(dm, RF_PATH_A, 0xb8, RFREGOFFSETMASK, 0xC4440);	
		odm_set_rf_reg(dm, RF_PATH_A, 0xba, RFREGOFFSETMASK, 0x6840D);
		odm_set_rf_reg(dm, RF_PATH_A, 0xb8, RFREGOFFSETMASK, 0x80440);		
		ODM_delay_ms(1);
	}
	//Path B
	// SYN is in the path A
	RF_DBG(dm, DBG_RF_IQK, "[X2K]X2K check end!!!!!!!\n");
}

/*LCK VERSION:0x1*/
void phy_lc_calibrate_8822e(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
#if 1
	_phy_aac_calibrate_8822e(dm);
	_phy_rt_calibrate_8822e(dm);
#endif
}

void configure_txpower_track_8822e(struct txpwrtrack_cfg *config)
{
	config->swing_table_size_cck = TXSCALE_TABLE_SIZE;
	config->swing_table_size_ofdm = TXSCALE_TABLE_SIZE;
	config->threshold_iqk = IQK_THRESHOLD;
	config->threshold_dpk = DPK_THRESHOLD;
	config->average_thermal_num = AVG_THERMAL_NUM_8822E;
	config->rf_path_count = MAX_PATH_NUM_8822E;
	config->thermal_reg_addr = RF_T_METER_8822E;

	config->odm_tx_pwr_track_set_pwr = odm_tx_pwr_track_set_pwr8822e;
	config->do_iqk = do_iqk_8822e;
	config->phy_lc_calibrate = halrf_lck_trigger;
	//config->do_tssi_dck = halrf_tssi_dck;
	config->get_delta_swing_table = get_delta_swing_table_8822e;
}

void halrf_kip_rsvd_page_8822e(
	void *dm_void,
	u8 *buf,
	u32 *buf_size)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 array_len;
#if 0
	u32 c = 0;

	while (c < 10000) {
		c++;
		if (array_mp_8822e_cal_init[c * 2] == 0x1b80)
			break;
	}

	array_len = (c * 2) * 4;
#endif
	array_len =  sizeof(array_mp_8822e_cal_init);

	RF_DBG(dm, DBG_RF_RFK, "[KIP]array_len =%d !\n", array_len);

	if (buf) {
		odm_move_memory(dm, buf, &array_len , 4);
		odm_move_memory(dm, buf + 4, array_mp_8822e_cal_init, array_len);
	}
	if (buf_size)
		*buf_size = array_len + 4;
}

#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
void phy_set_rf_path_switch_8822e(struct dm_struct *dm, boolean is_main)
#else
void phy_set_rf_path_switch_8822e(void *adapter, boolean is_main)
#endif
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(((PADAPTER)adapter));
	struct dm_struct *dm = &hal_data->DM_OutSrc;
#endif
#endif
	/*BY mida Request */
	if (is_main) {
		/*WiFi*/
		odm_set_bb_reg(dm, R_0x70, BIT(26), 0x1);
	} else {
		/*BT*/
		odm_set_bb_reg(dm, R_0x70, BIT(26), 0x0);
	}
}

#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
boolean _phy_query_rf_path_switch_8822e(struct dm_struct *dm)
#else
boolean _phy_query_rf_path_switch_8822e(void *adapter)
#endif
{
#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(((PADAPTER)adapter));
	struct dm_struct *dm = &hal_data->DM_OutSrc;
#endif
#endif
	if (odm_get_bb_reg(dm, R_0x70, BIT(26)) == 0x1)
		return true;	/*WiFi*/
	else
		return false;
}

#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
boolean phy_query_rf_path_switch_8822e(struct dm_struct *dm)
#else
boolean phy_query_rf_path_switch_8822e(void *adapter)
#endif
{
#if DISABLE_BB_RF
	return true;
#endif
#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
	return _phy_query_rf_path_switch_8822e(dm);
#else
	return _phy_query_rf_path_switch_8822e(adapter);
#endif
}

void halrf_rxbb_dc_cal_8822e(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	u8 path, i;

	for (path = 0; path < 2; path++) {
		odm_set_rf_reg(dm, (enum rf_path)path, 0x92, RFREG_MASK, 0x84800);
		ODM_delay_us(5);
		odm_set_rf_reg(dm, (enum rf_path)path, 0x92, RFREG_MASK, 0x84801);
		for (i = 0; i < 30; i++) /*delay 600us*/
			ODM_delay_us(20);
		odm_set_rf_reg(dm, (enum rf_path)path, 0x92, RFREG_MASK, 0x84800);
	}
}

void halrf_rfk_handshake_8822e(void *dm_void, boolean is_before_k)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	void *padapter = dm->adapter;
	u8 u1b_tmp, h2c_parameter;
	u16 count;
	u8 rfk_type = rf->rfk_type;
	u8 path;
	u32 i;

	rf->is_rfk_h2c_timeout = false;

#if ((DM_ODM_SUPPORT_TYPE & ODM_AP) || (DM_ODM_SUPPORT_TYPE == ODM_CE))
		path = phy_query_rf_path_switch_8822e(dm);
#else
		path = phy_query_rf_path_switch_8822e(padapter);
#endif

	if (is_before_k) {

		RF_DBG(dm, DBG_RF_IQK | DBG_RF_DPK | DBG_RF_TX_PWR_TRACK,
		       "[RFK] WiFi / BT RFK handshake start!!\n");

		if (!rf->is_bt_iqk_timeout) {
			/* Check if BT request to do IQK (0xaa[6]) or is doing IQK (0xaa[5]), 600ms timeout*/
			count = 0;
			u1b_tmp = (u8)odm_get_mac_reg(dm, 0xa8, BIT(22) | BIT(21));
			while (u1b_tmp != 0 && count < 30000) {
				ODM_delay_us(20);
				u1b_tmp = (u8)odm_get_mac_reg(dm, 0xa8, BIT(22) | BIT(21));
				count++;
			}

			if (count >= 30000) {
				RF_DBG(dm, DBG_RF_IQK | DBG_RF_DPK | DBG_RF_TX_PWR_TRACK,
				       "[RFK] Wait BT IQK finish timeout!!\n");

				rf->is_bt_iqk_timeout = true;
			}
		}

		/* Send RFK start H2C cmd*/
		h2c_parameter = 1;
		odm_fill_h2c_cmd(dm, ODM_H2C_WIFI_CALIBRATION, 1, &h2c_parameter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE) && (CONFIG_BTCOEX_SUPPORT_BTC_CMN == 1)		
		hal_btcoex_WLRFKNotify(padapter, path, rfk_type, 0);
	#endif		
		for (i = 0; i < 1000; i++)
			ODM_delay_us(5);
		//ODM_delay_us(20);	
	} else {
		/* Send RFK finish H2C cmd*/
		h2c_parameter = 0;
		odm_fill_h2c_cmd(dm, ODM_H2C_WIFI_CALIBRATION, 1, &h2c_parameter);
#if (DM_ODM_SUPPORT_TYPE == ODM_CE) && (CONFIG_BTCOEX_SUPPORT_BTC_CMN == 1)		
		hal_btcoex_WLRFKNotify(padapter, path, rfk_type, 1);
#endif		
		ODM_delay_us(20);
		RF_DBG(dm, DBG_RF_IQK | DBG_RF_DPK | DBG_RF_TX_PWR_TRACK,
		       "[RFK] WiFi / BT RFK handshake finish!!\n");
	}
}

void halrf_rfk_power_save_8822e(
	void *dm_void,
	boolean is_power_save)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 path  = 0;

	for(path = 0; path < SS_8822E; path++) {
		odm_set_bb_reg(dm, R_0x1b00, BIT(2)| BIT(1), path);
		if (is_power_save)
			odm_set_bb_reg(dm, R_0x1b08, BIT(7), 0x0);
		else
			odm_set_bb_reg(dm, R_0x1b08, BIT(7), 0x1);
		}
}

u8 halrf_get_thermal_8822e(
	void *dm_void,
	u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x42, BIT(19), 0x1);
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x42, BIT(19), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x42, BIT(19), 0x1);
	ODM_delay_us(15);

	return (u8)odm_get_rf_reg(dm, (enum rf_path)path, RF_0x42, 0x0007e);
}

void halrf_lck_track_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &(dm->rf_calibrate_info);
	struct _hal_rf_ *rf = &dm->rf_table;

	u8 channel = *dm->channel;
	u8 i = 0, path;
	u8 thermal_lck_avg_count = 0, thermal_value[2] = {0};
	u32 thermal_lck_avg[2] = {0};
	s8 delta_lck[2];

	if ((cali_info->thermal_lck[RF_PATH_A] == 0) && (cali_info->thermal_lck[RF_PATH_B] == 0)) {
		cali_info->thermal_lck[RF_PATH_A] = halrf_get_thermal_8822e(dm, RF_PATH_A);
		cali_info->thermal_lck[RF_PATH_B] = halrf_get_thermal_8822e(dm, RF_PATH_B);
		RF_DBG(dm, DBG_RF_LCK, "[LCK_track] Updata LCK Base thermal A/B=%d/%d\n",
			cali_info->thermal_lck[RF_PATH_A], cali_info->thermal_lck[RF_PATH_B]);
	} else
		RF_DBG(dm, DBG_RF_LCK,
		       "[LCK_track] ================[CH %d]================\n", channel);

	/*get thermal meter*/
	for (path = 0; path < MAX_PATH_NUM_8822E; path++) {
		thermal_value[path] = halrf_get_thermal_8822e(dm, path);

		RF_DBG(dm, DBG_RF_LCK,
		       "[LCK_track] S%d thermal now = %d\n", path, thermal_value[path]);
	}

	cali_info->thermal_lck_avg[RF_PATH_A][cali_info->thermal_lck_avg_index] =
		thermal_value[RF_PATH_A];
	cali_info->thermal_lck_avg[RF_PATH_B][cali_info->thermal_lck_avg_index] =
		thermal_value[RF_PATH_B];
	cali_info->thermal_lck_avg_index++;

	/*Average times */
	if (cali_info->thermal_lck_avg_index == AVG_THERMAL_NUM_LCK)
		cali_info->thermal_lck_avg_index = 0;

	for (i = 0; i < AVG_THERMAL_NUM_LCK; i++) {
		if (cali_info->thermal_lck_avg[RF_PATH_A][i] ||
		    cali_info->thermal_lck_avg[RF_PATH_B][i]) {
			thermal_lck_avg[RF_PATH_A] += cali_info->thermal_lck_avg[RF_PATH_A][i];
			thermal_lck_avg[RF_PATH_B] += cali_info->thermal_lck_avg[RF_PATH_B][i];
			thermal_lck_avg_count++;
		}
	}

	/*Calculate Average ThermalValue after average enough times*/
	if (thermal_lck_avg_count) {
#if 0
		RF_DBG(dm, DBG_RF_LCK,
		       "[LCK_track] S0 ThermalValue_LCK_AVG (count) = %d (%d))\n",
		       thermal_lck_avg[RF_PATH_A], thermal_lck_avg_count);

		RF_DBG(dm, DBG_RF_LCK,
		       "[LCK_track] S1 ThermalValue_LCK_AVG (count) = %d (%d))\n",
		       thermal_lck_avg[RF_PATH_B], thermal_lck_avg_count);
#endif
		thermal_value[0] = (u8)(thermal_lck_avg[0] / thermal_lck_avg_count);
		thermal_value[1] = (u8)(thermal_lck_avg[1] / thermal_lck_avg_count);
	}

	for (path = 0; path < MAX_PATH_NUM_8822E; path++) {
		delta_lck[path] = thermal_value[path] - cali_info->thermal_lck[path];

		RF_DBG(dm, DBG_RF_LCK, "(thermal[%d], base_thermal, delta_LCK) = (%d, %d, %d)\n",
			path, thermal_value[path], cali_info->thermal_lck[path], delta_lck[path]);
	}

	for (path = 0; path < MAX_PATH_NUM_8822E; path++) {
		if (rf->is_dpk_in_progress || dm->rf_calibrate_info.is_iqk_in_progress ||
			rf->is_tssi_in_progress) {
			RF_DBG(dm, DBG_RF_LCK,
				"is_dpk_in_progress=%d is_iqk_in_progress=%d is_tssi_in_progress=%d Return !!!\n",
				rf->is_dpk_in_progress , dm->rf_calibrate_info.is_iqk_in_progress, rf->is_tssi_in_progress);
			return;
		}

		if (delta_lck[path] >= 4 || delta_lck[path] <= -4) {
				halrf_lck_trigger(dm);
				cali_info->thermal_lck[RF_PATH_A] = thermal_value[RF_PATH_A];
				cali_info->thermal_lck[RF_PATH_B] = thermal_value[RF_PATH_B];
				RF_DBG(dm, DBG_RF_LCK, "Do LCK Trigger !!!\n");
		}
	}
}

void halrf_rxspurk_8822e(
	struct dm_struct *dm)
{
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_rxspurk_info *rxspurk = &(rf->halrf_rxspurk_info);
	u8 channel = *dm->channel;
	u32 rf0_b, i, j, k, psd_result[RXSPURK_PSD_RESULT_NUM] = {0}, psd_val1, psd_val2;
	u32 psd_result_zero_num = 0;
	u32 min_psd_idx, temp;
	u32 psd_start_point = 4, psd_end_point = 16;
	u32 reg_0x824, reg_0x1b1c, reg_0x1d58, reg_0x1c38;

	RF_DBG(dm, DBG_RF_RXSPURK, "[SpurK] ======> %s channel=%d\n",
		__func__, channel);

	if (channel == 151 || channel == 153 || channel == 155 ||
		channel == 159 || channel == 161 || channel == 167 ||
		channel == 169 || channel == 171) {
		reg_0x824 = odm_get_bb_reg(dm, R_0x824, 0x000f0000);
		reg_0x1b1c = odm_get_bb_reg(dm, R_0x1b1c, 0xffffffff);
		reg_0x1d58 = odm_get_bb_reg(dm, R_0x1d58, 0xffffffff);
		reg_0x1c38 = odm_get_bb_reg(dm, R_0x1c38, 0xffffffff);
		/*BB settings*/
		odm_set_bb_reg(dm, R_0x1d58, 0x00000ff8, 0x1ff);
		odm_set_bb_reg(dm, R_0x824, 0x000f0000, 0x3);
		odm_set_bb_reg(dm, R_0x1e24, 0x00020000, 0x1);
		odm_set_bb_reg(dm, R_0x1cd0, 0x10000000, 0x1);
		odm_set_bb_reg(dm, R_0x1cd0, 0x20000000, 0x1);
		odm_set_bb_reg(dm, R_0x1cd0, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x1cd0, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x4164, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x416c, 0x00000080, 0x1);
		odm_set_bb_reg(dm, R_0x410c, 0x00000003, 0x0);
		odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x2);
		/*[7]=1: CIP_Pow*/
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);
		/*AFE on Settings*/
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);

		rf0_b = odm_get_rf_reg(dm, RF_PATH_B, 0x0, RFREGOFFSETMASK);

		btc_set_gnt_wl_bt_8822e(dm, true);

		/*RF*/
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x0, RFREGOFFSETMASK, 0x31daf);

		/*KIP*/
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, 0x1);
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);
		odm_set_bb_reg(dm, R_0x1b0c, 0x00000c00, 0x3);
		odm_set_bb_reg(dm, R_0x1b14, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x1b18, MASKDWORD, 0x00000001);
		odm_set_bb_reg(dm, R_0x1b1c, MASKDWORD, 0x82183d21);
		odm_set_bb_reg(dm, R_0x1b20, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x1b28, MASKDWORD, 0x00000000);

		k = 0;
		for (i = psd_start_point; i < psd_end_point; i++) {
			odm_set_bb_reg(dm, R_0x1814, 0x40000000, 0x0);
			odm_set_bb_reg(dm, R_0x4114, 0x40000000, 0x0);

			odm_set_bb_reg(dm, R_0x2c, 0x0001e000, i);
			odm_set_bb_reg(dm, R_0x1b2c, MASKDWORD, 0x00000010);

			if (channel == 151 || channel == 159 || channel == 167)
				odm_set_bb_reg(dm, R_0x1b2c, 0x0fff0000, 0x20);
			else if (channel == 153 || channel == 161 || channel == 169)
				odm_set_bb_reg(dm, R_0x1b2c, 0x0fff0000, 0x3e0);
			else if (channel == 155 || channel == 171)
				odm_set_bb_reg(dm, R_0x1b2c, 0x0fff0000, 0x3a0);

			odm_set_bb_reg(dm, R_0x1b34, MASKDWORD, 0x00000001);
			odm_set_bb_reg(dm, R_0x1b34, MASKDWORD, 0x00000000);

			for (j = 0; j < 500; j++)
				ODM_delay_us(1);

			odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00250001);
			psd_val1 = odm_get_bb_reg(dm, R_0x1bfc, 0x07ff0000);
			odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x002e0001);
			psd_val2 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
			psd_result[k] = ((psd_val1 << 21) + (psd_val2 >> 11)) & 0xffffffff;
			rxspurk->rxspurk_psd_result[k] = psd_result[k];

			RF_DBG(dm, DBG_RF_RXSPURK, "[SpurK] BB  k=%02d psd_result[%02d]=0x%08x\n",
				k, i, psd_result[k]);

			k++;
		}

		k = psd_end_point - psd_start_point;
		for (i = psd_start_point; i < psd_end_point; i++) {
			odm_set_bb_reg(dm, R_0x1814, 0x40000000, 0x1);
			odm_set_bb_reg(dm, R_0x4114, 0x40000000, 0x1);

			odm_set_bb_reg(dm, R_0x2c, 0x0001e000, i);
			odm_set_bb_reg(dm, R_0x1b2c, MASKDWORD, 0x00000010);

			if (channel == 151 || channel == 159 || channel == 167)
				odm_set_bb_reg(dm, R_0x1b2c, 0x0fff0000, 0x20);
			else if (channel == 153 || channel == 161 || channel == 169)
				odm_set_bb_reg(dm, R_0x1b2c, 0x0fff0000, 0x3e0);
			else if (channel == 155 || channel == 171)
				odm_set_bb_reg(dm, R_0x1b2c, 0x0fff0000, 0x3a0);

			odm_set_bb_reg(dm, R_0x1b34, MASKDWORD, 0x00000001);
			odm_set_bb_reg(dm, R_0x1b34, MASKDWORD, 0x00000000);

			for (j = 0; j < 500; j++)
				ODM_delay_us(1);

			odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00250001);
			psd_val1 = odm_get_bb_reg(dm, R_0x1bfc, 0x07ff0000);
			odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x002e0001);
			psd_val2 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
			psd_result[k] = ((psd_val1 << 21) + (psd_val2 >> 11)) & 0xffffffff;
			rxspurk->rxspurk_psd_result[k] = psd_result[k];

			RF_DBG(dm, DBG_RF_RXSPURK, "[SpurK] PLL k=%02d psd_result[%02d]=0x%08x\n",
				k, i, psd_result[k]);

			k++;
		}

		btc_set_gnt_wl_bt_8822e(dm, false);

		for (i = 0; i < (psd_end_point - psd_start_point) * 2; i++) {
			if (psd_result[i] == 0)
				psd_result_zero_num++;
		}

		if (psd_result_zero_num == (psd_end_point - psd_start_point) * 2) {
			min_psd_idx = 8;
			odm_set_bb_reg(dm, R_0x2c, 0x0001e000, min_psd_idx);
			rxspurk->final_psd_idx = min_psd_idx;
			odm_set_bb_reg(dm, R_0x1814, 0x40000000, 0x1);
			odm_set_bb_reg(dm, R_0x4114, 0x40000000, 0x1);
		} else {
			temp = psd_result[0];
			k = 0;
			for (i = 1; i < (psd_end_point - psd_start_point) * 2; i++) {
				if (temp > psd_result[i]) {
					temp = psd_result[i];
					k = i;
				}
			}

			RF_DBG(dm, DBG_RF_RXSPURK, "[SpurK] Find min k=%d\n", k);

			if ((psd_result[k] < 0x7fff) &&
				(channel == 159 || channel == 161 || channel == 167 ||
				channel == 169 || channel == 171)) {
				odm_set_bb_reg(dm, R_0x1944, 0x0001f000, 0x0);
				odm_set_bb_reg(dm, R_0x4044, 0x0001f000, 0x0);
				odm_set_bb_reg(dm, R_0x1940, 0x80000000, 0x0);
				odm_set_bb_reg(dm, R_0x4040, 0x80000000, 0x0);
				odm_set_bb_reg(dm, R_0x818, 0x00000800, 0x0);
				odm_set_bb_reg(dm, R_0x1d3c, 0x78000000, 0x0);
				odm_set_bb_reg(dm, R_0x810, 0x0000000f, 0x0);
				odm_set_bb_reg(dm, R_0x810, 0x000f0000, 0x0);
				odm_set_bb_reg(dm, R_0x88c, 0x00030000, 0x2);
				odm_set_bb_reg(dm, R_0x1944, 0x00000300, 0x3);
				odm_set_bb_reg(dm, R_0x4044, 0x00000300, 0x3);

				rxspurk->nbi_csi_en = false;

				RF_DBG(dm, DBG_RF_RXSPURK, "[SpurK] psd_result < 0x7fff channel=%d\n",
					channel);
			} else
				rxspurk->nbi_csi_en = true;

			if (k < (psd_end_point - psd_start_point)) {
				min_psd_idx = k + psd_start_point;
				odm_set_bb_reg(dm, R_0x2c, 0x0001e000, min_psd_idx);
				odm_set_bb_reg(dm, R_0x1814, 0x40000000, 0x0);
				odm_set_bb_reg(dm, R_0x4114, 0x40000000, 0x0);
			} else {
				min_psd_idx = k - (psd_end_point - psd_start_point) +
					psd_start_point;
				odm_set_bb_reg(dm, R_0x2c, 0x0001e000, min_psd_idx);
				odm_set_bb_reg(dm, R_0x1814, 0x40000000, 0x1);
				odm_set_bb_reg(dm, R_0x4114, 0x40000000, 0x1);
			}

			RF_DBG(dm, DBG_RF_RXSPURK, "[SpurK] min_psd_idx=%d 0x2c[16:13]=0x%x\n",
				min_psd_idx, odm_get_bb_reg(dm, R_0x2c, 0x0001e000));

			RF_DBG(dm, DBG_RF_RXSPURK, "[SpurK] 0x1814=0x%x 0x4114=0x%x\n",
				odm_get_bb_reg(dm, R_0x1814, 0x40000000),
				odm_get_bb_reg(dm, R_0x4114, 0x40000000));

			rxspurk->final_psd_idx = min_psd_idx;
		}	

		/*AFE Restore Settings*/
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0x00000000);
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, reg_0x1c38);
		/*s1*/
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, 0x1);
		/*[7]=0: CIP_Pow*/
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000000);
		/*BB Restore Settings*/
		odm_set_bb_reg(dm, R_0x4164, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x0);
		odm_set_bb_reg(dm, R_0x416c, 0x00000080, 0x0);
		odm_set_bb_reg(dm, R_0x410c, 0x00000003, 0x3);
		odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x0);
		odm_set_bb_reg(dm, R_0x1b20, 0x20000000, 0x1);
		odm_set_bb_reg(dm, R_0x1b1c, MASKDWORD, reg_0x1b1c);
		odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x1);
		odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x0);
		odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x1);
		odm_set_bb_reg(dm, R_0x0, 0x00010000, 0x1);
		odm_set_bb_reg(dm, R_0x0, 0x00010000, 0x0);
		odm_set_bb_reg(dm, R_0x0, 0x00010000, 0x1);
		odm_set_bb_reg(dm, R_0x824, 0x000f0000, reg_0x824);
		odm_set_bb_reg(dm, R_0x1d58, 0x00000ff8, reg_0x1d58);

		odm_set_rf_reg(dm, RF_PATH_B, 0x0, RFREGOFFSETMASK, rf0_b);

	} else if (channel == 54 || channel == 58) {
		odm_set_bb_reg(dm, R_0x1814, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x4114, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x2c, 0x0001e000, 0x8);
	} else {
		odm_set_bb_reg(dm, R_0x1814, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x4114, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x2c, 0x0001e000, 0x8);
	}

	RF_DBG(dm, DBG_RF_RXSPURK, "[SpurK] <====== %s channel=%d\n",
		__func__, channel);
}

void halrf_rxspurk_info_8822e(void *dm_void, u32 *_used, char *output, u32 *_out_len)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_rxspurk_info *rxspurk = &(rf->halrf_rxspurk_info);
	u8 i, j;

	RF_DBG(dm, DBG_RF_RXSPURK, "[RF][SpurK] ======>%s\n", __func__);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used,
		"\n===============[ SpurK info ]===============\n");

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-25s = %s\n",
		 "RX Spur K Version", HALRF_RXSPURK_VER);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-25s = %d\n",
		"CH", *dm->channel);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-25s = 0x%x\n",
		"RX Spur K final idx", rxspurk->final_psd_idx);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-25s = 0x%x / 0x%x / 0x%x\n",
		"0x1814 / 0x4114 / 0x2c",
		odm_get_bb_reg(dm, R_0x1814, 0x40000000),
		odm_get_bb_reg(dm, R_0x4114, 0x40000000),
		odm_get_bb_reg(dm, R_0x2c, 0x0001e000));

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-25s = %s\n",
		"NBI CSI", rxspurk->nbi_csi_en ? "Enable" : "Disable");

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-25s = %llums\n",
		"RX Spur K Time",
		rf->rxspurk_progressing_time);

	j = 4;
	for (i = 0; i < RXSPURK_PSD_RESULT_NUM / 2; i++)
		PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%02d %-22s = 0x%08x\n",
			j++, "BB  PSD Result", rxspurk->rxspurk_psd_result[i]);

	j = 4;
	for (i = RXSPURK_PSD_RESULT_NUM / 2; i < RXSPURK_PSD_RESULT_NUM; i++)
		PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%02d %-22s = 0x%08x\n",
			j++, "PLL PSD Result", rxspurk->rxspurk_psd_result[i]);

}

void halrf_pwr_trk_info_8822e(void *dm_void, u32 *_used, char *output, u32 *_out_len)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_rxspurk_info *rxspurk = &(rf->halrf_rxspurk_info);
	struct dm_rf_calibration_struct *cali_info = &(dm->rf_calibrate_info);
	u8 i;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[RF] ======>%s\n", __func__);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used,
		"\n===============[ Power Tracking info ]===============\n");

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-30s : %s\n",
		 "Power Tracking", HALRF_POWRTRACKING_VER);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-30s = %d\n",
		"CH", *dm->channel);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used,
		"%-30s = 0x%02x / 0x%02x / 0x%02x\n",
		"Current / AVG / PG TherA",
		halrf_get_thermal_8822e(dm, RF_PATH_A),
		cali_info->thermal_value_avg_pwrtrk[RF_PATH_A],
		cali_info->thermal_value_path[RF_PATH_A]);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used,
		"%-30s = 0x%02x / 0x%02x / 0x%02x\n",
		"Current / AVG / PG TherB",
		halrf_get_thermal_8822e(dm, RF_PATH_B),
		cali_info->thermal_value_avg_pwrtrk[RF_PATH_B],
		cali_info->thermal_value_path[RF_PATH_B]);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-30s = 0x%02x / 0x%02x\n",
		"Pwr Trk Ofst Tbl A / B",
		(u8)cali_info->absolute_ofdm_swing_idx[RF_PATH_A],
		(u8)cali_info->absolute_ofdm_swing_idx[RF_PATH_B]);

	PDM_SNPF(*_out_len, *_used, output + *_used, *_out_len - *_used, "%-30s = 0x%02x / 0x%02x\n",
		"Pwr Trk Ofst Reg A / B",
		odm_get_bb_reg(dm, R_0x18a0, 0x000000ff),
		odm_get_bb_reg(dm, R_0x41a0, 0x000000ff));
}
#endif /*(RTL8822E_SUPPORT == 0)*/
