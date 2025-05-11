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

/*---------------------------Define Local Constant---------------------------*/

/*8822E DPK ver:0x11 20240829*/

static u32
_btc_wait_indirect_reg_ready_8822e(
	struct dm_struct *dm)
{
	u32 delay_count = 0;
	u16 i;
	
	/* wait for ready bit before access 0x1700 */
	while (1) {
		if ((odm_read_1byte(dm, 0x1703) & BIT(5)) == 0) {
			for (i = 0; i < 500; i++) /*delay 10ms*/
				ODM_delay_us(20);

			if (++delay_count >= 10)
			break;
		} else {
			break;
		}
	}
	
	return delay_count;
}

static u32
_btc_read_indirect_reg_8822e(
	struct dm_struct *dm,
	u16 reg_addr)
{
	u32 delay_count = 0;

	/* wait for ready bit before access 0x1700 */
	_btc_wait_indirect_reg_ready_8822e(dm);

	odm_write_4byte(dm, 0x1700, 0x800F0000 | reg_addr);

	return odm_read_4byte(dm, 0x1708); /* get read data */
}

static void
_btc_write_indirect_reg_8822e(
	struct dm_struct *dm,
	u16 reg_addr,
	u32 bit_mask,
	u32 reg_value)
{
	u32 val, i = 0, bitpos = 0, delay_count = 0;

	if (bit_mask == 0x0)
		return;

	if (bit_mask == 0xffffffff) {
	/* wait for ready bit before access 0x1700 */
	_btc_wait_indirect_reg_ready_8822e(dm);

	/* put write data */
	odm_write_4byte(dm, 0x1704, reg_value);

	odm_write_4byte(dm, 0x1700, 0xc00F0000 | reg_addr);
	} else {
		for (i = 0; i <= 31; i++) {
			if (((bit_mask >> i) & 0x1) == 0x1) {
				bitpos = i;
				break;
			}
		}

		/* read back register value before write */
		val = _btc_read_indirect_reg_8822e(dm, reg_addr);
		val = (val & (~bit_mask)) | (reg_value << bitpos);

		/* wait for ready bit before access 0x1700 */
		_btc_wait_indirect_reg_ready_8822e(dm);

		odm_write_4byte(dm, 0x1704, val); /* put write data */
		odm_write_4byte(dm, 0x1700, 0xc00F0000 | reg_addr);
	}
}

void btc_set_gnt_wl_bt_8822e(
	void *dm_void,
	boolean is_before_k)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	if (is_before_k) {
		dpk_info->gnt_control = odm_get_mac_reg(dm, R_0x70, MASKDWORD);
		dpk_info->gnt_value = _btc_read_indirect_reg_8822e(dm, 0x38);
		
		/*force GNT control to WL*/
		odm_set_mac_reg(dm, R_0x70, BIT(26), 0x1);
		/*force GNT_WL=1, GNT_BT=0*/
		_btc_write_indirect_reg_8822e(dm, 0x38, 0xFF00, 0x77);
#if 0
		RF_DBG(dm, DBG_RF_DPK, "[DPK] ori 0x70 / 0x38 = 0x%x / 0x%x\n",
		       dpk_info->gnt_control, dpk_info->gnt_value);

		RF_DBG(dm, DBG_RF_DPK, "[DPK] set 0x70/0x38 = 0x%x/0x%x\n",
		       odm_get_mac_reg(dm, R_0x70, MASKDWORD),
		       _btc_read_indirect_reg_8822e(dm, 0x38));
#endif
	} else {
		_btc_write_indirect_reg_8822e(dm, 0x38, MASKDWORD, dpk_info->gnt_value);
		odm_set_mac_reg(dm, R_0x70, MASKDWORD, dpk_info->gnt_control);
#if 0
		RF_DBG(dm, DBG_RF_DPK, "[DPK] load 0x70 / 0x38 = 0x%x / 0x%x\n",
		       odm_get_mac_reg(dm, R_0x70, MASKDWORD),
		       _btc_read_indirect_reg_8822e(dm, 0x38));
#endif
	}
}

void _backup_mac_bb_registers_8822e(
	struct dm_struct *dm,
	u32 *reg,
	u32 *reg_backup,
	u32 reg_num)
{
	u32 i;

	for (i = 0; i < reg_num; i++) {
		reg_backup[i] = odm_read_4byte(dm, reg[i]);
#if 0
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Backup MAC/BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
#endif
	}
}

void _backup_kip_registers_8822e(
	struct dm_struct *dm,
	u32 *kip_reg,
	u32 kip_reg_backup[][2])
{
	u32 i, j;

	for (i = 0; i < DPK_KIP_REG_NUM_8822E; i++) {
		for (j = 0; j < DPK_RF_PATH_NUM_8822E; j++) {
			odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), j);
			kip_reg_backup[i][j] = odm_get_bb_reg(dm, kip_reg[i], MASKDWORD);
#if 0
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Backup KIP 0x%x = 0x%x\n",
		       kip_reg[i], kip_reg_backup[i][j]);
#endif			
		}
	}
}

void _backup_rf_registers_8822e(
	struct dm_struct *dm,
	u32 *rf_reg,
	u32 rf_reg_backup[][2])
{
	u32 i;

	for (i = 0; i < DPK_RF_REG_NUM_8822E; i++) {
		rf_reg_backup[i][RF_PATH_A] = odm_get_rf_reg(dm, RF_PATH_A,
			rf_reg[i], RFREG_MASK);
		rf_reg_backup[i][RF_PATH_B] = odm_get_rf_reg(dm, RF_PATH_B,
			rf_reg[i], RFREG_MASK);
#if 0
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Backup RF_A 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_A]);
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Backup RF_B 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_B]);
#endif
	}
}

void _reload_mac_bb_registers_8822e(
	struct dm_struct *dm,
	u32 *reg,
	u32 *reg_backup,
	u32 reg_num)

{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u32 i;

	/*toggle IGI*/
	odm_write_4byte(dm, R_0x1d70, 0x50505050);

	for (i = 0; i < reg_num; i++) {
		odm_write_4byte(dm, reg[i], reg_backup[i]);
#if 0
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Reload MAC/BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
#endif
	}
}

void _reload_kip_registers_8822e(
	struct dm_struct *dm,
	u32 *kip_reg,
	u32 kip_reg_backup[][2])
{
	u32 i, j;

	for (i = 0; i < DPK_KIP_REG_NUM_8822E; i++) {
		for (j = 0; j < DPK_RF_PATH_NUM_8822E; j++) {
			odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), j);
			odm_set_bb_reg(dm, kip_reg[i], MASKDWORD, kip_reg_backup[i][j]);
#if 0
			RF_DBG(dm, DBG_RF_DPK, "[DPK] Reload KIP 0x%x = 0x%x\n",
			       kip_reg[i], kip_reg_backup[i][j]);
#endif
		}
	}
}

void _reload_rf_registers_8822e(
	struct dm_struct *dm,
	u32 *rf_reg,
	u32 rf_reg_backup[][2])
{
	u32 i;

	for (i = 0; i < DPK_RF_REG_NUM_8822E; i++) {
		odm_set_rf_reg(dm, RF_PATH_A, rf_reg[i], RFREG_MASK,
			       rf_reg_backup[i][RF_PATH_A]);
		odm_set_rf_reg(dm, RF_PATH_B, rf_reg[i], RFREG_MASK,
			       rf_reg_backup[i][RF_PATH_B]);
#if 0
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Reload RF_A 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_A]);
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Reload RF_B 0x%x = 0x%x\n",
		       rf_reg[i], rf_reg_backup[i][RF_PATH_B]);
#endif
	}
}

void _dpk_kip_clk_off_8822e(
	struct dm_struct *dm)
{
	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2); /*subpage 2*/
	odm_set_bb_reg(dm, R_0x1bd4, 0x000000f0, 0x4); /*force CLK off for power saving*/
	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x0); /*subpage 0*/
}

void _dpk_information_8822e(
	struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u32  reg_rf18;

	if (odm_get_bb_reg(dm, R_0x1e7c, BIT(30)))
		dpk_info->is_tssi_mode = true;
	else
		dpk_info->is_tssi_mode = false;

	dpk_info->dpk_rf18[0] = odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK);

	dpk_info->dpk_band = (u8)((dpk_info->dpk_rf18[0] & BIT(16)) >> 16); /*0/1:G/A*/
	dpk_info->dpk_ch = (u8)dpk_info->dpk_rf18[0] & 0xff;
	dpk_info->dpk_bw = (u8)((dpk_info->dpk_rf18[0] & 0x3000) >> 12); /*3/2/1:20/40/80*/

	RF_DBG(dm, DBG_RF_DPK, "[DPK] TSSI/ Band/ CH/ BW = %d / %s / %d / %s\n",
	       dpk_info->is_tssi_mode, dpk_info->dpk_band == 0 ? "2G" : "5G",
	       dpk_info->dpk_ch,
	       dpk_info->dpk_bw == 3 ? "20M" : (dpk_info->dpk_bw == 2 ? "40M" : 

	       (dpk_info->dpk_bw == 1 ?"80M" : "other BW")));
}

u8 _dpk_one_shot_8822e(
	struct dm_struct *dm,
	u8 path,
	u8 action)
{
#ifdef  HALRF_DZ_LOG
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct halrf_rfk_dz_rpt *rfk_dz = &(rf->rfk_dz_rpt);
#endif
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 temp = 0x0, bw = 0x0, reg_2d9c = 0x0, result = 0;
	u16 dpk_cmd = 0x0, count = 0;

	if (dpk_info->dpk_bw == 0x1) /*80M*/
		bw = 2;
	else
		bw = 1;

	if (action == SYNC_DC)
		temp = 0x10 + bw;
	else if (action == DAGC)
		temp = 0x13 + path;
	else if (action == MDPK_DC)
		temp = 0x15 + bw;
	else if (action == GAIN_LOSS)
		temp = 0x18 + path;
	else if (action == DO_DPK)
		temp = 0x1a + path;
	else if (action == DPK_ON)
		temp = 0x1c + path;
	else if (action == LBK_RXIQK)
		temp = 0xe;

	btc_set_gnt_wl_bt_8822e(dm, true);

	if (action != DPK_ON)
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x00, 0xF0000, 0x5);

	if (action == LBK_RXIQK)
		dpk_cmd = (temp << 8) | (0x19 + path * 0x10);
	else
		dpk_cmd = (temp << 8) | 0x4c;

	odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, dpk_cmd);
	odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, dpk_cmd + 1);

	reg_2d9c = odm_read_1byte(dm, R_0x2d9c);
	while (reg_2d9c != 0x55 && count < 2000) {
		ODM_delay_us(10);
		reg_2d9c = odm_read_1byte(dm, R_0x2d9c);
		count++;
	}

//	RF_DBG(dm, DBG_RF_DPK, "[DPK] 0x1b00 after = 0x%x\n", odm_get_bb_reg(dm, R_0x1b00, MASKDWORD));

	RF_DBG(dm, DBG_RF_DPK, "[DPK] one-shot for S%d %s = 0x%x (count=%d)\n", path,
		action == SYNC_DC ? "SYNC_DC" : (action == DAGC ? "DAGC" :
	        (action == MDPK_DC ? "MDPK_DC" : (action == GAIN_LOSS ? "GAIN_LOSS" : 
	        (action == DO_DPK ? "DO_DPK" : (action == DPK_ON ? "DPK_ON" :
		(action == LBK_RXIQK ? "LBK_RXIQK" : "unknown id")))))),
	       	dpk_cmd, count);

	if (count == 2000) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] one-shot over 20ms!!!!\n");
#ifdef  HALRF_DZ_LOG
		rfk_dz->dpk_dz_code |= BIT(8 * path);
#endif
		result = 1;
	}

	btc_set_gnt_wl_bt_8822e(dm, false);

	odm_write_1byte(dm, 0x1b10, 0x0);

	return result;
}

void _dpk_rxdck_8822e(
	struct dm_struct *dm,
	u8 path)
{
	u32 i;

	RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d RXDCK\n", path);

	btc_set_gnt_wl_bt_8822e(dm, true);

	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x00, 0xF0000, 0x5);

	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x93, BIT(6), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x1);

	for (i = 0; i < 150; i++)
		ODM_delay_us(10); /*delay 1.5ms*/

	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x93, BIT(6), 0x1);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x1);

	for (i = 0; i < 150; i++)
		ODM_delay_us(10); /*delay 1.5ms*/

	odm_set_rf_reg(dm, (enum rf_path)path, 0x92, BIT(0), 0x0);

	btc_set_gnt_wl_bt_8822e(dm, false);
}

void _dpk_lbk_rxiqk_8822e(
	struct dm_struct *dm,
	u8 path)
{
	u32 reg_rf0;

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
	reg_rf0 = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, RFREG_MASK);
	odm_set_bb_reg(dm, R_0x1b20, 0xc0000000, 0x3); /*CFIR to TX*/
	odm_set_bb_reg(dm, R_0x1b24, RFREG_MASK, 0x501ec); /*PI data to RF0x0*/
	odm_set_bb_reg(dm, R_0x1bcc, 0x0000003F, 0x1b); /*[5:0] Tx I/Q Swing*/
	odm_set_bb_reg(dm, R_0x1b2c, 0x0000003F, 0x00180018); /*tone idx*/

	_dpk_one_shot_8822e(dm, path, LBK_RXIQK);

	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x00, RFREG_MASK, reg_rf0);
	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
	odm_set_bb_reg(dm, R_0x1b20, 0xc0000000, 0x3); /*CFIR to TX*/
	odm_set_bb_reg(dm, R_0x1b20, BIT(25), 0x0); /*bypass DPK*/
	
	RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d LBK RXIQC = 0x%x\n", path,
		odm_get_bb_reg(dm, R_0x1b3c, MASKDWORD));
}

void _dpk_tx_pause_8822e(
	struct dm_struct *dm)
{
	u8 reg_rf0_a, reg_rf0_b;
	u16 count = 0;

	odm_write_1byte(dm, R_0x522, 0xff);
	odm_set_bb_reg(dm, R_0x1e70, 0x0000000f, 0x2); /*hw tx stop*/

	reg_rf0_a = (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000);
	reg_rf0_b = (u8)odm_get_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000);

	while (((reg_rf0_a != 3) && (reg_rf0_b != 3)) && count < 2500) {
		reg_rf0_a = (u8)odm_get_rf_reg(dm, RF_PATH_A, RF_0x00, 0xF0000);
		reg_rf0_b = (u8)odm_get_rf_reg(dm, RF_PATH_B, RF_0x00, 0xF0000);
		ODM_delay_us(2);
		count++;
	}

	RF_DBG(dm, DBG_RF_DPK, "[DPK] Tx pause!!\n");
}

void _dpk_mac_bb_setting_8822e(
	struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	_dpk_tx_pause_8822e(dm);

	if (dpk_info->is_tssi_mode) {
		odm_set_bb_reg(dm, R_0x1e7c, BIT(30), 0x0);
		odm_set_bb_reg(dm, R_0x18a4, BIT(28), 0x0);
		odm_set_bb_reg(dm, R_0x41a4, BIT(28), 0x0);
	}

	odm_set_bb_reg(dm, R_0x1e24, BIT(17), 0x1); /*r_gothrough_iqkdpk*/
	odm_set_bb_reg(dm, R_0x1cd0, 0xF0000000, 0x7);

	odm_set_bb_reg(dm, R_0x1d58, 0xff8, 0x1ff); /*OFDM CCA off*/
	odm_set_bb_reg(dm, R_0x1a00, BIT(1) | BIT(0), 0x2); /*CCK CCA off*/

	/*r_rftxen_gck_force*/
	odm_set_bb_reg(dm, R_0x1864, BIT(31), 0x1);
	odm_set_bb_reg(dm, R_0x4164, BIT(31), 0x1);
	/*r_dis_sharerx_txgat*/
	odm_set_bb_reg(dm, R_0x180c, BIT(27), 0x1);
	odm_set_bb_reg(dm, R_0x410c, BIT(27), 0x1);

	odm_set_bb_reg(dm, R_0x186c, BIT(7), 0x1);
	odm_set_bb_reg(dm, R_0x416c, BIT(7), 0x1);

	odm_set_bb_reg(dm, R_0x180c, BIT(1) | BIT(0), 0x0);
	odm_set_bb_reg(dm, R_0x410c, BIT(1) | BIT(0), 0x0);

	odm_set_bb_reg(dm, R_0x80c, 0x0000000f, 0x8); /*freq shap filter*/

	RF_DBG(dm, DBG_RF_DPK, "[DPK] MAC/BB setting for DPK mode\n");
}

void _dpk_manual_txagc_8822e(
	struct dm_struct *dm,
	boolean is_manual)
{
	odm_set_bb_reg(dm, R_0x18a4, BIT(7), is_manual);
	odm_set_bb_reg(dm, R_0x41a4, BIT(7), is_manual);
}

void _dpk_set_txagc_8822e(
	struct dm_struct *dm)
{
	odm_set_bb_reg(dm, R_0x18a0, 0x007C0000, 0x1f);
	odm_set_bb_reg(dm, R_0x41a0, 0x007C0000, 0x1f);
	odm_set_bb_reg(dm, 0x18e8, 0x0001F000, 0x1f);
	odm_set_bb_reg(dm, 0x41e8, 0x0001F000, 0x1f);
}

void _dpk_afe_setting_8822e(
	struct dm_struct *dm,
	boolean is_do_dpk)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	if (is_do_dpk) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0x00000000);

		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x0);
		odm_set_bb_reg(dm, 0x1860, 0xfffff000, 0xf0001); /*[31:12]*/
		odm_set_bb_reg(dm, R_0x4130, BIT(30), 0x0);
		odm_set_bb_reg(dm, 0x4160, 0xfffff000, 0xf0001); /*[31:12]*/

		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);

		RF_DBG(dm, DBG_RF_DPK, "[DPK] AFE for DPK mode\n");
	} else {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0x00000000);

		odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x1);
		odm_set_bb_reg(dm, R_0x4130, BIT(30), 0x1);

		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);
		
		RF_DBG(dm, DBG_RF_DPK, "[DPK] AFE for normal mode\n");
	}

	halrf_ex_dac_fifo_rst(dm);
}

void _dpk_pre_setting_8822e(
	struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 path;

	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {
		odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
		odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080); /*POW_KIP*/
		odm_set_bb_reg(dm, R_0x1b20, 0xc0000000, 0x3); /*CFIR to TX*/
		odm_set_bb_reg(dm, R_0x1b20, BIT(25), 0x0); /*bypass DPK*/
#if 0		
		if (dpk_info->dpk_bw == 0x3) /*20M*/
			odm_set_bb_reg(dm, R_0x1b44, BIT(14) | BIT(13) | BIT(12), 0x2);
		else
			odm_set_bb_reg(dm, R_0x1b44, BIT(14) | BIT(13) | BIT(12), 0x3);

//		if (dpk_info->dpk_band == 0x0) /*txagc bnd*/
//			odm_set_bb_reg(dm, R_0x1b60, MASKDWORD, 0x1f040000);
//		else
//			odm_set_bb_reg(dm, R_0x1b60, MASKDWORD, 0x1f100000);
#endif
	}
	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2);
	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0xb000c000);
//	odm_set_bb_reg(dm, R_0x1be4, MASKDWORD, 0x3b23170b); /*one-shot 6ms*/
//	odm_set_bb_reg(dm, R_0x1be8, MASKDWORD, 0x775f5347);
}

void _dpk_rf_setting_8822e(
	struct dm_struct *dm,
	u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 i;

	if (dpk_info->dpk_band == 0x0) { /*2G*/
		/*TXAGC for gainloss*/
		odm_set_rf_reg(dm, (enum rf_path)path,
			       RF_0x00, RFREG_MASK, 0x50014);
		/*ATT Gain*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x88,
			       BIT(6) | BIT(5) | BIT(4), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x88,
			       0x0000F, 0x1);
		/*TIA*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xdf,
			       BIT(16), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x87,
			       BIT(19) | BIT(18), 0x1);
		/*SW to GND*/
		odm_set_rf_reg(dm, (enum rf_path)path, 0x89,
			       BIT(12), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, 0xdf,
			       BIT(3), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, 0x8a,
			       0xF0000, 0xe);
	} else { /*5G*/
		/*TXAGC for gainloss*/
		odm_set_rf_reg(dm, (enum rf_path)path,
			       RF_0x00, RFREG_MASK, 0x50016);
		/*ATT Gain*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x8b,
			       BIT(10) | BIT(9) | BIT(8), 0x2);
		/*TIA*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xdf,
			       BIT(17), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, 0x89,
			       BIT(1) | BIT(0), 0x1);
		/*switch*/
		odm_set_rf_reg(dm, (enum rf_path)path, 0x89,
			       BIT(12), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, 0xdf,
			       BIT(3), 0x1);
		odm_set_rf_reg(dm, (enum rf_path)path, 0x8a,
			       0xF0000, 0xe);
	}
		/*PGA2*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x00,
			       0x003e0, 0xc); /*[9:5]*/

//		odm_set_rf_reg(dm, (enum rf_path)path, RF_0xde,
//			       BIT(2), 0x1);

		/*BW of RXBB*/
		odm_set_rf_reg(dm, (enum rf_path)path, RF_0x1a,
			       BIT(11) | BIT(10), 0x0);
		/*BW of TXBB*/
#if 0
		if (dpk_info->dpk_bw == 0x1) /*80M*/
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x1a,
				       BIT(14) | BIT(13) | BIT(12), 0x2);
		else
			odm_set_rf_reg(dm, (enum rf_path)path, RF_0x1a,
				       BIT(14) | BIT(13) | BIT(12), 0x1);

		odm_set_rf_reg(dm, (enum rf_path)path, 0x90, BIT(15), 0x1);
#endif
	RF_DBG(dm, DBG_RF_DPK, "[DPK] RF 0x0 / 0x1a = 0x%x / 0x%x\n",
		odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, RFREG_MASK),
		odm_get_rf_reg(dm, (enum rf_path)path, RF_0x1a, RFREG_MASK));

	for (i = 0; i < 5; i++) /*delay 100us for TIA SV loop*/
		ODM_delay_us(20);
}

u16 _dpk_dgain_read_8822e(
	struct dm_struct *dm,
	u8 path)
{
	u16 dgain = 0x0;

	odm_set_bb_reg(dm, 0x1bd4, 0x001F0000, 0x0); /*[20:16]*/

	dgain = (u16)odm_get_bb_reg(dm, R_0x1bfc, 0x0FFF0000); /*[27:16]*/

	RF_DBG(dm, DBG_RF_DPK, "[DPK] DGain = 0x%x (%d)\n", dgain, dgain);

	return dgain;
}

u8 _dpk_thermal_read_8822e(
	void *dm_void,
	u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x42, BIT(19), 0x1);
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x42, BIT(19), 0x0);
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x42, BIT(19), 0x1);
	ODM_delay_us(15);

	return (u8)odm_get_rf_reg(dm, (enum rf_path)path, RF_0x42, 0x0007e);
}

u8 _dpk_pas_read_8822e(
	struct dm_struct *dm,
	u8 path,
	boolean is_check)
{
#ifdef  HALRF_DZ_LOG
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct halrf_rfk_dz_rpt *rfk_dz = &(rf->rfk_dz_rpt);
#endif
	u8 k, result;
	u32 val1, val2;
	u32 val1_i = 0, val1_q = 0, val2_i = 0, val2_q = 0;

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
	//odm_set_bb_reg(dm, R_0x1b48, BIT(14), 0x0);

	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00060000);
	//odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, 0x00000000);

	if (is_check) {
		odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, 0x00080000);

		val1 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);

		val1_q = (val1 & MASKHWORD) >> 16;
		val1_i = val1 & MASKLWORD;

		if (val1_i >> 15 != 0)
			val1_i = 0x10000 - val1_i;
		if (val1_q >> 15 != 0)
			val1_q = 0x10000 - val1_q;

		odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, 0x3f080000);

		val2 = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);

		val2_q = (val1 & MASKHWORD) >> 16;
		val2_i = val1 & MASKLWORD;

		if (val2_i >> 15 != 0)
			val2_i = 0x10000 - val2_i;
		if (val2_q >> 15 != 0)
			val2_q = 0x10000 - val2_q;

		if ((val2_i * val2_i + val2_q * val2_q) != 0) /*to avoid BSOD issue*/	
			RF_DBG(dm, DBG_RF_DPK, "[DPK] PAS_delta = 0x%x\n",
				(val1_i * val1_i + val1_q * val1_q) / 
				(val2_i * val2_i + val2_q * val2_q));
	} else {
		for (k = 0; k < 64; k++) {
			odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD,
				       (0x00080000 | (k << 24)));
#ifdef HALRF_DZ_LOG
			rfk_dz->dpk_pas[path][k] = odm_get_bb_reg(dm, 0x80fc, MASKDWORD);
			RF_DBG(dm, DBG_RF_DPK, "[DPK] PAS_Read[%02d]= 0x%08x\n", k, rfk_dz->dpk_pas[path][k]);
#else
			RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d PA scan[%02d] = 0x%08x\n",
			       path, k, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
#endif
		}
	}
	odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, 0x00000000);

	if ((val1_i * val1_i + val1_q * val1_q) < (val2_i * val2_i + val2_q * val2_q))
		return 2;
	else if ((val1_i * val1_i + val1_q * val1_q) >= ((val2_i * val2_i + val2_q * val2_q) * 5 / 2))
		return 1; /*delta >=4dB*/
	else
		return 0;
}

void _dpk_rxsram_8822e(
	struct dm_struct *dm,
	u8 path)
{
#ifdef  HALRF_DZ_LOG
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct halrf_rfk_dz_rpt *rfk_dz = &(rf->rfk_dz_rpt);
#endif
	u32 addr;

	odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2);
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x000200f1);

	for (addr = 0x0; addr < 0x200; addr++) {
		odm_set_bb_reg(dm, R_0x1bf4, MASKDWORD, 0x0 | (addr << 8));
#ifdef  HALRF_DZ_LOG
		rfk_dz->dpk_rxsram[path][addr] = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
		RF_DBG(dm, DBG_RF_DPK, "[DPK] RXSRAM [%03d] = 0x%08x\n",
			       addr, rfk_dz->dpk_rxsram[path][addr]);
#else
		RF_DBG(dm, DBG_RF_DPK, "[DPK] RXSRAM [%03d] = 0x%08x\n",
			       addr, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
#endif
	}
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x000200f0);
	odm_set_bb_reg(dm, R_0x1bf4, MASKDWORD, 0x15020010);
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x13040000);
}

u8 _dpk_dc_corr_check_8822e(
	struct dm_struct *dm,
	u8 path)
{
#ifdef  HALRF_DZ_LOG
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct halrf_rfk_dz_rpt *rfk_dz = &(rf->rfk_dz_rpt);
#endif
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u16 dc_i, dc_q;
	u8 corr_val, corr_idx;

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
	odm_set_bb_reg(dm, 0x1bd4, 0x003F0000, 0x9); /*[21:16]*/
	dc_i = (u16)odm_get_bb_reg(dm, 0x1bfc, 0x0fff0000); /*[27:16]*/
	dc_q = (u16)odm_get_bb_reg(dm, 0x1bfc, 0x00000fff); /*[11:0]*/

	if (dc_i >> 11 == 1)
		dc_i = 0x1000 - dc_i;
	if (dc_q >> 11 == 1)
		dc_q = 0x1000 - dc_q;

	RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d DC I/Q, = %d / %d\n", path, dc_i, dc_q);

	dpk_info->dc_i[path] = dc_i;
	dpk_info->dc_q[path] = dc_q;

	odm_set_bb_reg(dm, 0x1bd4, 0x001F0000, 0x0); /*[20:16]*/
	corr_idx = (u8)odm_get_bb_reg(dm, 0x1bfc, 0x000000ff); /*[7:0]*/
	corr_val = (u8)odm_get_bb_reg(dm, 0x1bfc, 0x0000ff00); /*[15:8]*/

	RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d Corr_idx / Corr_val = %d / %d\n",
	       path, corr_idx, corr_val);

	dpk_info->corr_idx[path] = corr_idx;
	dpk_info->corr_val[path] = corr_val;

	if ((dc_i > 200) || (dc_q > 200) || (corr_val < 150)) {
#ifdef  HALRF_DZ_LOG
		rfk_dz->dpk_dz_code |= BIT(8 * path + 1);
#endif
		return 1;
	} else
		return 0;

}

u8 _dpk_gainloss_result_8822e(
	struct dm_struct *dm,
	u8 path)
{
	u8 result;

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
	odm_set_bb_reg(dm, R_0x1b48, BIT(14), 0x1);
	odm_set_bb_reg(dm, 0x1bd4, 0x003F0000, 0x6); /*[21:16]*/

	result = (u8)odm_get_bb_reg(dm, R_0x1bfc, 0x000000f0); /*[7:4]*/

	odm_set_bb_reg(dm, R_0x1b48, BIT(14), 0x0);

	RF_DBG(dm, DBG_RF_DPK, "[DPK] tmp GL = %d\n", result);
	return result;
}

u8 _dpk_pas_chk_8822e(
	struct dm_struct *dm,
	u8 path,
	u8 limited_pga,
	u8 check)
{
	u8 result = 0;
	u16 dgain =0;
	u32 loss = 0;
	u32 loss_db = 0;

	loss = _dpk_pas_read_8822e(dm, path, LOSS_CHK);

	if (loss < 0x4000000) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] GLoss < 0dB happen!!\n");
		result = 4;
		return result;
	}
	loss_db = 3 * halrf_psd_log2base(loss >> 13);
		
#if (DPK_PAS_CHK_DBG_8822E)
	RF_DBG(dm, DBG_RF_DPK, "[DPK] GLoss = %d.%02ddB\n",
	       (loss_db - 3870) / 100, (loss_db -3870) % 100);
#endif	
	if ((loss_db - 3870) > 1000) { /*GL > 10dB*/
		RF_DBG(dm, DBG_RF_DPK, "[DPK] GLoss > 10dB happen!!\n");
		result = 3;
		return result;
	} else if ((loss_db - 3870) < 250) { /*GL < 2.5dB*/
		RF_DBG(dm, DBG_RF_DPK, "[DPK] GLoss < 2.5dB happen!!\n");
		result = 4;
		return result;
	}  else
		return result;

	return result;
}

boolean _dpk_sync_8822e(
	struct dm_struct *dm,
	u8 path)
{
	boolean is_fail = false;

	_dpk_one_shot_8822e(dm, path, SYNC_DC);
	_dpk_one_shot_8822e(dm, path, DAGC);

	is_fail = _dpk_dc_corr_check_8822e(dm, path);
#if 0
	if (is_fail) {
		_dpk_rxdck_8822e(dm, path); /*re-do DCK for DPK*/
		_dpk_one_shot_8822e(dm, path, SYNC_DC);
		_dpk_one_shot_8822e(dm, path, DAGC);
		is_fail = _dpk_dc_corr_check_8822e(dm, path);
	}
#endif
	return is_fail;
}

u32 _dpk_lms_error_rpt_8822e(
	struct dm_struct *dm,
	u8 path)
{
	u32 err_val;

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
	odm_set_bb_reg(dm, 0x1bd4, 0x003F0000, 0x5); /*[21:16]*/
	err_val = odm_get_bb_reg(dm, 0x1bfc, 0x00003fff); /*[13:0]*/

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] S%d LMS err_val = 0x%x\n", path, err_val);	

	return err_val;
}

u8 _dpk_gainloss_8822e(
	struct dm_struct *dm,
	u8 path)
{
	_dpk_one_shot_8822e(dm, path, SYNC_DC);
	_dpk_one_shot_8822e(dm, path, DAGC);
	_dpk_one_shot_8822e(dm, path, MDPK_DC);
	_dpk_one_shot_8822e(dm, path, GAIN_LOSS);

	//_dpk_lms_error_rpt_8822e(dm, path);

	return _dpk_gainloss_result_8822e(dm, path);
}

u8 _dpk_pas_agc_8822e(
	struct dm_struct *dm,
	u8 path)
{
#ifdef  HALRF_DZ_LOG
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct halrf_rfk_dz_rpt *rfk_dz = &(rf->rfk_dz_rpt);
#endif
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 tmp_pga, tmp_txagc, tmp_gl_idx = 0, i = 0, gl_cnt = 0;
	u8 goout = 0, limited_pga = 0, agc_cnt = 0;
	boolean is_fail;
	u32 dgain;
	u8 pas_ind;

	do {
		switch (i) {
		case 0: /*SYNC*/
			is_fail = _dpk_sync_8822e(dm, path);
			dgain = _dpk_dgain_read_8822e(dm, path);

			tmp_pga = (u8)odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, 0x003e0);
			tmp_txagc = (u8)odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, 0x0001f);

			RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] Initial TXAGC / PGA = 0x%x / 0x%x\n",
				tmp_txagc, tmp_pga);

			if (is_fail) {
				_dpk_rxsram_8822e(dm, path);
				goout = 1;
				break;
			}

			if (agc_cnt == 0)
				_dpk_lbk_rxiqk_8822e(dm, path);

			if ((dgain > 1535) && !limited_pga) { /*DGain > 1535 happen*/
				RF_DBG(dm, DBG_RF_DPK, "[DPK] Small DGain!!\n");
				i = 2;
			} else if ((dgain < 768) && !limited_pga) { /*DGain < 768 happen*/
				RF_DBG(dm, DBG_RF_DPK, "[DPK] Large DGain!!\n");
				i = 1;
			} else
				i = 3;
			break;

		case 1: /*Gain > criterion*/
			if (tmp_pga > 0xe) {
				odm_set_rf_reg(dm, (enum rf_path)path,
					       RF_0x00, 0x003e0, 0xc);
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] PGA(-1) = 0xc\n");
			} else if ((0xb < tmp_pga) && (tmp_pga < 0xf)) {
				odm_set_rf_reg(dm, (enum rf_path)path,
					       RF_0x00, 0x003e0, 0x0);
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] PGA(-1) = 0x0\n");
			} else if (tmp_pga < 0xc) {
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] PGA@ lower bound!!\n");
				limited_pga = 1;
			}
			i = 0;
			agc_cnt++;
			break;

		case 2: /*Gain < criterion*/
			if (tmp_pga < 0xc) {
				odm_set_rf_reg(dm, (enum rf_path)path,
					       RF_0x00, 0x003e0, 0xc);
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] PGA(+1) = 0xc\n");
			} else if ((0xb < tmp_pga) && (tmp_pga < 0xf)) {
				odm_set_rf_reg(dm, (enum rf_path)path,
					       RF_0x00, 0x003e0, 0xf);
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] PGA(+1) = 0xf\n");
			} else if (tmp_pga > 0xe) {
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] PGA@ upper bound!!\n");
				limited_pga = 1;
			}
			i = 0;
			agc_cnt++;
			break;

		case 3: /*gainloss*/
			tmp_gl_idx = _dpk_gainloss_8822e(dm, path);

			if (DPK_PAS_DBG_8822E)
				_dpk_pas_read_8822e(dm, path, false);

			pas_ind = _dpk_pas_read_8822e(dm, path, true);

			if ((pas_ind == 2) && (tmp_gl_idx > 0))
				i = 5;
			else if ((tmp_gl_idx == 0 && pas_ind == 1) || tmp_gl_idx >= 5)
				i = 4;
			else if (tmp_gl_idx == 0)
				i = 5;
			else
				i = 6;

			gl_cnt++;
			break;

		case 4: /*GL > criterion*/
			if (tmp_txagc == 0x0) {
				goout = 1;
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] TXAGC@ lower bound!!\n");
				break;
			}
			tmp_txagc = tmp_txagc - 3;
			odm_set_rf_reg(dm, (enum rf_path)path,
				       RF_0x00, 0x0001f, tmp_txagc);
			RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] TXAGC (-3) = 0x%x\n",
			       tmp_txagc);
			limited_pga = 0;
			i = 3;
			agc_cnt++;
			break;

		case 5:	/*GL < criterion*/
			if (tmp_txagc == 0x1f) {
				goout = 1;
				RF_DBG(dm, DBG_RF_DPK,
				       "[DPK][AGC] TXAGC@ upper bound!!\n");
				break;
			}
			tmp_txagc = tmp_txagc + 2;
			odm_set_rf_reg(dm, (enum rf_path)path,
				       RF_0x00, 0x0001f, tmp_txagc);
			RF_DBG(dm, DBG_RF_DPK, "[DPK][AGC] TXAGC(+2) = 0x%x\n",
			       tmp_txagc);
			limited_pga = 0;
			i = 3;
			agc_cnt++;
			break;

		case 6:
			if ((tmp_txagc - tmp_gl_idx) <= 0)
				tmp_txagc = 0;
			else
				tmp_txagc = tmp_txagc - tmp_gl_idx;

			odm_set_rf_reg(dm, (enum rf_path)path,
				       RF_0x00, 0x0001f, tmp_txagc);

			odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
			odm_set_bb_reg(dm, R_0x1b64, MASKBYTE3, tmp_txagc); /*pwsf bnd*/
			dpk_info->dpk_txagc[path] = tmp_txagc;
			goout = 1;
			break;
		default:
			goout = 1;
			break;
		}	
	} while (!goout && (agc_cnt < 6));

	if (gl_cnt >= 6) {
		_dpk_pas_read_8822e(dm, path, false);
#ifdef  HALRF_DZ_LOG
		rfk_dz->dpk_dz_code |= BIT(8 * path + 2);
#endif
	}

	return is_fail;
}

void _dpk_coef_read_8822e(
	struct dm_struct *dm,
	u8 path)
{
	RF_DBG(dm, DBG_RF_DPK, "[DPK] -------- S%d Coef --------\n", path);

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2);
	
	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1bb4, BIT(24), 0x0);
		odm_set_bb_reg(dm, R_0x1b04, BIT(29) | BIT(28), 0x2);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x1bb4, BIT(24), 0x1);
		odm_set_bb_reg(dm, R_0x1b5c, BIT(29) | BIT(28), 0x2);
	}

	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x000400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x040400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x080400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x010400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x050400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x090400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x020400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x060400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0A0400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x030400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x070400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0B0400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0C0400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x100400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0D0400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x110400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0E0400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x120400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0F0400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x130400F0);
	RF_DBG(dm, DBG_RF_DPK, "[DPK][coef_r] 0x%08x\n",
		odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
}

void _dpk_fill_result_8822e(
	void *dm_void,
	u8 path,
	u8 result)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 dpk_txagc;

	dpk_txagc = (u8)odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, 0x0001f);

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);

	if (result) /*check PASS*/
		odm_write_1byte(dm, R_0x1b67, (u8)(dpk_txagc - 6)); /*ItQt -6dB*/
	else
		odm_write_1byte(dm, R_0x1b67, 0x00);	

	dpk_info->result[0] = dpk_info->result[0] | (result << path);
	dpk_info->dpk_txagc[path] = odm_read_1byte(dm, R_0x1b67);

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] S%d 0x1b67 = 0x%x\n", path, odm_read_1byte(dm, R_0x1b67));
}

boolean _dpk_mdpk_8822e(
	struct dm_struct *dm,
	u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	boolean is_fail = false;

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2);
	odm_set_bb_reg(dm, R_0x1bd4, 0x000000F0, 0xf); /*[7:4] force all clock on*/

	_dpk_one_shot_8822e(dm, path, SYNC_DC);
	_dpk_one_shot_8822e(dm, path, DAGC);
	_dpk_one_shot_8822e(dm, path, MDPK_DC);
	_dpk_one_shot_8822e(dm, path, DO_DPK);

	dpk_info->dpk_lms_err[path] = _dpk_lms_error_rpt_8822e(dm, path);
#if 0
	if (dpk_info->dpk_lms_error[path] > 0x500) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] LMS err_val Overlimit!!!\n");
		is_fail = true;
	}
#endif
	if (DPK_COEF_DBG_8822E)
		_dpk_coef_read_8822e(dm, path);

	/*set RX mode*/
	odm_set_rf_reg(dm, (enum rf_path)path, RF_0x00, 0xF0000, 0x3);

	return is_fail;
}

boolean _dpk_main_8822e(
	struct dm_struct *dm,
	u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	boolean is_fail;
	u8 t1, t2;

	dpk_info->thermal_dpk[path] = _dpk_thermal_read_8822e(dm, path);

	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal@K = %d\n", dpk_info->thermal_dpk[path]);

	_dpk_rf_setting_8822e(dm, path);
	_dpk_rxdck_8822e(dm, path); /*DCK for DPK*/

	t1 = _dpk_thermal_read_8822e(dm, path);
#if 1
	is_fail = _dpk_pas_agc_8822e(dm, path);
#else
	_dpk_dagc_8822e(dm, path);
	_dpk_gainloss_8822e(dm, path);
#endif
	if (!is_fail)
		is_fail = _dpk_mdpk_8822e(dm, path);

	t2 = _dpk_thermal_read_8822e(dm, path);

	dpk_info->thermal_dpk_delta[path] = HALRF_ABS(t2, t1);

	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal_delta = %d\n", dpk_info->thermal_dpk_delta[path]);

	return is_fail;
}

void _dpk_cal_gs_8822e(
	struct dm_struct *dm,
	u8 path)
{
#if 0
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u16 tmp_gs = 0;

	odm_set_bb_reg(dm, R_0x1b00, 0x0000000f, 0x8 | (path << 1));

	odm_set_bb_reg(dm, R_0x1b20, BIT(25), 0x0);	/*BypassDPD=0*/
	odm_set_bb_reg(dm, R_0x1b20, 0xc0000000, 0x0);	/* disable CFIR to TX*/
	odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x9);	/* ItQt shift 1 bit*/
	odm_set_bb_reg(dm, R_0x1bcc, BIT(21), 0x1);	/* inner loopback*/
	
	odm_set_bb_reg(dm, R_0x1b00, 0x0000000f, 0xc);
	odm_set_bb_reg(dm, R_0x1bd4, 0x000000f0, 0xf);

	if (path == RF_PATH_A) {
		/*manual pwsf+gs*/
		odm_set_bb_reg(dm, R_0x1b04, 0x0fffffff, 0x1066680);
		/*enable MDPD*/
		odm_set_bb_reg(dm, R_0x1b08, BIT(31), 0x1);
	} else {
		odm_set_bb_reg(dm, R_0x1b5c, 0x0fffffff, 0x1066680);
		odm_set_bb_reg(dm, R_0x1b60, BIT(31), 0x1);
	}
	
	if (dpk_info->dpk_bw == 0x1) { /*80M*/
		/*TPG DC*/
		odm_write_4byte(dm, R_0x1bf8, 0x80001310);
		odm_write_4byte(dm, R_0x1bf8, 0x00001310);
		odm_write_4byte(dm, R_0x1bf8, 0x810000DB);
		odm_write_4byte(dm, R_0x1bf8, 0x010000DB);
		odm_write_4byte(dm, R_0x1bf8, 0x0000B428);
		/*set TPG*/
		odm_write_4byte(dm, R_0x1bf4, 0x05020000 | (BIT(path) << 28));
	} else {
		odm_write_4byte(dm, R_0x1bf8, 0x8200190C);
		odm_write_4byte(dm, R_0x1bf8, 0x0200190C);
		odm_write_4byte(dm, R_0x1bf8, 0x8301EE14);
		odm_write_4byte(dm, R_0x1bf8, 0x0301EE14);
		odm_write_4byte(dm, R_0x1bf8, 0x0000B428);
		odm_write_4byte(dm, R_0x1bf4, 0x05020008 | (BIT(path) << 28));
	}

	odm_set_bb_reg(dm, R_0x1bb4, 0xff000000, 0x8 | path);

	_dpk_one_shot_8822e(dm, path, 0);

	/*restore*/
	odm_set_bb_reg(dm, R_0x1bf4, 0xff000000, 0x0); /*TPG off*/
	odm_set_bb_reg(dm, R_0x1b00, 0x0000000f, 0x8 | (path << 1));
	odm_set_bb_reg(dm, R_0x1bcc, 0xc000003f, 0x0);	/* ItQt*/
	odm_set_bb_reg(dm, R_0x1bcc, BIT(21), 0x0);	/* inner loopback off*/
	
	odm_set_bb_reg(dm, R_0x1b00, 0x0000000f, 0xc);

	if (path == RF_PATH_A)
		odm_set_bb_reg(dm, R_0x1b04, 0x0fffffff, 0x5b);
	else 
		odm_set_bb_reg(dm, R_0x1b5c, 0x0fffffff, 0x5b);

	odm_set_bb_reg(dm, R_0x1bd4, 0x001f0000, 0x0);

	tmp_gs = (u16)odm_get_bb_reg(dm, R_0x1bfc, 0x0FFF0000);
#if 0
	RF_DBG(dm, DBG_RF_DPK, "[DPK] tmp_gs = 0x%x\n", tmp_gs);
#endif
	tmp_gs = (tmp_gs * 910) >> 10; /*910 = 0x5b * 10*/

	if ((tmp_gs % 10) >= 5)
		tmp_gs = tmp_gs / 10 + 1;
	else
		tmp_gs = tmp_gs / 10;

	if (path == RF_PATH_A)
		odm_set_bb_reg(dm, R_0x1b04, 0x0fffffff, tmp_gs);
	else 
		odm_set_bb_reg(dm, R_0x1b5c, 0x0fffffff, tmp_gs);

	dpk_info->dpk_gs[path] = tmp_gs;
#endif
}

void _dpk_on_8822e(
	struct dm_struct *dm,
	u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	if (DPK_COEF_DBG_8822E)
		_dpk_coef_read_8822e(dm, path);

	_dpk_one_shot_8822e(dm, path, DPK_ON);

	/*KIP restore*/
	//odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2);
	//odm_set_bb_reg(dm, R_0x1bf4, BIT(29) | BIT(28), 0x0); /*tpg_sel*/

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
	odm_set_bb_reg(dm, R_0x1b20, BIT(31) | BIT(30), 0x0); /* disable CFIR to TX*/
	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000000); /* KIP POWER*/

	RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d DPD on & KIP restore!!!\n\n", path);
}

void _dpk_result_reset_8822e(
	struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 path;

	dpk_info->dpk_path_ok = 0x0;
	dpk_info->dpk_status = 0x0;
	dpk_info->result[0] = 0x0;

	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {
		dpk_info->dpk_txagc[path] = 0;
		dpk_info->dpk_gs[path] = 0x5b;
		dpk_info->pre_pwsf[path] = 0;
	}
}

void _dpk_by_path_8822e(
	struct dm_struct *dm,
	u8 path)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	boolean is_dpk_fail = 0;
	u8 retry_cnt;

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] =========== S%d DPK Start ===========\n", path);

	for (retry_cnt = 0; retry_cnt < 1; retry_cnt++) {

		RF_DBG(dm, DBG_RF_DPK, "[DPK] retry = %d\n", retry_cnt);

		is_dpk_fail = _dpk_main_8822e(dm, path);

		if (!is_dpk_fail)
			dpk_info->dpk_path_ok |= BIT(path);
	}

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] =========== S%d DPK Finish ==========\n", path);
}

void _dpk_path_select_8822e(
	struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	dpk_info->dpk_status = dpk_info->dpk_status | BIT(1);

#if (DPK_PATH_A_8822E)
	_dpk_by_path_8822e(dm, RF_PATH_A);
#endif

#if (DPK_PATH_B_8822E)
	_dpk_by_path_8822e(dm, RF_PATH_B);
#endif
#if (DPK_PATH_A_8822E)
	_dpk_on_8822e(dm, RF_PATH_A);
#endif
#if (DPK_PATH_B_8822E)
	_dpk_on_8822e(dm, RF_PATH_B);
#endif

	if (dpk_info->dpk_path_ok == 0x3)
		dpk_info->dpk_status = dpk_info->dpk_status | BIT(2);
}

void _dpk_count_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	dpk_info->dpk_cal_cnt += (dpk_info->dpk_status & BIT(1)) >> 1;
	dpk_info->dpk_ok_cnt += (dpk_info->dpk_status & BIT(2)) >> 2;
	dpk_info->dpk_reload_cnt += dpk_info->dpk_status & BIT(0);

	RF_DBG(dm, DBG_RF_DPK, "[DPK] Cal / OK / Reload = %d / %d / %d\n",
	       dpk_info->dpk_cal_cnt, dpk_info->dpk_ok_cnt, dpk_info->dpk_reload_cnt);
}

void _dpk_result_summary_8822e(
	struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 path;

	RF_DBG(dm, DBG_RF_DPK, "[DPK] ======== DPK Result Summary =======\n");

	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {

		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK] S%d dpk_txagc = 0x%x, gain scaling = 0x%x\n",
		       path, dpk_info->dpk_txagc[path], dpk_info->dpk_gs[path]);

		RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d DPK is %s\n", path,
		       ((dpk_info->dpk_path_ok & BIT(path)) >> path) ?
		       "Success" : "Fail");
	}

	RF_DBG(dm, DBG_RF_DPK, "[DPK] dpk_path_ok = 0x%x\n",
	       dpk_info->dpk_path_ok);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] ======== DPK Result Summary =======\n");

}

u8 _dpk_reload_index_8822e(
	struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u32 reg_rf18;
	u8 i = 99;

	reg_rf18 = odm_get_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK);

	if (reg_rf18 == dpk_info->dpk_rf18[0])
		i = 0;
	else if (reg_rf18 == dpk_info->dpk_rf18[1])
		i = 1;

	if (i != 99) {
		dpk_info->dpk_path_ok = dpk_info->result[i];
		dpk_info->dpk_band = (u8)((reg_rf18 & BIT(16)) >> 16);	/*0/1:G/A*/
		dpk_info->dpk_ch = (u8)(reg_rf18 & 0xff);
		dpk_info->dpk_bw = (u8)((reg_rf18 & 0x3000) >> 12);	/*3/2/1:20/40/80*/
	}

#if 1
	RF_DBG(dm, DBG_RF_DPK, "[DPK] Current RF0x18 = 0x%x, reload idx = %d\n",
	       reg_rf18, i);
#endif
	return i;
}

void _dpk_reload_data_8822e(
	void *dm_void,
	u8 reload_idx)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 path;

	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {

		odm_set_bb_reg(dm, R_0x1b00, 0x0000000f, 0x8 | (path << 1));
		if (dpk_info->dpk_band == 0x0) /*txagc bnd*/
			odm_set_bb_reg(dm, R_0x1b60, MASKDWORD, 0x1f100000);
		else
			odm_set_bb_reg(dm, R_0x1b60, MASKDWORD, 0x1f0d0000);

		odm_write_1byte(dm, R_0x1b67, dpk_info->dpk_txagc[path]);

		_dpk_coef_write_8822e(dm, path, (dpk_info->dpk_path_ok & BIT(path)) >> path);

		_dpk_one_shot_8822e(dm, path, DPK_ON);

		odm_set_bb_reg(dm, R_0x1b00, 0x0000000f, 0xc);

		if (path == RF_PATH_A)
			odm_set_bb_reg(dm, R_0x1b04, 0x0fffffff, dpk_info->dpk_gs[0]);
		else
			odm_set_bb_reg(dm, R_0x1b5c, 0x0fffffff, dpk_info->dpk_gs[1]);
	}
#endif
}

u32 _dpk_coef_transfer_8822e(struct dm_struct *dm)
{
    u32 reg_1bfc = 0;
    u16 coef_i = 0, coef_q = 0;

    reg_1bfc = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);

    coef_i = (u16)odm_get_bb_reg(dm, R_0x1bfc, MASKHWORD) & 0x1fff;
    coef_q = (u16)odm_get_bb_reg(dm, R_0x1bfc, MASKLWORD) & 0x1fff;

    coef_q = ((0x2000 - coef_q) & 0x1fff) - 1;

    reg_1bfc = (coef_i << 16) | coef_q;

    return reg_1bfc;
}

void
dpk_backup_coef_8822e(struct dm_struct *dm, u8 path)
{
    struct dm_dpk_info *dpk_info = &dm->dpk_info;

    odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0x0000000c);

    if (path == RF_PATH_A) {
     odm_set_bb_reg(dm, R_0x1bb4, MASKDWORD, 0x08000000);
     odm_set_bb_reg(dm, R_0x1b04, MASKDWORD, 0x2000005B);

//     odm_set_bb_reg(dm, R_0x1bb4, BIT(24), 0x0);
//        odm_set_bb_reg(dm, R_0x1b04, BIT(29) | BIT(28), 0x2);
    } else {
     odm_set_bb_reg(dm, R_0x1bb4, MASKDWORD, 0x09000000);
     odm_set_bb_reg(dm, R_0x1b5c, MASKDWORD, 0x2000005B );
//        odm_set_bb_reg(dm, R_0x1bb4, BIT(24), 0x1);
   //     odm_set_bb_reg(dm, R_0x1b5c, BIT(29) | BIT(28), 0x2);
    }
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x000400F0);
    dpk_info->coef[path][0] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x040400F0);
    dpk_info->coef[path][1] = _dpk_coef_transfer_8822e(dm);
     odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x080400F0);
    dpk_info->coef[path][2] = _dpk_coef_transfer_8822e(dm);
     odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x010400F0);
    dpk_info->coef[path][3] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x050400F0);
    dpk_info->coef[path][4] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x090400F0);
    dpk_info->coef[path][5] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x020400F0);
    dpk_info->coef[path][6] = _dpk_coef_transfer_8822e(dm);
     odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x060400F0);
    dpk_info->coef[path][7] = _dpk_coef_transfer_8822e(dm);
     odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0A0400F0);
    dpk_info->coef[path][8] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x030400F0);
    dpk_info->coef[path][9] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x070400F0);
    dpk_info->coef[path][10] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0B0400F0);
    dpk_info->coef[path][11] = _dpk_coef_transfer_8822e(dm);
     odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0C0400F0);
    dpk_info->coef[path][12] = _dpk_coef_transfer_8822e(dm);
     odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x100400F0);
    dpk_info->coef[path][13] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0D0400F0);
    dpk_info->coef[path][14] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x110400F0);
    dpk_info->coef[path][15] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0E0400F0);
    dpk_info->coef[path][16] = _dpk_coef_transfer_8822e(dm);
     odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x120400F0);
    dpk_info->coef[path][17] = _dpk_coef_transfer_8822e(dm);
     odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0F0400F0);
    dpk_info->coef[path][18] = _dpk_coef_transfer_8822e(dm);
    odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x130400F0);
    dpk_info->coef[path][19] = _dpk_coef_transfer_8822e(dm);
}

u8 dpk_reload_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 i;

	dpk_info->dpk_status = 0x0;

	if (dpk_info->dpk_rf18[0] == 0)
		return false; /*never do DPK before*/

	i = _dpk_reload_index_8822e(dm);

	if (i < DPK_RF18) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] DPK reload for RF0x18 0x%x!!\n", dpk_info->dpk_rf18[i]);
		_dpk_reload_data_8822e(dm, i);
		dpk_info->dpk_status = dpk_info->dpk_status | BIT(0);
	}

	return dpk_info->dpk_status;
}

void _dpk_force_bypass_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2); /*subpage 2*/

	odm_set_bb_reg(dm, R_0x1b08, BIT(15) | BIT(14), 0x3);
	odm_set_bb_reg(dm, R_0x1b04, 0x000000ff, 0x5b);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] S0 DPK bypass !!!\n");

	odm_set_bb_reg(dm, R_0x1b60, BIT(15) | BIT(14), 0x3);
	odm_set_bb_reg(dm, R_0x1b5c, 0x000000ff, 0x5b);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] S1 DPK bypass !!!\n");

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x0); /*subpage 0*/
}

void dpk_backup_8822e(struct dm_struct *dm)
{
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

    odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0x8);
    dpk_info->dpk_data[0] = odm_get_bb_reg(dm, 0x1b58, MASKDWORD);
    dpk_info->dpk_data[1] = odm_get_bb_reg(dm, 0x1b64, MASKDWORD);
    odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0xa);
    dpk_info->dpk_data[2] = odm_get_bb_reg(dm, 0x1b58, MASKDWORD);
    dpk_info->dpk_data[3] = odm_get_bb_reg(dm, 0x1b64, MASKDWORD);
    odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0xc);
    dpk_info->dpk_data[4] = odm_get_bb_reg(dm, 0x1b04, MASKDWORD);
    dpk_info->dpk_data[5] = odm_get_bb_reg(dm, 0x1b08, MASKDWORD);
    dpk_info->dpk_data[6] = odm_get_bb_reg(dm, 0x1b5c, MASKDWORD);
    dpk_info->dpk_data[7] = odm_get_bb_reg(dm, 0x1b60, MASKDWORD);
    dpk_info->dpk_data[8] = odm_get_bb_reg(dm, 0x1bb4, MASKDWORD);
    dpk_info->dpk_data[9] = odm_get_bb_reg(dm, 0x1bd4, MASKDWORD);
    dpk_info->dpk_data[10] = odm_get_bb_reg(dm, 0x1bf4, MASKDWORD);

    dpk_backup_coef_8822e(dm, 0);
    dpk_backup_coef_8822e(dm, 1);

}

void do_dpk_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	struct _hal_rf_ *rf = &dm->rf_table;

	u32 bb_reg_backup[DPK_BB_REG_NUM_8822E];
	u32 kip_reg_backup[DPK_KIP_REG_NUM_8822E][DPK_RF_PATH_NUM_8822E];
	u32 rf_reg_backup[DPK_RF_REG_NUM_8822E][DPK_RF_PATH_NUM_8822E];

	u32 bb_reg[DPK_BB_REG_NUM_8822E] = {
		R_0x520, R_0x1d58, R_0x1a00, R_0x1864, R_0x4164,
		R_0x180c, R_0x410c, R_0x186c, R_0x416c, R_0x1e70,
		R_0x80c, R_0x1d70, R_0x1e7c, R_0x18a4, R_0x41a4};
	u32 kip_reg[DPK_RF_REG_NUM_8822E] = {
		R_0x1b3c, R_0x1b70};
	u32 rf_reg[DPK_RF_REG_NUM_8822E] = {
		RF_0x1a, RF_0x55, RF_0x87, 0x90, RF_0xde, RF_0xdf};
	u8 path;

	if (rf->ext_pa && (*dm->band_type == ODM_BAND_2_4G)) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Skip DPK due to ext_PA exist!!\n");
		_dpk_force_bypass_8822e(dm);
		return;
	} else if (rf->ext_pa_5g && (*dm->band_type == ODM_BAND_5G)) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Skip DPK due to 5G_ext_PA exist!!\n");
		_dpk_force_bypass_8822e(dm);
		return;
	} else if (dm->rfe_type == 21 || dm->rfe_type == 22) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Skip DPK due RFE type = 21/22!!\n");
		_dpk_force_bypass_8822e(dm);
		return;
	} else if ((dm->rfe_type == 23 || dm->rfe_type == 24) && (*dm->band_type == ODM_BAND_2_4G)) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Skip 2.4G DPK due RFE type = 23/24!!\n");
		_dpk_force_bypass_8822e(dm);
		return;
	}

	RF_DBG(dm, DBG_RF_DPK,
	       "[DPK] ****** DPK Start (Ver: %s), Cv: %d ******\n",
	       DPK_VER_8822E, dm->cut_version);

	_dpk_information_8822e(dm);

	_backup_mac_bb_registers_8822e(dm, bb_reg, bb_reg_backup, DPK_BB_REG_NUM_8822E);
	_backup_rf_registers_8822e(dm, rf_reg, rf_reg_backup);
	_backup_kip_registers_8822e(dm, kip_reg, kip_reg_backup);

	_dpk_result_reset_8822e(dm);
	_dpk_mac_bb_setting_8822e(dm);
	_dpk_afe_setting_8822e(dm, true);
	_dpk_pre_setting_8822e(dm);

	_dpk_path_select_8822e(dm);
	_dpk_result_summary_8822e(dm);
	_dpk_count_8822e(dm);

	_dpk_afe_setting_8822e(dm, false);

	dpk_enable_disable_8822e(dm);

	dpk_backup_8822e(dm);

	_reload_kip_registers_8822e(dm, kip_reg, kip_reg_backup);
	_reload_rf_registers_8822e(dm, rf_reg, rf_reg_backup);

	btc_set_gnt_wl_bt_8822e(dm, true);
	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {
		odm_set_rf_reg(dm, (enum rf_path)path, 0x0, RFREG_MASK, 0x30000);
		halrf_set_rx_dck_8822e(dm, path); /*DCK for Rx*/
	}
	btc_set_gnt_wl_bt_8822e(dm, false);

	_reload_mac_bb_registers_8822e(dm, bb_reg, bb_reg_backup, DPK_BB_REG_NUM_8822E);
	_dpk_kip_clk_off_8822e(dm);
}

void dpk_enable_disable_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u8 val, path;

	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2); /*subpage 2*/

	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {
		if (dpk_info->is_dpk_enable & ((dpk_info->dpk_path_ok & BIT(path)) >> path))
			val = 0; /*enable*/
		else
			val = 1; /*disable*/

		if (path == RF_PATH_A) {
			odm_set_bb_reg(dm, R_0x1b08, BIT(15), val);
			odm_set_bb_reg(dm, R_0x1b04, 0x000000ff,
				       dpk_info->dpk_gs[RF_PATH_A]);
	} else {
			odm_set_bb_reg(dm, R_0x1b60, BIT(15), val);
			odm_set_bb_reg(dm, R_0x1b5c, 0x000000ff,
				       dpk_info->dpk_gs[RF_PATH_B]);
		}
		RF_DBG(dm, DBG_RF_DPK, "[DPK] S%d DPK %s !!!\n", path,
				val ? "disable" : "enable");
	}
	odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x0); /*subpage 0*/
}

void dpk_track_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	struct _hal_rf_ *rf = &dm->rf_table;

	u8 is_increase, i = 0, k = 0, path;
	u8 thermal_dpk_avg_count = 0, cur_thermal[2] = {0};
	u32 thermal_dpk_avg[2] = {0};
	s8 offset[2], delta_dpk[2];

	if ((dpk_info->thermal_dpk[0] == 0) && (dpk_info->thermal_dpk[1] == 0)) {
		RF_DBG(dm, DBG_RF_DPK_TRACK, "[DPK_track] Bypass DPK tracking!!!!\n");
		return;
	} else if (dm->rfe_type == 21 || dm->rfe_type == 22)
		return;
	else
		RF_DBG(dm, DBG_RF_DPK_TRACK,
		       "[DPK_track] ================[CH %d]================\n",
		       dpk_info->dpk_ch);

	/*get thermal meter*/
	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {
		cur_thermal[path] = _dpk_thermal_read_8822e(dm, path);

		RF_DBG(dm, DBG_RF_DPK_TRACK,
		       "[DPK_track] S%d thermal now = %d\n", path, cur_thermal[path]);
	}

	dpk_info->thermal_dpk_avg[0][dpk_info->thermal_dpk_avg_index] =
		cur_thermal[0];
	dpk_info->thermal_dpk_avg[1][dpk_info->thermal_dpk_avg_index] =
		cur_thermal[1];
	dpk_info->thermal_dpk_avg_index++;

	/*Average times */
	if (dpk_info->thermal_dpk_avg_index == THERMAL_DPK_AVG_NUM)
		dpk_info->thermal_dpk_avg_index = 0;

	for (i = 0; i < THERMAL_DPK_AVG_NUM; i++) {
		if (dpk_info->thermal_dpk_avg[0][i] ||
		    dpk_info->thermal_dpk_avg[1][i]) {
			thermal_dpk_avg[0] += dpk_info->thermal_dpk_avg[0][i];
			thermal_dpk_avg[1] += dpk_info->thermal_dpk_avg[1][i];
			thermal_dpk_avg_count++;
		}
	}

	/*Calculate Average ThermalValue after average enough times*/
	if (thermal_dpk_avg_count) {
#if 0
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK_track] S0 ThermalValue_DPK_AVG (count) = %d (%d))\n",
		       thermal_dpk_avg[0], thermal_dpk_avg_count);

		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK_track] S1 ThermalValue_DPK_AVG (count) = %d (%d))\n",
		       thermal_dpk_avg[1], thermal_dpk_avg_count);
#endif
		cur_thermal[0] = (u8)(thermal_dpk_avg[0] / thermal_dpk_avg_count);
		cur_thermal[1] = (u8)(thermal_dpk_avg[1] / thermal_dpk_avg_count);

		RF_DBG(dm, DBG_RF_DPK_TRACK,
		       "[DPK_track] S0 thermal avg = %d (DPK @ %d)\n",
		       cur_thermal[0], dpk_info->thermal_dpk[0]);

		RF_DBG(dm, DBG_RF_DPK_TRACK,
		       "[DPK_track] S1 thermal avg = %d (DPK @ %d)\n",
		       cur_thermal[1], dpk_info->thermal_dpk[1]);
	}

	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {
		delta_dpk[path] = dpk_info->thermal_dpk[path] - cur_thermal[path];

		offset[path] = (delta_dpk[path] - dpk_info->thermal_dpk_delta[path]) & 0x7f;

	RF_DBG(dm, DBG_RF_DPK_TRACK,
		       "[DPK_track] S%d thermal_diff= %d, cal_diff= %d, offset= %d\n",
		       path, delta_dpk[path], dpk_info->thermal_dpk_delta[path],
		       offset[path] > 64 ? offset[path] - 128 : offset[path]);
	}

	if (rf->is_dpk_in_progress || dm->rf_calibrate_info.is_iqk_in_progress ||
		rf->is_tssi_in_progress || rf->is_txgapk_in_progress ||
		rf->is_rxspurk_in_progress || odm_get_bb_reg(dm, R_0x1b68, BIT(0)) == 1)
		return;

#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
	if (*dm->is_fcs_mode_enable)
		return;
#endif

	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {
		if (offset[path] != dpk_info->pre_pwsf[path]) {
			odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
			odm_set_bb_reg(dm, R_0x1b58, 0x0000007f, offset[path]);
			dpk_info->pre_pwsf[path] = offset[path];
			RF_DBG(dm, DBG_RF_DPK_TRACK,
			       "[DPK_track] S%d new pwsf is 0x%x, 0x1b58=0x%x\n",
			       path, dpk_info->pre_pwsf[path],
			       odm_get_bb_reg(dm, R_0x1b58, MASKDWORD));
		} else
			RF_DBG(dm, DBG_RF_DPK_TRACK,
			       "[DPK_track] S%d pwsf unchanged (0x%x)\n",
			       path, dpk_info->pre_pwsf[path]);
	}
}

void dpk_info_by_8822e(
	void *dm_void,
	u32 *_used,
	char *output,
	u32 *_out_len)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u32 used = *_used;
	u32 out_len = *_out_len;
	u8 path, addr;

	PDM_SNPF(out_len, used, output + used, out_len - used, " %-25s = %d / %d\n",
		 "S0 DC (I/Q)", dpk_info->dc_i[0], dpk_info->dc_q[0]);

	PDM_SNPF(out_len, used, output + used, out_len - used, " %-25s = %d / %d\n",
		 "S0 Corr (idx/val)", dpk_info->corr_idx[0], dpk_info->corr_val[0]);

	PDM_SNPF(out_len, used, output + used, out_len - used, " %-25s = %d / %d\n",
		 "S1 DC (I/Q)", dpk_info->dc_i[1], dpk_info->dc_q[1]);

	PDM_SNPF(out_len, used, output + used, out_len - used, " %-25s = %d / %d\n",
		 "S1 Corr (idx/val)", dpk_info->corr_idx[1], dpk_info->corr_val[1]);

	PDM_SNPF(out_len, used, output + used, out_len - used, " %-25s = 0x%x / 0x%x\n",
		 "DPK LMS error (path)", dpk_info->dpk_lms_err[0], dpk_info->dpk_lms_err[1]);

	PDM_SNPF(out_len, used, output + used, out_len - used, " %-25s = 0x%x / 0x%x\n",
		 "DPK TxAGC (path)", dpk_info->dpk_txagc[0], dpk_info->dpk_txagc[1]);

	PDM_SNPF(out_len, used, output + used, out_len - used, " %-25s = 0x%x / 0x%x\n",
		 "DPK Gain Scaling (path)", dpk_info->dpk_gs[0], dpk_info->dpk_gs[1]);

	PDM_SNPF(out_len, used, output + used, out_len - used,
		 "\n==============[ Coef Read Start ]==============\n");

	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {
		odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2);
	
		if (path == RF_PATH_A) {
			odm_set_bb_reg(dm, R_0x1bb4, BIT(24), 0x0);
			odm_set_bb_reg(dm, R_0x1b04, BIT(29) | BIT(28), 0x2);
		} else if (path == RF_PATH_B) {
			odm_set_bb_reg(dm, R_0x1bb4, BIT(24), 0x1);
			odm_set_bb_reg(dm, R_0x1b5c, BIT(29) | BIT(28), 0x2);
		}

		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x000400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x040400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x080400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x010400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x050400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x090400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x020400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x060400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0A0400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x030400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x070400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0B0400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0C0400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x100400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0D0400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x110400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0E0400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x120400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0F0400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x130400F0);
		PDM_SNPF(out_len, used, output + used, out_len - used, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	}

	PDM_SNPF(out_len, used, output + used, out_len - used,
		 "==============[ Coef Read Finish ]==============\n");
	
	*_used = used;
	*_out_len = out_len;
}

void ex_dpk_info_by_8822e(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	u8 path, addr;

	RF_DBG(dm, DBG_RF_DZ_LOG, " %-25s = %d / %d\n",
		 "S0 DC (I/Q)", dpk_info->dc_i[0], dpk_info->dc_q[0]);

	RF_DBG(dm, DBG_RF_DZ_LOG,  " %-25s = %d / %d\n",
		 "S0 Corr (idx/val)", dpk_info->corr_idx[0], dpk_info->corr_val[0]);

	RF_DBG(dm, DBG_RF_DZ_LOG, " %-25s = %d / %d\n",
		 "S1 DC (I/Q)", dpk_info->dc_i[1], dpk_info->dc_q[1]);

	RF_DBG(dm, DBG_RF_DZ_LOG, " %-25s = %d / %d\n",
		 "S1 Corr (idx/val)", dpk_info->corr_idx[1], dpk_info->corr_val[1]);

	RF_DBG(dm, DBG_RF_DZ_LOG, " %-25s = 0x%x / 0x%x\n",
		 "DPK LMS error (path)", dpk_info->dpk_lms_err[0], dpk_info->dpk_lms_err[1]);

	RF_DBG(dm, DBG_RF_DZ_LOG, " %-25s = 0x%x / 0x%x\n",
		 "DPK TxAGC (path)", dpk_info->dpk_txagc[0], dpk_info->dpk_txagc[1]);

	RF_DBG(dm, DBG_RF_DZ_LOG, " %-25s = 0x%x / 0x%x\n",
		 "DPK Gain Scaling (path)", dpk_info->dpk_gs[0], dpk_info->dpk_gs[1]);

	RF_DBG(dm, DBG_RF_DZ_LOG, 
		 "\n==============[ Coef Read Start ]==============\n");

	for (path = 0; path < DPK_RF_PATH_NUM_8822E; path++) {
		odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), 0x2);
	
		if (path == RF_PATH_A) {
			odm_set_bb_reg(dm, R_0x1bb4, BIT(24), 0x0);
			odm_set_bb_reg(dm, R_0x1b04, BIT(29) | BIT(28), 0x2);
		} else if (path == RF_PATH_B) {
			odm_set_bb_reg(dm, R_0x1bb4, BIT(24), 0x1);
			odm_set_bb_reg(dm, R_0x1b5c, BIT(29) | BIT(28), 0x2);
		}

		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x000400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x040400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x080400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x010400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x050400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x090400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x020400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x060400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0A0400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x030400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x070400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0B0400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0C0400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x100400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0D0400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x110400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0E0400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x120400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x0F0400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
		odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x130400F0);
		RF_DBG(dm, DBG_RF_DZ_LOG, " Read S%d Coef = 0x%08x\n",
			 path, odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD));
	}

	RF_DBG(dm, DBG_RF_DZ_LOG,
		 "==============[ Coef Read Finish ]==============\n");
}

void dpk_info_rsvd_page_8822e(
	void *dm_void,
	u8 *buf,
	u32 *buf_size)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	u32 i;

	if (buf) {
		odm_move_memory(dm, buf, dpk_info->dpk_data, 44);
		odm_move_memory(dm, buf + 44, dpk_info->coef, 160);
	}
#if 0
	if (buf) {
		odm_move_memory(dm, buf, &(dpk_info->dpk_path_ok), 2);
		odm_move_memory(dm, buf + 2, dpk_info->dpk_txagc, 2);
		odm_move_memory(dm, buf + 4, dpk_info->dpk_gs, 4);
		odm_move_memory(dm, buf + 8, dpk_info->coef, 160);
		odm_move_memory(dm, buf + 168, &(dpk_info->dpk_ch), 1);
		odm_move_memory(dm, buf + 169, dpk_info->result, 2);
		odm_move_memory(dm, buf + 171, dpk_info->dpk_rf18, 8);
	}
#endif

	for (i = 0; i<11; i++) {
	RF_DBG(dm, DBG_RF_DPK, "[DPK] dpk_data %d=0x%x!!!\n", i, dpk_info->dpk_data[i]);
	}

	for (i = 0; i<20; i++) {
	RF_DBG(dm, DBG_RF_DPK, "[DPK] S0 c%d=0x%x!!!\n", i, dpk_info->coef[0][i]);
	}
	for (i = 0; i<20; i++) {
	RF_DBG(dm, DBG_RF_DPK, "[DPK] S1 c%d=0x%x!!!\n", i, dpk_info->coef[1][i]);
	}
	if (buf_size)
		*buf_size = DPK_INFO_RSVD_LEN_8822E;
}

void dpk_c2h_report_transfer_8822e(
	void	*dm_void,
	boolean is_ok,
	u8 *buf,
	u8 buf_size)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;
	struct dm_dpk_c2h_report dpk_c2h_report;

	u8 i, j, idx;
	u8 path_status = 0;

	if (!is_ok) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] FW DPK C2H data fail!!!\n");
		return;
	} else if (buf_size < DPK_C2H_REPORT_LEN_8822E) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] FW DPK: error buf size(%d)!!!\n", buf_size);
		return;
	} else if (!(dm->fw_offload_ability & PHYDM_RF_DPK_OFFLOAD)) {
		RF_DBG(dm, DBG_RF_DPK,
		       "[DPK] Skip FW DPK transfer (DPK OFFLOAD not support)\n");
		return;
	}

	RF_DBG(dm, DBG_RF_DPK, "[DPK] FW DPK data transfer start!!\n");

	/*copy C2H data to struct dpk_c2h_report*/
	for (i = 0; i < 2; i++) {
		odm_move_memory(dm, &(dpk_c2h_report.result[i]), buf + i, 1);

		for (j = 0; j < DPK_RF_PATH_NUM_8822E; j++) {
			odm_move_memory(dm, &(dpk_c2h_report.therm[i][j]),
					buf + 2 + i * 2 + j, 1);
			odm_move_memory(dm, &(dpk_c2h_report.therm_delta[i][j]),
					buf + 6 + i * 2 + j, 1);
		}

		odm_move_memory(dm, &(dpk_c2h_report.dpk_rf18[i]), buf + 10 + i * 4, 4);
	}
	odm_move_memory(dm, &dpk_c2h_report.dpk_status, buf + 18, 1);

	/*check abnormal*/
	if (dpk_c2h_report.dpk_rf18[0] == 0x0) {
		RF_DBG(dm, DBG_RF_DPK, "[DPK] Check C2H RF0x18 data fail!!!\n");
		return;
	}

	/*copy struct dpk_c2h_report to struct dpk_info*/
	dpk_info->dpk_status = dpk_c2h_report.dpk_status;

	_dpk_count_8822e(dm);

	for (i = 0; i < 2; i++) {
		dpk_info->dpk_rf18[i] = dpk_c2h_report.dpk_rf18[i];
		dpk_info->result[i] = dpk_c2h_report.result[i];
	}

	idx = _dpk_reload_index_8822e(dm);
	if (idx < 2)
		for (i = 0; i < DPK_RF_PATH_NUM_8822E; i++) {
			dpk_info->thermal_dpk[i] = dpk_c2h_report.therm[idx][i];
			dpk_info->thermal_dpk_delta[i] = dpk_c2h_report.therm_delta[idx][i];
		}
#if 0
	for (i = 0; i < DPK_C2H_REPORT_LEN_8822E; i++)
		RF_DBG(dm, DBG_RF_DPK, "[DPK] buf[%d] = 0x%x\n", i, *(buf + i));

	RF_DBG(dm, DBG_RF_DPK, "[DPK] result[0] = 0x%x\n", dpk_c2h_report.result[0]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] result[1] = 0x%x\n", dpk_c2h_report.result[1]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal_dpk[0][0] = 0x%x\n", dpk_c2h_report.therm[0][0]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal_dpk[0][1] = 0x%x\n", dpk_c2h_report.therm[0][1]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal_dpk[1][0] = 0x%x\n", dpk_c2h_report.therm[1][0]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal_dpk[1][1] = 0x%x\n", dpk_c2h_report.therm[1][1]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal_dpk_delta[0][0] = 0x%x\n", dpk_c2h_report.therm_delta[0][0]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal_dpk_delta[0][1] = 0x%x\n", dpk_c2h_report.therm_delta[0][1]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal_dpk_delta[1][0] = 0x%x\n", dpk_c2h_report.therm_delta[1][0]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] thermal_dpk_delta[1][1] = 0x%x\n", dpk_c2h_report.therm_delta[1][1]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] dpk_rf18[0] = 0x%x\n", dpk_c2h_report.dpk_rf18[0]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] dpk_rf18[1] = 0x%x\n", dpk_c2h_report.dpk_rf18[1]);
	RF_DBG(dm, DBG_RF_DPK, "[DPK] dpk_status = 0x%x\n", dpk_c2h_report.dpk_status);
#endif
}
#endif
