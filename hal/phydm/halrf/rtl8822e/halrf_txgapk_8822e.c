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

void _halrf_txgapk_backup_bb_registers_8822e(
	void *dm_void,
	u32 *reg,
	u32 *reg_backup,
	u32 reg_num)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i;

	for (i = 0; i < reg_num; i++) {
		reg_backup[i] = odm_get_bb_reg(dm, reg[i], MASKDWORD);

		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Backup BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
	}
}

void _halrf_txgapk_reload_bb_registers_8822e(
	void *dm_void,
	u32 *reg,
	u32 *reg_backup,
	u32 reg_num)

{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i;

	for (i = 0; i < reg_num; i++) {
		odm_set_bb_reg(dm, reg[i], MASKDWORD, reg_backup[i]);
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Reload BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
	}
}

void _halrf_txgapk_tx_pause_8822e(
	struct dm_struct *dm)
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

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Tx pause!!\n");
}

void _halrf_txgapk_bb_iqk_8822e(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);
	
	odm_set_bb_reg(dm, R_0x1e24, 0x00020000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x10000000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x20000000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x40000000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x80000000, 0x0);

	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1864, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x186c, 0x00000080, 0x1);
		odm_set_bb_reg(dm, R_0x180c, 0x00000003, 0x0);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x4164, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x416c, 0x00000080, 0x1);
		odm_set_bb_reg(dm, R_0x410c, 0x00000003, 0x0);
	}
	
	odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x2);
	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000080);
}

void _halrf_txgapk_afe_iqk_8822e(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x706f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70af0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bf0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cf0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70df0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ef0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ff0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ff0001);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x701f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x702f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x703f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x704f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x705f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x706f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x707f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x708f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x709f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70af0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70bf0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70cf0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70df0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ef0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ff0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ff0001);
	}

	halrf_ex_dac_fifo_rst(dm);
}

void _halrf_txgapk_afe_iqk_restore_8822e(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70144041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70244041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70344041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70444041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70644041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70144041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70244041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70344041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70444041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70644041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70fb8041);
	}

	halrf_ex_dac_fifo_rst(dm);
}

void _halrf_txgapk_bb_iqk_restore_8822e(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	odm_set_rf_reg(dm, path, RF_0xde, 0x10000, 0x0);
	odm_set_bb_reg(dm, R_0x1b00, 0x00000006, 0x0);
	odm_set_bb_reg(dm, R_0x1b08, MASKDWORD, 0x00000000);
#if 1
	odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x1);
	odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x0);
	odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x1);
#endif

	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1864, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x0);
		odm_set_bb_reg(dm, R_0x186c, 0x00000080, 0x0);
		odm_set_bb_reg(dm, R_0x180c, 0x00000003, 0x3);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x4164, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x0);
		odm_set_bb_reg(dm, R_0x416c, 0x00000080, 0x0);
		odm_set_bb_reg(dm, R_0x410c, 0x00000003, 0x3);
	}

	odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x0);
}

void _halrf_txgapk_write_gain_bb_table_8822e(
	void *dm_void)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	u8 channel = *dm->channel, i;
	u32 tmp_3f;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s channel=%d\n",
		__func__, channel);
	
	odm_set_bb_reg(dm, R_0x1b00, 0x00000006, 0x0);

	if (channel >= 1 && channel <= 14)
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x0);
	else if (channel >= 36 && channel <= 64)
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x2);
	else if (channel >= 100 && channel <= 144)
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x3);
	else if (channel >= 149 && channel <= 177)
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x4);

	odm_set_bb_reg(dm, R_0x1b9c, 0x000000ff, 0x88);

	for (i = 0; i < 11; i++) {
		tmp_3f = txgapk->txgapk_rf3f_bp[0][i][RF_PATH_A] & 0xfff;
		odm_set_bb_reg(dm, R_0x1b98, 0x00000fff, tmp_3f);
		odm_set_bb_reg(dm, R_0x1b98, 0x000f0000, i);
		odm_set_bb_reg(dm, R_0x1b98, 0x00008000, 0x1);
		odm_set_bb_reg(dm, R_0x1b98, 0x00008000, 0x0);

		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Set 0x1b98[11:0]=0x%03X   0x%x\n",
			tmp_3f, i);
	}
#else
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	u8 channel = *dm->channel, i;
	u8 path_idx, gain_idx, band_idx, check_txgain;
	u32 tmp_3f = 0;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s channel=%d\n",
		__func__, channel);

	for (band_idx = 0; band_idx < 5; band_idx++) {
		for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822E; path_idx++) {
			odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path_idx);

			//if (band_idx == 0 || band_idx == 1)	/*2G*/
			if (band_idx == 1)
				odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x0);
			else if (band_idx == 2)	/*5GL*/
				odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x2);
			else if (band_idx == 3)	/*5GM*/
				odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x3);
			else if (band_idx == 4)	/*5GH*/
				odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x4);

			odm_set_bb_reg(dm, R_0x1b9c, 0x000000ff, 0x88);

			check_txgain = 0;
			for (gain_idx = 0; gain_idx < 11; gain_idx++) {
#if 0
				if (((txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] & 0xf00) >> 8) >= 0xc &&
					((txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] & 0xf0) >> 4) >= 0xe) {
					if (check_txgain == 0) {
						tmp_3f = txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx];
						check_txgain = 1;
					}
					RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] tx_gain=0x%03X >= 0xCEX\n",
						txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx]);
				} else
					tmp_3f = txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] & 0xfff;
#else
				tmp_3f = txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] & 0xfff;
#endif
				odm_set_bb_reg(dm, R_0x1b98, 0x00000fff, tmp_3f);
				odm_set_bb_reg(dm, R_0x1b98, 0x000f0000, gain_idx);
				odm_set_bb_reg(dm, R_0x1b98, 0x00008000, 0x1);
				odm_set_bb_reg(dm, R_0x1b98, 0x00008000, 0x0);

				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Write Gain 0x1b98 Band=%d 0x1b98[11:0]=0x%03X path=%d\n",
					band_idx, tmp_3f, path_idx);
			}
		}
	}
#endif
}

void _halrf_txgapk_calculate_offset_8822e(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;

	u8 i;
	u8 channel = *dm->channel;

	u32 set_3rd_tx_gain[MAX_PATH_NUM_8822E] = {0x46f, 0xc69};
	u32 set_pi[MAX_PATH_NUM_8822E] = {R_0x1c, R_0xec};
	u32 backup_pi[MAX_PATH_NUM_8822E] = {0};
	u32 set_1b00_cfg1[MAX_PATH_NUM_8822E] = {0x00000d19, 0x00000d29};
	u32 tmp = 0;

	//u32 bb_reg[5] = {R_0x820, R_0x1e2c, R_0x1e28, R_0x1800, R_0x4100};
	//u32 bb_reg_backup[5] = {0};
	//u32 backup_num = 5;
	
	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s channel=%d\n",
		__func__, channel);

	//_halrf_txgapk_backup_bb_registers_8822e(dm, bb_reg, bb_reg_backup, backup_num);

	if (channel >= 1 && channel <= 14) {	/*2G*/
		btc_set_gnt_wl_bt_8822e(dm, true);

		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_rf_reg(dm, path, RF_0xde, 0x10000, 0x1);
		odm_set_rf_reg(dm, path, RF_0x00, 0xf0000, 0x5);
		odm_set_rf_reg(dm, path, RF_0x88, 0x00700, 0x1);
		odm_set_rf_reg(dm, path, RF_0xdf, 0x10000, 0x1);
		odm_set_rf_reg(dm, path, RF_0x87, 0xc0000, 0x3);
		odm_set_rf_reg(dm, path, RF_0x00, 0x003e0, 0x0f);
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x0);

		backup_pi[path] = odm_get_bb_reg(dm, set_pi[path], 0xc0000000);
		odm_set_bb_reg(dm, set_pi[path], 0xc0000000, 0x0);
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x12);
		odm_set_bb_reg(dm, R_0x1b2c, 0x00000fff, 0x038);
		odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, set_1b00_cfg1[path]);
		halrf_delay_1us(10000);
		odm_set_bb_reg(dm, set_pi[path], 0xc0000000, backup_pi[path]);

		for (i = 0; i < 30; i++) {
			halrf_delay_1us(100);
			tmp = odm_get_bb_reg(dm, R_0x2d9c, 0x000000ff);
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK]   0x2d9c[7:0](0x%x) = 0x55   delay %d*100us\n", tmp, i + 1);
			if (tmp == 0x55)
				break;
		}

		for (i = 0; i < 30; i++) {
			halrf_delay_1us(100);
			tmp = odm_get_bb_reg(dm, 0x1bfc, 0x0000ffff);
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK]   0x1bfc[15:0](0x%x) = 0x8000   delay %d*100us\n", tmp, i + 1);
			if (tmp == 0x8000) {
				odm_set_bb_reg(dm, R_0x1b10, 0x000000ff, 0x00);
				break;
			}
		}

		btc_set_gnt_wl_bt_8822e(dm, false);

		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, R_0x1bd4, 0x00200000, 0x1);
		odm_set_bb_reg(dm, R_0x1bd4, 0x001f0000, 0x12);

		odm_set_bb_reg(dm, R_0x1b9c, 0x00000f00, 0x3);
		tmp = odm_get_bb_reg(dm, R_0x1bfc, 0xffffffff);
		txgapk->offset[0][path] = (s8)(tmp & 0xf);
		txgapk->offset[1][path] = (s8)((tmp & 0xf0) >> 4);
		txgapk->offset[2][path] = (s8)((tmp & 0xf00) >> 8);
		txgapk->offset[3][path] = (s8)((tmp & 0xf000) >> 12);
		txgapk->offset[4][path] = (s8)((tmp & 0xf0000) >> 16);
		txgapk->offset[5][path] = (s8)((tmp & 0xf00000) >> 20);
		txgapk->offset[6][path] = (s8)((tmp & 0xf000000) >> 24);
		txgapk->offset[7][path] = (s8)((tmp & 0xf0000000) >> 28);

		odm_set_bb_reg(dm, R_0x1b9c, 0x00000f00, 0x4);
		tmp = odm_get_bb_reg(dm, R_0x1bfc, 0x000000ff);
		txgapk->offset[8][path] = (s8)(tmp & 0xf);
		txgapk->offset[9][path] = (s8)((tmp & 0xf0) >> 4);

		for (i = 0; i < 10; i++) {
			if (txgapk->offset[i][path] & BIT(3))
				txgapk->offset[i][path] = txgapk->offset[i][path] | 0xf0;
		}

		for (i = 0; i < 10; i++)
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] offset %d   %d   path=%d\n",
				txgapk->offset[i][path], i, path);

		RF_DBG(dm, DBG_RF_TXGAPK, "========================================\n");
	} else {	/*5G*/
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_rf_reg(dm, path, RF_0xde, 0x10000, 0x1);
		odm_set_rf_reg(dm, path, RF_0x00, 0xf0000, 0x5);
		odm_set_rf_reg(dm, path, RF_0x8b, 0x00700, 0x0);
		odm_set_rf_reg(dm, path, RF_0xdf, 0x20000, 0x1);
		odm_set_rf_reg(dm, path, 0x89, 0x00003, 0x3);
		odm_set_rf_reg(dm, path, RF_0x00, 0x003e0, 0x0f);

		if (channel >= 36 && channel <= 64)
			odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x2);
		else if (channel >= 100 && channel <= 144)
			odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x3);
		else if (channel >= 149 && channel <= 177) 
			odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x4);

		backup_pi[path] = odm_get_bb_reg(dm, set_pi[path], 0xc0000000);
		odm_set_bb_reg(dm, set_pi[path], 0xc0000000, 0x0);
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x12);
		odm_set_bb_reg(dm, R_0x1b2c, 0x00000fff, 0x038);
		odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, set_1b00_cfg1[path]);
		halrf_delay_1us(10000);
		odm_set_bb_reg(dm, set_pi[path], 0xc0000000, backup_pi[path]);

		for (i = 0; i < 30; i++) {
			halrf_delay_1us(100);
			tmp = odm_get_bb_reg(dm, R_0x2d9c, 0x000000ff);
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK]   0x2d9c[7:0](0x%x) = 0x55   delay %d*100us\n", tmp, i + 1);
			if (tmp == 0x55)
				break;
		}

		for (i = 0; i < 30; i++) {
			halrf_delay_1us(100);
			tmp = odm_get_bb_reg(dm, 0x1bfc, 0x0000ffff);
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK]   0x1bfc[15:0](0x%x) = 0x8000   delay %d*100us\n", tmp, i + 1);
			if (tmp == 0x8000) {
				odm_set_bb_reg(dm, R_0x1b10, 0x000000ff, 0x00);
				break;
			}
		}

		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, R_0x1bd4, 0x00200000, 0x1);
		odm_set_bb_reg(dm, R_0x1bd4, 0x001f0000, 0x12);

		odm_set_bb_reg(dm, R_0x1b9c, 0x00000f00, 0x3);
		tmp = odm_get_bb_reg(dm, R_0x1bfc, 0xffffffff);
		txgapk->offset[0][path] = (s8)(tmp & 0xf);
		txgapk->offset[1][path] = (s8)((tmp & 0xf0) >> 4);
		txgapk->offset[2][path] = (s8)((tmp & 0xf00) >> 8);
		txgapk->offset[3][path] = (s8)((tmp & 0xf000) >> 12);
		txgapk->offset[4][path] = (s8)((tmp & 0xf0000) >> 16);
		txgapk->offset[5][path] = (s8)((tmp & 0xf00000) >> 20);
		txgapk->offset[6][path] = (s8)((tmp & 0xf000000) >> 24);
		txgapk->offset[7][path] = (s8)((tmp & 0xf0000000) >> 28);

		odm_set_bb_reg(dm, R_0x1b9c, 0x00000f00, 0x4);
		tmp = odm_get_bb_reg(dm, R_0x1bfc, 0x000000ff);
		txgapk->offset[8][path] = (s8)(tmp & 0xf);
		txgapk->offset[9][path] = (s8)((tmp & 0xf0) >> 4);

		for (i = 0; i < 10; i++) {
			if (txgapk->offset[i][path] & BIT(3))
				txgapk->offset[i][path] = txgapk->offset[i][path] | 0xf0;
		}

		for (i = 0; i < 10; i++)
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] offset %d   %d   path=%d\n",
				txgapk->offset[i][path], i, path);

		RF_DBG(dm, DBG_RF_TXGAPK, "========================================\n");
	}
	//_halrf_txgapk_reload_bb_registers_8822e(dm, bb_reg, bb_reg_backup, backup_num);
}

void _halrf_txgapk_rf_restore_8822e(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	odm_set_rf_reg(dm, path, RF_0x0, 0xf0000, 0x3);
	odm_set_rf_reg(dm, path, RF_0xde, 0x10000, 0x0);
	odm_set_rf_reg(dm, path, RF_0xdf, 0x30000, 0x0);
}

u32 _halrf_txgapk_calculat_tx_gain_8822e(
	void *dm_void, u32 original_tx_gain, s8 offset)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	u32 modify_tx_gain = original_tx_gain;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

#if 0
	if (((original_tx_gain & 0xf00) >> 8) >= 0xc && ((original_tx_gain & 0xf0) >> 4) >= 0xe) {
		modify_tx_gain = original_tx_gain;
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] original_tx_gain=0x%03X(>=0xCEX) offset=%d modify_tx_gain=0x%03X\n",
			original_tx_gain, offset, modify_tx_gain);
		return modify_tx_gain;
	}
#endif

	if (offset < 0) {
		if ((offset % 2) == 0)
			modify_tx_gain = modify_tx_gain + (offset / 2);
		else {
			modify_tx_gain = modify_tx_gain + 0x1000 + (offset / 2) - 1;
		}
	} else {
		if ((offset % 2) == 0)
			modify_tx_gain = modify_tx_gain + (offset / 2);
		else {
			modify_tx_gain = modify_tx_gain + 0x1000 + (offset / 2);
		}
	}

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] original_tx_gain=0x%X offset=%d modify_tx_gain=0x%X\n",
		original_tx_gain, offset, modify_tx_gain);

	return modify_tx_gain;
}

void _halrf_txgapk_write_tx_gain_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;

	u32 i, j, tmp = 0x20, tmp1 = 0x60, tmp_3f;
	s8 offset_tmp[11] = {0};
	u8 channel = *dm->channel, path_idx, band_idx = 1;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (channel >= 1 && channel <= 14) {
		tmp = 0x20;	/*2G OFDM*/
		tmp1 = 0x60;	/*2G CCK*/
		band_idx = 1;
	} else if (channel >= 36 && channel <= 64) {
		tmp = 0x200;	/*5G L*/
		tmp1 = 0x0;
		band_idx = 2;
	} else if (channel >= 100 && channel <= 144) {
		tmp = 0x280;	/*5G M*/
		tmp1 = 0x0;
		band_idx = 3;
	} else if (channel >= 149 && channel <= 177) {
		tmp = 0x300;	/*5G H*/
		tmp1 = 0x0;
		band_idx = 4;
	}

	for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822E; path_idx++) {
		for (i = 0; i < 10; i++) {
#if 0
			offset_tmp[i] = 0;
			for (j = i; j <= 10; j++) {
				if ((((txgapk->txgapk_rf3f_bp[band_idx][j][path_idx] & 0xf00) >> 8) >= 0xc) &&
					(((txgapk->txgapk_rf3f_bp[band_idx][j][path_idx] & 0xf0) >> 4) >= 0xe))
					continue;

				offset_tmp[i] = offset_tmp[i] + txgapk->offset[j][path_idx];
				txgapk->fianl_offset[i][path_idx] = offset_tmp[i];
			}

			if ((((txgapk->txgapk_rf3f_bp[band_idx][i][path_idx] & 0xf00) >> 8) >= 0xc) &&
				(((txgapk->txgapk_rf3f_bp[band_idx][i][path_idx] & 0xf0) >> 4) >= 0xe))
				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] tx_gain=0x%03X >= 0xCEX\n",
					txgapk->txgapk_rf3f_bp[band_idx][i][path_idx]);
			else
				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Total offset %d   %d\n", offset_tmp[i], i);
#else
			offset_tmp[i] = 0;
			for (j = i; j < 10; j++) {
				if (txgapk->txgapk_rf3f_same[band_idx][j][path_idx])
					continue;

				offset_tmp[i] = offset_tmp[i] + txgapk->offset[j][path_idx];
				txgapk->fianl_offset[i][path_idx] = offset_tmp[i];
			}

			if (txgapk->txgapk_rf3f_same[band_idx][i][path_idx])
				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Total offset   %d   %d   tx_gain[11:5] the same 0x%x 0x%x\n",
					offset_tmp[i], i,
					txgapk->txgapk_rf3f_bp[band_idx][i][path_idx],
					txgapk->txgapk_rf3f_bp[band_idx][i + 1][path_idx]);
			else
				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Total offset   %d   %d\n", offset_tmp[i], i);
#endif
		}

		odm_set_rf_reg(dm, path_idx, 0xee, 0xfffff, 0x10000);

		j = 0;
		for (i = tmp; i <= (tmp + 10); i++) {
			odm_set_rf_reg(dm, path_idx, RF_0x33, 0xfffff, i);

			tmp_3f = _halrf_txgapk_calculat_tx_gain_8822e(dm,
					txgapk->txgapk_rf3f_bp[band_idx][j][path_idx], offset_tmp[j]);
			tmp_3f = tmp_3f & 0x01fff;
			odm_set_rf_reg(dm, path_idx, RF_0x3f, 0x7ffff, tmp_3f << 6);

			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x33=0x%05X   0x3f[18:6]=0x%05X   0x3f=0x%05X\n",
				i,
				odm_get_rf_reg(dm, path_idx, RF_0x3f, 0x7ffc0),
				odm_get_rf_reg(dm, path_idx, RF_0x3f, 0xfffff));
			j++;
		}

#if 0
		if (tmp1 == 0x60) {
			j = 0;
			for (i = tmp1; i <= (tmp1 + 10); i++) {
				odm_set_rf_reg(dm, path_idx, RF_0x33, 0xfffff, i);

				tmp_3f = _halrf_txgapk_calculat_tx_gain_8822e(dm,
					txgapk->txgapk_rf3f_bp[band_idx][j][path_idx], offset_tmp[j]);
				tmp_3f = tmp_3f & 0x01fff;
				odm_set_rf_reg(dm, path_idx, RF_0x3f, 0x7ffff, tmp_3f << 6);

				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x33=0x%05X   0x3f[18:6]=0x%05X   0x3f=0x%05X\n",
					i,
					odm_get_rf_reg(dm, path_idx, RF_0x3f, 0x7ffc0),
					odm_get_rf_reg(dm, path_idx, RF_0x3f, 0xfffff));
				j++;
			}
		}
#endif
		odm_set_rf_reg(dm, path_idx, 0xee, 0xfffff, 0x0);
	}
}

void _halrf_txgapk_disable_power_trim_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 path_idx;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822E; path_idx++) {
		odm_set_rf_reg(dm, path_idx, RF_0xde, BIT(9), 0x1);
		odm_set_rf_reg(dm, path_idx, RF_0x55, 0x000fc000, 0x0);
	}

}

void _halrf_txgapk_enable_power_trim_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 path_idx;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822E; path_idx++)
		odm_set_rf_reg(dm, path_idx, RF_0xde, BIT(9), 0x0);
}

void halrf_txgapk_save_all_tx_gain_table_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	u32 three_wire[2] = {R_0x180c, R_0x410c}, rf18;
	u8 ch_num[5] = {1, 1, 36, 100, 149};
	u8 ch_setting[5] = {0, 0, 1, 1, 1};
	u8 band_num[5] = {0x0, 0x0, 0x1, 0x3, 0x5};
	u8 cck[5] = {0x1, 0x0, 0x0, 0x0, 0x0};
	u8 path_idx, band_idx, gain_idx, rf0_idx;
	
	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (txgapk->read_txgain == 1) {
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Already Read txgapk->read_txgain return!!!\n");
		_halrf_txgapk_write_gain_bb_table_8822e(dm);
		return;
	}

	for (band_idx = 0; band_idx < 5; band_idx++) {
		for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822E; path_idx++) {
			rf18 = odm_get_rf_reg(dm, path_idx, RF_0x18, 0xfffff);

			odm_set_bb_reg(dm, three_wire[path_idx], 0x00000003, 0x0);

			odm_set_rf_reg(dm, path_idx, RF_0x18, 0x000ff, ch_num[band_idx]);
			odm_set_rf_reg(dm, path_idx, RF_0x18, 0x70000, band_num[band_idx]);
			odm_set_rf_reg(dm, path_idx, RF_0x18, 0x00100, ch_setting[band_idx]);
			odm_set_rf_reg(dm, path_idx, RF_0x1a, 0x00001, cck[band_idx]);
			odm_set_rf_reg(dm, path_idx, RF_0x1a, 0x10000, cck[band_idx]);

			gain_idx = 0;
			for (rf0_idx = 1; rf0_idx < 32; rf0_idx = rf0_idx + 3) {
				odm_set_rf_reg(dm, path_idx, 0x0, 0x000ff, rf0_idx);
				txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] = odm_get_rf_reg(dm, path_idx, 0x5f, 0xfffff);

				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x5f=0x%03X band_idx=%d path=%d\n",
					txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx], band_idx, path_idx);
				gain_idx++;
			}

			odm_set_rf_reg(dm, path_idx, RF_0x18, 0xfffff, rf18);
			odm_set_bb_reg(dm, three_wire[path_idx], 0x00000003, 0x3);
		}
	}

	_halrf_txgapk_write_gain_bb_table_8822e(dm);

	for (band_idx = 0; band_idx < 5; band_idx++) {
		for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822E; path_idx++) {
			for (gain_idx = 0; gain_idx < RF_GAIN_TABLE_NUM - 1; gain_idx++) {
				if ((txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] & 0xfe0) ==
					(txgapk->txgapk_rf3f_bp[band_idx][gain_idx + 1][path_idx] & 0xfe0))
					txgapk->txgapk_rf3f_same[band_idx][gain_idx][path_idx] = 1;
				else
					txgapk->txgapk_rf3f_same[band_idx][gain_idx][path_idx] = 0;

				RF_DBG(dm, DBG_RF_TXGAPK, "=======================================\n");
				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] txgapk->txgapk_rf3f_bp[band_idx=%d][gain_idx=%d][path_idx=%d]=0x%x(0x%x)\n",
					band_idx, gain_idx, path_idx,
					txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx],
					(txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] & 0xfe0));
				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] txgapk->txgapk_rf3f_bp[band_idx=%d][gain_idx+1=%d][path_idx=%d]=0x%x(0x%x)\n",
					band_idx, gain_idx + 1, path_idx,
					txgapk->txgapk_rf3f_bp[band_idx][gain_idx + 1][path_idx],
					(txgapk->txgapk_rf3f_bp[band_idx][gain_idx + 1][path_idx] & 0xfe0));
				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] txgapk->txgapk_rf3f_same[band_idx=%d][gain_idx=%d][path_idx=%d]=0x%x\n",
					band_idx, gain_idx, path_idx, txgapk->txgapk_rf3f_same[band_idx][gain_idx][path_idx]);
				RF_DBG(dm, DBG_RF_TXGAPK, "=======================================\n");
			}
		}
	}

	txgapk->read_txgain = 1;
}

void halrf_txgapk_reload_tx_gain_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;

	u32 i, j, tmp, tmp1;
	u8 path_idx, band_idx;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	for (band_idx = 1; band_idx <= 4; band_idx++) {
		if (band_idx == 1) {
			tmp = 0x20;	/*2G OFDM*/
			tmp1 = 0x60;	/*2G CCK*/
		} else if (band_idx == 2) {
			tmp = 0x200;	/*5G L*/
			tmp1 = 0x0;
		} else if (band_idx == 3) {
			tmp = 0x280;	/*5G M*/
			tmp1 = 0x0;
		} else if (band_idx == 4) {
			tmp = 0x300;	/*5G H*/
			tmp1 = 0x0;
		}

		for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822E; path_idx++) {
			odm_set_rf_reg(dm, path_idx, 0xee, 0xfffff, 0x10000);

			j = 0;
			for (i = tmp; i <= (tmp + 10); i++) {
				odm_set_rf_reg(dm, path_idx, RF_0x33, 0xfffff, i);

				odm_set_rf_reg(dm, path_idx, RF_0x3f, 0x7ffff,
					txgapk->txgapk_rf3f_bp[band_idx][j][path_idx] << 6);

				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x33=0x%05X   0x3f[18:6]=0x%03X   0x3f=0x%05X\n",
					i,
					txgapk->txgapk_rf3f_bp[band_idx][j][path_idx],
					odm_get_rf_reg(dm, path_idx, RF_0x3f, 0xfffff));
				j++;
			}

#if 0
			if (tmp1 == 0x60) {
				j = 0;
				for (i = tmp1; i <= (tmp1 + 10); i++) {
					odm_set_rf_reg(dm, path_idx, RF_0x33, 0xfffff, i);

					odm_set_rf_reg(dm, path_idx, RF_0x3f, 0x7ffff,
						txgapk->txgapk_rf3f_bp[band_idx][j][path_idx] << 6);

					RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x33=0x%05X   0x3f[18:6]=0x%04X   0x3f=0x%05X\n",
						i,
						txgapk->txgapk_rf3f_bp[band_idx][j][path_idx],
						odm_get_rf_reg(dm, path_idx, RF_0x3f, 0xfffff));
					j++;
				}
			}
#endif
			odm_set_rf_reg(dm, path_idx, 0xee, 0xfffff, 0x0);
		}
	}
}

void halrf_txgapk_8822e(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	u8 path_idx;
	u32 bb_reg_backup[2];
	u32 bb_reg[2] = {R_0x520, R_0x1e70};	/*TX Pause Regiter 0x520, 0x1e70*/

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (txgapk->read_txgain == 0) {
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] txgapk->read_txgain == 0 return!!!\n");
		return;
	}

	if (*dm->mp_mode == 1) {
		if (cali_info->txpowertrack_control == 2 ||
			cali_info->txpowertrack_control == 3 ||
			cali_info->txpowertrack_control == 4 ||
			cali_info->txpowertrack_control == 5) {
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] MP Mode in TSSI mode. return!!!\n");
			return;
		}
	} else {
		if (rf->power_track_type >= 4 && rf->power_track_type <= 7) {
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Normal Mode in TSSI mode. return!!!\n");
			return;
		}
	}

	rf->is_tssi_in_progress = 1;

	/*_halrf_txgapk_disable_power_trim_8822e(dm);*/

	_halrf_txgapk_backup_bb_registers_8822e(dm, bb_reg, bb_reg_backup, 2);

	_halrf_txgapk_tx_pause_8822e(dm);

	for (path_idx = 0; path_idx < MAX_PATH_NUM_8822E; path_idx++) {
		_halrf_txgapk_bb_iqk_8822e(dm, path_idx);
		_halrf_txgapk_afe_iqk_8822e(dm, path_idx);
		_halrf_txgapk_calculate_offset_8822e(dm, path_idx);
		_halrf_txgapk_rf_restore_8822e(dm, path_idx);
		_halrf_txgapk_afe_iqk_restore_8822e(dm, path_idx);
		_halrf_txgapk_bb_iqk_restore_8822e(dm, path_idx);
	}

	_halrf_txgapk_write_tx_gain_8822e(dm);

	_halrf_txgapk_reload_bb_registers_8822e(dm, bb_reg, bb_reg_backup, 2);

	/*_halrf_txgapk_enable_power_trim_8822e(dm);*/

	rf->is_tssi_in_progress = 0;
}


#endif
