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

#ifndef __odm_func_aon__
#define __odm_func_aon__
#endif

__odm_func_aon__ 
boolean _iqk_check_cal_8822e(
	struct dm_struct *dm,
	u8 path,
	u8 cmd)
{
	boolean notready = true, fail = true;
	u32 cnt1 = 0x0;
	u32 cnt2 = 0x0;

	//RF_DBG(dm, DBG_RF_IQK, "[IQK]===>%s\n", __func__);
	ODM_delay_us(1);
	while (notready) {
		if (odm_get_bb_reg(dm, 0x2d9c, 0x000000ff) == 0x55) {
			if (cmd == LOK1 || cmd == LOK2) /*LOK*/
				fail = false;
			else
				fail = (boolean)odm_get_bb_reg(dm, 0x1b08, BIT(26));
			notready = false;
		} else {
			ODM_delay_us(1);
			cnt1++;
		}

		if (cnt1 >= 3000) {
			fail = true;
			notready = false;
			RF_DBG(dm, DBG_RF_IQK, "[IQK]NCTL1 1 timeout!!!\n");
			break;
		}
	}	
	notready = true;
	cnt2 = 0;
	ODM_delay_us(1);
	while (notready) {
		if (odm_get_bb_reg(dm, 0x1bfc, 0x0000ffff) ==0x8000) {
			notready = false;
		} else {
			ODM_delay_us(1);
			cnt2++;
		}

		if (cnt2 >= 500) {
			fail = true;			
			notready = false;
			RF_DBG(dm, DBG_RF_IQK, "[IQK]NCTL2 timeout!!!\n");
			break;
		}
	}
	
	ODM_delay_us(50);
	odm_set_bb_reg(dm, 0x1b10, 0x000000ff, 0x0);
	odm_set_bb_reg(dm, 0x1b08, BIT(26),0x0);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]cnt1 = %d, cnt2 = %d, 0x1b08 = 0x%x!!!\n", cnt1, cnt2, odm_get_bb_reg(dm, 0x1b08, BIT(26)));

	return fail;
}

/*---------------------------Define Local Constant---------------------------*/

__odm_func_aon__
static u32 _iqk_btc_wait_indirect_reg_ready_8822e(struct dm_struct *dm)
{
	u32 delay_count = 0;
	
	/* wait for ready bit before access 0x1700 */
	while (1) {
		if ((odm_get_bb_reg(dm, 0x1700, 0xff000000) & BIT(5)) == 0) {
			ODM_delay_us(100);
			if (++delay_count >= 10)
			break;
		} else {
			break;
		}
	}
	
	return delay_count;
}

__odm_func_aon__
static u32 _iqk_btc_read_indirect_reg_8822e(struct dm_struct *dm, u16 reg_addr)
{
	/* wait for ready bit before access 0x1700 */
	_iqk_btc_wait_indirect_reg_ready_8822e(dm);

	odm_set_bb_reg(dm, 0x1700, MASKDWORD, 0x800F0000 | reg_addr);

	return odm_get_bb_reg(dm, 0x1708, MASKDWORD); /* get read data */
}

__odm_func_aon__
static void _iqk_btc_write_indirect_reg_8822e(struct dm_struct *dm, u16 reg_addr,
		       u32 bit_mask, u32 reg_value)
{
	u32 val, i = 0, bitpos = 0;

	if (bit_mask == 0x0)
		return;

	if (bit_mask == 0xffffffff) {
	/* wait for ready bit before access 0x1700 */
	_iqk_btc_wait_indirect_reg_ready_8822e(dm);

	/* put write data */
	odm_set_bb_reg(dm, 0x1704, MASKDWORD, reg_value);
	odm_set_bb_reg(dm, 0x1700, MASKDWORD, 0xc00F0000 | reg_addr);
	} else {
		for (i = 0; i <= 31; i++) {
			if (((bit_mask >> i) & 0x1) == 0x1) {
				bitpos = i;
				break;
			}
		}

		/* read back register value before write */
		val = _iqk_btc_read_indirect_reg_8822e(dm, reg_addr);
		val = (val & (~bit_mask)) | (reg_value << bitpos);

		/* wait for ready bit before access 0x1700 */
		_iqk_btc_wait_indirect_reg_ready_8822e(dm);

		odm_set_bb_reg(dm, 0x1704, MASKDWORD, val);
		odm_set_bb_reg(dm, 0x1700, MASKDWORD, 0xc00F0000 | reg_addr);
	}
}

__odm_func_aon__
void _iqk_set_gnt_wl_high_8822e(struct dm_struct *dm)
{
	u32 val = 0;
	u8 state = 0x1;

	/*GNT_WL = 1*/
	val = (state << 1) | 0x1;
	_iqk_btc_write_indirect_reg_8822e(dm, 0x38, 0xff00, 0x77); /*0x38[13:12]*/
	//_iqk_btc_write_indirect_reg_8822e(dm, 0x38, 0x0300, val); /*0x38[9:8]*/
}

__odm_func_aon__
void _iqk_set_gnt_bt_low_8822e(struct dm_struct *dm)
{
#if 0
	u32 val = 0;
	u8 state = 0x0, sw_control = 0x1;

	/*GNT_BT = 0*/
	val = (sw_control) ? ((state << 1) | 0x1) : 0;
	//_iqk_btc_write_indirect_reg_8822e(dm, 0x38, 0xc000, val); /*0x38[15:14]*/
	//_iqk_btc_write_indirect_reg_8822e(dm, 0x38, 0x0c00, val); /*0x38[11:10]*/
#endif
	return;
}

__odm_func_aon__
void _iqk_set_gnt_wl_gnt_bt_8822e(struct dm_struct *dm, boolean beforeK)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	if (beforeK) {
		_iqk_set_gnt_wl_high_8822e(dm);
		//_iqk_set_gnt_bt_low_8822e(dm);
	} else {
		_iqk_btc_write_indirect_reg_8822e(dm, 0x38, MASKDWORD, iqk_info->tmp_gntwl);
	}
}

__odm_func_aon__
void _iqk_backup_txcfir_8822e(struct dm_struct *dm, u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i = 0;
	u32 tmp = 0;

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1b20, 0xc0000000, 0x3);
	odm_set_bb_reg(dm, 0x1bd8, MASKDWORD, 0x0000071);
	odm_set_bb_reg(dm, 0x1bd4, 0x00200000, 0x1);
	odm_set_bb_reg(dm, 0x1bd4, 0x001f0000, 0x10);
	
	for (i = 0; i < 17; i++) {
		odm_set_bb_reg(dm, 0x1bd8, 0x01f00000, i);
		tmp = odm_get_bb_reg(dm, 0x1bfc, MASKDWORD);
		iqk_info->iqk_cfir_real[0][path][0][i] =
						(u16)((tmp & 0x0fff0000) >> 16);
		iqk_info->iqk_cfir_imag[0][path][0][i] = (u16)(tmp & 0x0fff);		
	}
	
#if 0
	for (i = 0; i < 17; i++)
		RF_DBG(dm, DBG_RF_IQK, "[IQK][BK]iqk_cfir_real[%x][%x][1][%x] = 0x%x\n", ch, path,i, iqk_info->iqk_cfir_real[ch][path][1][i]);
	for (i = 0; i < 17; i++)
		RF_DBG(dm, DBG_RF_IQK, "[IQK][BK]iqk_cfir_imag[%x][%x][1][%x] = 0x%x\n", ch, path,i, iqk_info->iqk_cfir_imag[ch][path][1][i]);
#endif
	odm_set_bb_reg(dm, 0x1b20, 0xc0000000, 0x0);
	odm_set_bb_reg(dm, 0x1bd8, MASKDWORD, 0x0000070);
}

__odm_func_aon__
void _iqk_backup_rxcfir_8822e(struct dm_struct *dm, u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 idx_h = 0;
	u32 tmp = 0;

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1b20, 0xc0000000, 0x1);
	odm_set_bb_reg(dm, 0x1bd8, MASKDWORD, 0x0000071);
	odm_set_bb_reg(dm, 0x1bd4, 0x00200000, 0x1);
	odm_set_bb_reg(dm, 0x1bd4, 0x001f0000, 0x10);
	for (idx_h = 0; idx_h < 17; idx_h++) {
		//odm_set_bb_reg(dm, 0x1bd8, MASKDWORD, i);
		odm_set_bb_reg(dm, R_0x1bd8, 0x01f00000, idx_h);
		tmp = odm_get_bb_reg(dm, 0x1bfc, MASKDWORD);
		iqk_info->iqk_cfir_real[0][path][1][idx_h] =
						(u16)((tmp & 0x0fff0000) >> 16);
		iqk_info->iqk_cfir_imag[0][path][1][idx_h] = (u16)(tmp & 0x0fff);		
	}
#if 0
	for (idx_h = 0; idx_h < 17; idx_h++)
		RF_DBG(dm, DBG_RF_IQK, "[IQK][BK]iqk_cfir_real[%x][%x][1][%x] = 0x%x\n", ch, path,idx_h, iqk_info->iqk_cfir_real[ch][path][1][idx_h]);
	for (idx_h = 0; idx_h < 17; idx_h++)
		RF_DBG(dm, DBG_RF_IQK, "[IQK][BK]iqk_cfir_imag[%x][%x][1][%x] = 0x%x\n", ch, path,idx_h, iqk_info->iqk_cfir_imag[ch][path][1][idx_h]);
#endif
	odm_set_bb_reg(dm, 0x1b20, 0xc0000000, 0x0);
	odm_set_bb_reg(dm, 0x1bd8, MASKDWORD, 0x0000070);

}

__odm_func_aon__
void _iqk_reload_txcfir_8822e(struct dm_struct *dm, u8 ch, u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	u8 i = 0x0;

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);

	odm_set_bb_reg(dm, 0x1bd8, 0x000000ff, 0x63);
	for (i = 0; i < 17; i++) {
		odm_set_bb_reg(dm, 0x1bd8, 0x01f00000, i);
		odm_set_bb_reg(dm, 0x1bd8, 0x000fff00, iqk_info->iqk_cfir_real[ch][path][0][i]);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK][BK]iqk_cfir_real[%x][%x][0][%x] = 0x%x\n", ch, path,i, iqk_info->iqk_cfir_real[ch][path][0][i]);
	}

	odm_set_bb_reg(dm, 0x1bd8, 0x000000ff, 0x61);
	for (i = 0; i < 17; i++) {		
		odm_set_bb_reg(dm, 0x1bd8, 0x01f00000, i);
		odm_set_bb_reg(dm, 0x1bd8, 0x000fff00, iqk_info->iqk_cfir_imag[ch][path][0][i]);		
		//RF_DBG(dm, DBG_RF_IQK, "[IQK][BK]iqk_cfir_imag[%x][%x][0][%x] = 0x%x\n", ch, path,i, iqk_info->iqk_cfir_imag[ch][path][0][i]);
	}		
	odm_set_bb_reg(dm, 0x1b20, 0xc0000000, 0x0);
	odm_set_bb_reg(dm, 0x1bd8, MASKDWORD, 0x0000070);

}

__odm_func_aon__
void _iqk_reload_rxcfir_8822e(struct dm_struct *dm, u8 ch, u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	u8 idx_h = 0x0;

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);

	odm_set_bb_reg(dm, 0x1bd8, 0x000000ff, 0x33);
	for (idx_h = 0; idx_h < 17; idx_h++) {
		odm_set_bb_reg(dm, 0x1bd8, 0x01f00000, idx_h);
		odm_set_bb_reg(dm, 0x1bd8, 0x000fff00, iqk_info->iqk_cfir_real[ch][path][1][idx_h]);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK][RS]iqk_cfir_real[%x][%x][1][%x] = 0x%x\n", ch, path,idx_h, iqk_info->iqk_cfir_real[ch][path][1][idx_h]);
	}

	odm_set_bb_reg(dm, 0x1bd8, 0x000000ff, 0x31);
	for (idx_h = 0; idx_h < 17; idx_h++) {		
		odm_set_bb_reg(dm, 0x1bd8, 0x01f00000, idx_h);
		odm_set_bb_reg(dm, 0x1bd8, 0x000fff00, iqk_info->iqk_cfir_imag[ch][path][1][idx_h]);	
		//RF_DBG(dm, DBG_RF_IQK, "[IQK][RS]iqk_cfir_imag[%x][%x][1][%x] = 0x%x\n", ch, path, idx_h, iqk_info->iqk_cfir_imag[ch][path][1][idx_h]);
	}		
	odm_set_bb_reg(dm, 0x1b20, 0xc0000000, 0x0);
	odm_set_bb_reg(dm, 0x1bd8, MASKDWORD, 0x0000070);
}

__odm_func_aon__
void _iqk_backup_cfir_8822e(struct dm_struct *dm, u8 path, u8 idx)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);	
	switch(idx) {
		case TXIQK:
		case NBTXK:
			iqk_info->txxy[0][path] = odm_get_bb_reg(dm, 0x1b38, 0xffffffff);
			_iqk_backup_txcfir_8822e(dm, path);
		break;
		case RXIQK:	
		case RXIQK2:
		case NBRXK:			
			iqk_info->rxxy[0][path] = odm_get_bb_reg(dm, 0x1b3c, 0xffffffff);
			_iqk_backup_rxcfir_8822e(dm, path);
		break;
	}

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);	
	iqk_info->cfir_en[0][path] = odm_get_bb_reg(dm, 0x1b70, 0xffffffff);
	iqk_info->iqk_tab[0] = iqk_info->rf_reg18;

}

__odm_func_aon__
void _iqk_reload_cfir_8822e(struct dm_struct *dm, u8 ch, u8 path, u8 idx)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);

	switch(idx) {
		case TX_IQK:
		case NBTXK:
			_iqk_reload_txcfir_8822e(dm, ch, path);
			odm_set_bb_reg(dm, 0x1b38, 0xffffffff, iqk_info->txxy[ch][path]);
		break;		
		case RX_IQK:	
		case NBRXK:			
			_iqk_reload_rxcfir_8822e(dm, ch, path);
			odm_set_bb_reg(dm, 0x1b3c, 0xffffffff, iqk_info->rxxy[ch][path]);
		break;
	}	
		odm_set_bb_reg(dm, 0x1b70, 0xffffffff, iqk_info->cfir_en[ch][path]);
}

__odm_func_aon__
void _iqk_backup_mac_bb_8822e(
	struct dm_struct *dm,
	u32 *MAC_backup,
	u32 *BB_backup,
	u32 *backup_mac_reg,
	u32 *backup_bb_reg)
{
	u32 i;
	for (i = 0; i < MAC_REG_NUM_8822E; i++){
		MAC_backup[i] = odm_get_mac_reg(dm, backup_mac_reg[i], MASKDWORD);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]Backup mac addr = %x, value =% x\n", backup_mac_reg[i], MAC_backup[i]);
	}
	for (i = 0; i < BB_REG_NUM_8822E; i++){		
		BB_backup[i] = odm_get_bb_reg(dm, backup_bb_reg[i], MASKDWORD);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]Backup bbaddr = %x, value =% x\n", backup_bb_reg[i], BB_backup[i]);
	}
	//RF_DBG(dm, DBG_RF_IQK, "[IQK]BackupMacBB Success!!!!\n"); 
}

__odm_func_aon__
void _iqk_backup_rf_8822e(
	struct dm_struct *dm,
	u32 RF_backup[][SS_8822E],
	u32 *backup_rf_reg)
{
	u32 i;

	for (i = 0; i < RF_REG_NUM_8822E; i++) {
		RF_backup[i][RF_PATH_A] = odm_get_rf_reg(dm, RF_PATH_A, backup_rf_reg[i], RFREGOFFSETMASK);
		RF_backup[i][RF_PATH_B] = odm_get_rf_reg(dm, RF_PATH_B, backup_rf_reg[i], RFREGOFFSETMASK);
		//RF_backup[i][RF_PATH_C] = odm_get_rf_reg(dm, RF_PATH_C, backup_rf_reg[i], RFREGOFFSETMASK);
		//RF_backup[i][RF_PATH_D] = odm_get_rf_reg(dm, RF_PATH_D, backup_rf_reg[i], RFREGOFFSETMASK);
	}
	//RF_DBG(dm, DBG_RF_IQK, "[IQK]BackupRF Success!!!!\n"); 
}

__odm_func_aon__
void _iqk_afe_restore_8822e(struct dm_struct *dm)
{
	//10_8822E_AFE_for_IQK_restore_reg
	//RF_DBG(dm, DBG_RF_IQK, "[IQK]_iqk_afe_restore_8822e\n");	
	//odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0xffa1005e);
	odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0x0);
	odm_set_bb_reg(dm, 0x1830, 0x40000000, 0x1);
	odm_set_bb_reg(dm, 0x4130, 0x40000000, 0x1);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70144001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70244001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70344001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70444001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x705b0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70644001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x707b0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x708f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x709f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70af0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70bf0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70cb0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70db0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70eb0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70fb0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70fb0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70144001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70244001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70344001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70444001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x705b0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70644001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x707b0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x708f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x709f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70af0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70bf0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70cb0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70db0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70eb0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70fb0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70fb0001);
	odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0xffa1005e);
	halrf_ex_dac_fifo_rst(dm);
}

__odm_func_aon__
void _iqk_afe_setting_8822e(struct dm_struct *dm)
{
	//RF_DBG(dm, DBG_RF_IQK, "[IQK]_iqk_afe_setting_8822e\n");
	//03_8822E_AFE_for_IQK
	odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0x00000000);
	odm_set_bb_reg(dm, 0x1830, 0x40000000, 0x0);
	odm_set_bb_reg(dm, 0x1860, 0xfffff000, 0xf0001);
	odm_set_bb_reg(dm, 0x4130, 0x40000000, 0x0);
	odm_set_bb_reg(dm, 0x4160, 0xfffff000, 0xf0001);
	//odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0xffffffff);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x701f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x702f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x703f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x704f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x705f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x706f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x707f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x708f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x709f0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70af0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70bf0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70cf0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70df0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70ef0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70ff0001);
	odm_set_bb_reg(dm, 0x1830, MASKDWORD, 0x70ff0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x700f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x701f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x702f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x703f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x704f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x705f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x706f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x707f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x708f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x709f0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70af0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70bf0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70cf0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70df0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70ef0001);
	odm_set_bb_reg(dm, 0x4130, MASKDWORD, 0x70ff0001);
	odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0xffffffff);
	halrf_ex_dac_fifo_rst(dm);
}

__odm_func_aon__
void _iqk_restore_mac_bb_8822e(
	struct dm_struct *dm,
	u32 *MAC_backup,
	u32 *BB_backup,
	u32 *backup_mac_reg,
	u32 *backup_bb_reg)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 i;	

	/*toggle IGI*/
	
	odm_set_bb_reg(dm, 0x1d70, MASKDWORD, 0x50505050);

	for (i = 0; i < MAC_REG_NUM_8822E; i++){		
		odm_set_mac_reg(dm, backup_mac_reg[i], MASKDWORD, MAC_backup[i]);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]restore mac = %x, value = %x\n",backup_mac_reg[i],MAC_backup[i]);
		}
	for (i = 0; i < BB_REG_NUM_8822E; i++){
		odm_set_bb_reg(dm, backup_bb_reg[i], MASKDWORD, BB_backup[i]);
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]restore bb = %x, value = %x\n",backup_bb_reg[i],BB_backup[i]);
		}
	/*rx go throughput IQK*/
#if 0
	odm_set_bb_reg(dm, 0x180c, BIT(31), 0x1);
	odm_set_bb_reg(dm, 0x410c, BIT(31), 0x1);
#else
	if (iqk_info->iqk_fail_report[0][0][RXIQK] == true) 
		odm_set_bb_reg(dm, 0x180c, BIT(31), 0x0);
	else
		odm_set_bb_reg(dm, 0x180c, BIT(31), 0x1);

	if (iqk_info->iqk_fail_report[0][1][RXIQK] == true) 
		odm_set_bb_reg(dm, 0x410c, BIT(31), 0x0);
	else
		odm_set_bb_reg(dm, 0x410c, BIT(31), 0x1);
#endif
	//odm_set_bb_reg(dm, 0x520c, BIT(31), 0x1);
	//odm_set_bb_reg(dm, 0x530c, BIT(31), 0x1);
	/*	RF_DBG(dm, DBG_RF_IQK, "[IQK]RestoreMacBB Success!!!!\n"); */
}

__odm_func_aon__
void _iqk_restore_rf_8822e(
	struct dm_struct *dm,
	u32 *rf_reg,
	u32 temp[][SS_8822E])
{
	u32 i;
	
	odm_set_rf_reg(dm, RF_PATH_A, 0xef, 0xfffff, 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, 0xef, 0xfffff, 0x0);
	odm_set_rf_reg(dm, RF_PATH_A, 0xdf, 0x00010, 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, 0xdf, 0x00010, 0x0);

	/*0xdf[4]=0*/
	//_iqk_rf_set_check_8822e(dm, RF_PATH_A, 0xdf, temp[0][RF_PATH_A] & (~BIT(4)));
	//_iqk_rf_set_check_8822e(dm, RF_PATH_B, 0xdf, temp[0][RF_PATH_B] & (~BIT(4)));

	for (i = 0; i < RF_REG_NUM_8822E; i++) {
		odm_set_rf_reg(dm, RF_PATH_A, rf_reg[i],
			       0xfffff, temp[i][RF_PATH_A]);
		odm_set_rf_reg(dm, RF_PATH_B, rf_reg[i],
			       0xfffff, temp[i][RF_PATH_B]);
	}
	
	odm_set_rf_reg(dm, RF_PATH_A, 0xde, BIT(16), 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, 0xde, BIT(16), 0x0);
	//RF_DBG(dm, DBG_RF_IQK, "[IQK]RestoreRF Success!!!!\n"); 
}

__odm_func_aon__
void _iqk_switch_table_8822e(struct dm_struct *dm)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i, j, k, q;
	RF_DBG(dm, DBG_RF_IQK, "[IQK]_iqk_switch_table_8822e!!!!\n");

		iqk_info->iqk_tab[1] = iqk_info->iqk_tab[0];
		for (i = 0; i < SS_8822E; i++) {
			iqk_info->lok_idac[1][i] = iqk_info->lok_idac[0][i];
			iqk_info->txxy[1][i] = iqk_info->txxy[0][i];
			iqk_info->rxxy[1][i] = iqk_info->rxxy[0][i];
			iqk_info->cfir_en[1][i] = iqk_info->cfir_en[0][i];			
			for (j = 0; j < 2; j++) {
				iqk_info->iqk_fail_report[1][i][j] = iqk_info->iqk_fail_report[0][i][j];			
				for (k = 0; k < 17; k++) {
					iqk_info->iqk_cfir_real[1][i][j][k] = iqk_info->iqk_cfir_real[0][i][j][k];
					iqk_info->iqk_cfir_imag[1][i][j][k] = iqk_info->iqk_cfir_imag[0][i][j][k];
				}
			}
		}
	
#if 0
			for(q = 0; q < 2; q++) {
				RF_DBG(dm, DBG_RF_IQK, "[IQK](a)iqk_info->iqk_tab[%x]					 = %2x \n", q, iqk_info->iqk_tab[q]);
				for(i = 0; i < 2; i++) {//path	
					RF_DBG(dm, DBG_RF_IQK, "[IQK](a)iqk_info->cfir_en[%x][%x]			 = %2x \n", q, i, iqk_info->cfir_en[q][i]); 	
					RF_DBG(dm, DBG_RF_IQK, "[IQK](a)iqk_info->txxy[%x][%x] 				 = %2x \n", q, i, iqk_info->txxy[q][i]);		
					RF_DBG(dm, DBG_RF_IQK, "[IQK](a)iqk_info->rxxy[%x][%x] 				 = %2x \n", q, i, iqk_info->rxxy[q][i]);
					RF_DBG(dm, DBG_RF_IQK, "[IQK](a)iqk_info->lok_idac[%x][%x] 			 = %2x \n", q, i, iqk_info->lok_idac[q][i]);

					for(j = 0; j < 2; j++) { //trx				
						for (k=0 ; k < 17; k++){ //real
					//RF_DBG(dm, DBG_RF_IQK, "[IQK](a)iqk_info->iqk_cfir_real[%x][%x][%x][%x] = %2x \n", q, i, j, k, iqk_info->iqk_cfir_real[q][i][j][k]);
						}
					//		RF_DBG(dm, DBG_RF_IQK, "[IQK]\n");
						for (k = 0 ; k<17; k++){ //imag
					//RF_DBG(dm, DBG_RF_IQK, "[IQK](a)iqk_info->iqk_cfir_imag[%x][%x][%x][%x] = %2x \n", q, i, j, k, iqk_info->iqk_cfir_imag[q][i][j][k]);
						}
					}
						RF_DBG(dm, DBG_RF_IQK, "[IQK]\n");
				}
			}
#endif

}

__odm_func_aon__
void _iqk_preset_8822e(
	struct dm_struct *dm)
{
#if 0
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]_iqk_preset_8822e!!!!\n");

	//LCK_22E_and_DACK_defaultValue
	odm_set_bb_reg(dm, 0x3804, 0x3ff00000, 0x058);
	odm_set_bb_reg(dm, 0x3830, 0x3ff00000, 0x058);
	odm_set_bb_reg(dm, 0x3904, 0x3ff00000, 0x058);
	odm_set_bb_reg(dm, 0x3930, 0x3ff00000, 0x058);
	odm_set_rf_reg(dm, RF_PATH_A, 0xb1, 0x04000, 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, 0x18, 0x08000, 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, 0xb1, 0x04000, 0x0);
#endif
}
__odm_func_aon__
void _iqk_macbb_setting_8822e(
	struct dm_struct *dm)
{
	RF_DBG(dm, DBG_RF_IQK, "[IQK]_iqk_macbb_8822e!!!!\n");
	/*rx path on*/
#if 1
	odm_set_bb_reg(dm, 0x0824, 0x00030000, 0x3);
#else
	//phydm_config_rx_path_8822e(dm, 0x3); // for pathA/B
#endif
	//odm_write_1byte(dm, R_0x522, 0xff);
	odm_set_mac_reg(dm, 0x520, 0x00ff0000, 0xff);	

	odm_set_bb_reg(dm, R_0x1e70, 0x0000000f, 0x2); /*hw tx stop*/
	ODM_delay_us(10);
	//0x73[2] = 1 (PTA control path is at WLAN)
	odm_set_bb_reg(dm, 0x70, 0xff000000, 0x06);	
	//02_8822E_BB_for_IQK
	odm_set_bb_reg(dm, 0x1e24, 0x00020000, 0x1);
	odm_set_bb_reg(dm, 0x1cd0, 0x10000000, 0x1);
	odm_set_bb_reg(dm, 0x1cd0, 0x20000000, 0x1);
	odm_set_bb_reg(dm, 0x1cd0, 0x40000000, 0x1);
	odm_set_bb_reg(dm, 0x1cd0, 0x80000000, 0x0);
	//odm_set_bb_reg(dm, 0x1c68, 0x0f000000, 0xf);
	odm_set_bb_reg(dm, 0x1864, 0x80000000, 0x1);
	odm_set_bb_reg(dm, 0x4164, 0x80000000, 0x1);
	odm_set_bb_reg(dm, 0x180c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, 0x410c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, 0x186c, 0x00000080, 0x1);
	odm_set_bb_reg(dm, 0x416c, 0x00000080, 0x1);
	odm_set_bb_reg(dm, 0x180c, 0x00000003, 0x0);
	odm_set_bb_reg(dm, 0x410c, 0x00000003, 0x0);
	odm_set_bb_reg(dm, 0x1a00, 0x00000003, 0x2);
	odm_set_bb_reg(dm, 0x1b08, MASKDWORD, 0x00000080);
}

__odm_func_aon__
void _iqk_macbb_restore_8822e(
	struct dm_struct *dm)
{
	//RF_DBG(dm, DBG_RF_IQK, "[IQK]_iqk_macbb_restore_8822e\n");
	//11_8822E_BB_for_IQK_restore
	odm_set_rf_reg(dm, RF_PATH_A, 0xde, 0x10000, 0x0);
	odm_set_rf_reg(dm, RF_PATH_B, 0xde, 0x10000, 0x0);
#if 1
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x0);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x00);
	odm_set_bb_reg(dm, 0x1b20, BIT(25), 0x00);
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x1);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x00);	
	odm_set_bb_reg(dm, 0x1b20, BIT(25), 0x00);
#endif
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x0);
	odm_set_bb_reg(dm, 0x1b08, MASKDWORD, 0x00000000);
	//odm_set_bb_reg(dm, 0x1c68, 0x0f000000, 0x0);
	odm_set_bb_reg(dm, 0x1d0c, 0x00010000, 0x1);
	odm_set_bb_reg(dm, 0x1d0c, 0x00010000, 0x0);
	odm_set_bb_reg(dm, 0x1d0c, 0x00010000, 0x1);
	odm_set_bb_reg(dm, 0x1864, 0x80000000, 0x0);
	odm_set_bb_reg(dm, 0x4164, 0x80000000, 0x0);
	odm_set_bb_reg(dm, 0x180c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, 0x410c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, 0x186c, 0x00000080, 0x0);
	odm_set_bb_reg(dm, 0x416c, 0x00000080, 0x0);
	odm_set_bb_reg(dm, 0x180c, 0x00000003, 0x3);
	odm_set_bb_reg(dm, 0x410c, 0x00000003, 0x3);
	odm_set_bb_reg(dm, 0x1a00, 0x00000003, 0x0);
}

__odm_func_aon__
void _iqk_reload_lok_setting_8822e(
	struct dm_struct *dm,
	u8 path)
{
#if 1
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 tmp;
	u8 idac_i, idac_q;

	idac_i = (u8)((iqk_info->rf_reg58 & 0xfc000) >> 14);
	idac_q = (u8)((iqk_info->rf_reg58 & 0x3f00) >> 8);
	odm_set_rf_reg(dm, (enum rf_path)path, 0xdf, BIT(4), 0x0);//W LOK table
	odm_set_rf_reg(dm, (enum rf_path)path, 0xef, BIT(4), 0x1);

	if (*dm->band_type == ODM_BAND_2_4G)
		odm_set_rf_reg(dm, (enum rf_path)path, 0x33, 0x7f, 0x00);
	else
		odm_set_rf_reg(dm, (enum rf_path)path, 0x33, 0x7f, 0x20);

	odm_set_rf_reg(dm, (enum rf_path)path, 0x08, 0xfc000, idac_i);
	odm_set_rf_reg(dm, (enum rf_path)path, 0x08, 0x003f0, idac_q);
	odm_set_rf_reg(dm, (enum rf_path)path, 0xef, BIT(4), 0x0);// stop write
	
	tmp = odm_get_rf_reg(dm, (enum rf_path)path, 0x58, 0xfffff);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]S%d,reload 0x58 = 0x%x\n", path, tmp);
#endif
}

__odm_func_aon__
boolean _lok1_check_8822e(struct dm_struct *dm, u8 path)
{
	u32 temp;
	u8 idac_i, idac_q;

	//RF_DBG(dm, DBG_RF_IQK, "[IQK]===>%s\n", __func__);
	temp = odm_get_rf_reg(dm,path, 0x58, 0xfffff);
	
	idac_i = (u8)((temp & 0xf8000) >> 15);
	idac_q = (u8)((temp & 0x07c00) >> 10);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, 0x58 = 0x%x\n", path, odm_get_rf_reg(dm, path, 0x58, 0xfffff));

	if (idac_i <= 0x3 || idac_i >= 0x1c || idac_q <= 0x3 || idac_q >= 0x1c)
		return true;
	else
		return false;

}

__odm_func_aon__
boolean _iqk_one_shot_8822e(
	struct dm_struct *dm,
	u8 path,
	u8 idx)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean fail = true;
	u32 IQK_CMD = 0x0;
		
	switch(idx) {
	case NBTXK:
		RF_DBG(dm, DBG_RF_IQK, "[IQK]============ S%d NBTXIQK ============\n", path);		
		odm_set_bb_reg(dm, 0x1b2c, 0x00000fff, 0x0c);
		IQK_CMD = 0x200 | (1 << (4 + path)) | 0x8;		
		break;
	case NBRXK:		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]============ S%d NBRXIQK ============\n", path);		
		odm_set_bb_reg(dm, 0x1b2c, 0x0fff0000, 0x18);
		IQK_CMD = 0x300 | (1 << (4 + path)) | 0x8;		
		break;
	case TXIQK:		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]============ S%d WBTXIQK ============\n", path);
		IQK_CMD = ((iqk_info->iqk_bw + 4) << 8) | (1 << (path + 4)) | 0x8;
		break;	
	case RXIQK1:		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]============ S%d WBRXIQK1 ============\n", path);
		IQK_CMD = 0xf00 | (1 << (4 + path)) | 0x8;		
		break;	
	case RXIQK2:		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]============ S%d WBRXIQK2 ============\n", path);
		IQK_CMD = ((iqk_info->iqk_bw + 0xa) << 8) | (1 << (path + 4)) | 0x8;
		break;	
	case LOK1:
		RF_DBG(dm, DBG_RF_IQK, "[IQK]======S%d LOK1======\n", path);
		IQK_CMD = (1 << (4 + path)) | 0x8;
		break;	
	case LOK2:
		RF_DBG(dm, DBG_RF_IQK, "[IQK]======S%d LOK2======\n", path);
		IQK_CMD = 0x100 | (1 << (4 + path)) | 0x8;
		break;
	}
	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Trigger CMD = 0x%x\n", IQK_CMD + 1);
	odm_set_bb_reg(dm, 0x1b00, MASKDWORD, IQK_CMD);
	odm_set_bb_reg(dm, 0x1b00, MASKDWORD, IQK_CMD + 1);
	fail = _iqk_check_cal_8822e(dm, path, 0x1);

	return fail;
}

__odm_func_aon__
void _iqk_txxym_dump_8822e(struct dm_struct *dm, u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u32 reg1b14, reg1b1c, reg1b38;
	u32 i = 0;
	u32 low_bnd = 0, high_bnd = 0;

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	reg1b14 = odm_get_bb_reg(dm, 0x1b14, MASKDWORD);
	reg1b1c = odm_get_bb_reg(dm, 0x1b1c, MASKDWORD);
	reg1b38 = odm_get_bb_reg(dm, 0x1b38, MASKDWORD);

	odm_set_bb_reg(dm, 0x1b1c, 0x00000003, 0x2);

	/*	
	XYM6~XYM23 --> TRX BW80
	XYM10~XYM19 --> TRX BW40
	XYM12~XYM17 --> TRX BW20
	*/

    switch ((iqk_info->rf_reg18 & 0x3000) >> 12) {
    case 0x3: //20M
		low_bnd = 12;
		high_bnd = 17;
        break;
    case 0x2: //40M
		low_bnd = 10;
		high_bnd = 19;
        break;
    case 0x1: //80M
		low_bnd = 6;
		high_bnd = 23;
        break;
    default:
		low_bnd = 6;
		high_bnd = 23;
        break;
    }

	for(i = low_bnd; i <= high_bnd; i++) {
		odm_set_bb_reg(dm, 0x1b14, MASKDWORD, 0x000000e0 + i);
		odm_set_bb_reg(dm, 0x1b14, MASKDWORD, 0x00000000);		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, TXXYM%d = 0x%x\n", path, i, odm_get_bb_reg(dm, 0x1b38, MASKDWORD));
	}
	odm_set_bb_reg(dm, 0x1b14, MASKDWORD, reg1b14);
	odm_set_bb_reg(dm, 0x1b1c, MASKDWORD, reg1b1c);	
	odm_set_bb_reg(dm, 0x1b38, MASKDWORD, reg1b38);
}

__odm_func_aon__
boolean _iqk_5g_txk_iqk_8822e(struct dm_struct *dm, u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean kfail = false;
	u32 reg_tmp = 0;
	u32 i = 0, j = 0;
	u32 page = 0, addr = 0;
	RF_DBG(dm, DBG_RF_IQK, "[IQK]===>%s\n", __func__);	
	odm_set_rf_reg(dm, path, 0xdf, 0x00010, 0x0);
	odm_set_rf_reg(dm, path, 0xde, 0x10000, 0x1);
	odm_set_rf_reg(dm, path, 0x56, 0x00c00, 0x0);
	odm_set_rf_reg(dm, path, 0x56, 0x003e0, 0x07);
	odm_set_rf_reg(dm, path, 0x56, 0x0001f, 0x0c);
	odm_set_rf_reg(dm, path, 0x57, 0x08000, 0x1);
	odm_set_rf_reg(dm, path, 0x64, 0x07000, 0x0);
	odm_set_rf_reg(dm, path, 0xef, 0x00010, 0x1);
	odm_set_rf_reg(dm, path, 0x33, 0x0007f, 0x20);

	if (path == RF_PATH_A){
		reg_tmp = odm_get_mac_reg(dm, 0x1c, 0xc0000000);
		odm_set_mac_reg(dm, 0x1c, 0xc0000000, 0x0);
	} else {
		reg_tmp = odm_get_mac_reg(dm, 0xec, 0xc0000000);		
		odm_set_mac_reg(dm, 0xec, 0xc0000000, 0x0);
	}
	
	//LOK1		
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1b10, 0x000000ff, 0x00);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x00);
	odm_set_bb_reg(dm, 0x1bcc, 0x00000fc0, 0x09);
	odm_set_bb_reg(dm, 0x1b2c, 0x00000fff, 0x038);
	
	kfail = _iqk_one_shot_8822e(dm, path, LOK1);

	
	if(kfail) {
		iqk_info->fail_step |= BIT(0);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]5G LOK1 Fail \n");
	}
	//LOK2	
	odm_set_bb_reg(dm, 0x1b10, 0x000000ff, 0x00);
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x00);
	odm_set_bb_reg(dm, 0x1bcc, 0x00000fc0, 0x09);
	odm_set_bb_reg(dm, 0x1b2c, 0x00000fff, 0x038);

	kfail = _iqk_one_shot_8822e(dm, path, LOK2);


	if(kfail) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]5G LOK2 Fail \n");
	}
	if (path == RF_PATH_A)
		odm_set_mac_reg(dm, 0x1c, 0xc0000000, reg_tmp);
	else
		odm_set_mac_reg(dm, 0xec, 0xc0000000, reg_tmp);
		
	odm_set_rf_reg(dm, path, 0xef, 0x00010, 0x0);

	//LOK summary	
	iqk_info->lok_fail[path] = _lok1_check_8822e(dm, path);
	
	//TXK	
	odm_set_rf_reg(dm, path, 0x56, 0x0001f, 0x13);
	odm_set_rf_reg(dm, path, 0x64, 0x07000, 0x01);

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x00);
	odm_set_bb_reg(dm, 0x1bcc, 0x00000fc0, 0x12);
	RF_DBG(dm, DBG_RF_IQK, "[IQK](2) LOK S%x, 0x1bcc = 0x%x, txbb = 0x%x\n", path, odm_get_bb_reg(dm, 0x1bcc, 0x00000fc0), odm_get_rf_reg(dm, path, 0x56, 0x0001f));	
#if 0
	page = odm_get_bb_reg(dm, 0x1b00, 0x00000006);
	for (i = 0; i< 3; i++) {
		odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), i);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]TXK S%x,  ====  before TXK 0x1b00 = %x ====  \n", path, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));
		for (addr = 0x1b00; addr < 0x1c00; addr += 0x10) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK] S%x, 0x%x : 0x%08x  0x%08x  0x%08x  0x%08x\n", path, addr,
				odm_get_bb_reg(dm, addr, MASKDWORD),
				odm_get_bb_reg(dm, addr + 0x4, MASKDWORD),
				odm_get_bb_reg(dm, addr + 0x8, MASKDWORD),
				odm_get_bb_reg(dm, addr + 0xc, MASKDWORD));
		}
	}
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, page);
#endif
	if(iqk_info->is_nbiqk) {
		kfail = _iqk_one_shot_8822e(dm, path, NBTXK);
	} else {
		kfail = _iqk_one_shot_8822e(dm, path, TXIQK);		
	}
#if 0
	page = odm_get_bb_reg(dm, 0x1b00, 0x00000006);
	for (i = 0; i< 3; i++) {
		odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), i);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]TXK S%x, ====  after TXK 0x1b00 = %x ====  \n", path, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));	
		for (addr = 0x1b00; addr < 0x1c00; addr += 0x10) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK] S%x, 0x%x : 0x%08x  0x%08x  0x%08x  0x%08x\n", path, addr,
				odm_get_bb_reg(dm, addr, MASKDWORD),
				odm_get_bb_reg(dm, addr + 0x4, MASKDWORD),
				odm_get_bb_reg(dm, addr + 0x8, MASKDWORD),
				odm_get_bb_reg(dm, addr + 0xc, MASKDWORD));
		}
	}
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, page);
#endif
	//TXK summary
	RF_DBG(dm, DBG_RF_IQK, "[IQK]TXK S%x, 0x1b38 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b38, MASKDWORD));	
	iqk_info->iqk_fail_report[0][path][0] = kfail;

	if(kfail) {
		_iqk_txxym_dump_8822e(dm, path);
		iqk_info->fail_step |= BIT(1);
		odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, 0x1b38, MASKDWORD, 0x40000000);
		odm_set_bb_reg(dm, 0x1b70, BIT(8), 0x0);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]5G TXK Fail \n");
		RF_DBG(dm, DBG_RF_IQK, "[IQK](1)TXK S%x, 0x1b00 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));	
		RF_DBG(dm, DBG_RF_IQK, "[IQK](1)TXK S%x, 0x1b38 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b38, MASKDWORD));	
		RF_DBG(dm, DBG_RF_IQK, "[IQK](1)TXK S%x, 0x1b70 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b70, MASKDWORD));	

	}
	return kfail;
}

__odm_func_aon__
boolean _iqk_2g_txk_iqk_8822e(struct dm_struct *dm, u8 path)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean kfail = false;
	u32 reg_tmp = 0;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]===>%s\n", __func__);
	odm_set_rf_reg(dm, path, 0xdf, 0x00010, 0x0);
	odm_set_rf_reg(dm, path, 0xde, 0x10000, 0x1);
	odm_set_rf_reg(dm, path, 0x56, 0x00c00, 0x0);
	odm_set_rf_reg(dm, path, 0x56, 0x003e0, 0x0f);
	odm_set_rf_reg(dm, path, 0x56, 0x0001f, 0x05);
	odm_set_rf_reg(dm, path, 0x57, 0x08000, 0x1);
	odm_set_rf_reg(dm, path, 0x53, 0x000e0, 0x0);
	odm_set_rf_reg(dm, path, 0xef, 0x00010, 0x1);
	odm_set_rf_reg(dm, path, 0x33, 0x0007f, 0x00);
	if (path == RF_PATH_A){
		reg_tmp = odm_get_mac_reg(dm, 0x1c, 0xc0000000);
		odm_set_mac_reg(dm, 0x1c, 0xc0000000, 0x0);
	} else {
		reg_tmp = odm_get_mac_reg(dm, 0xec, 0xc0000000);		
		odm_set_mac_reg(dm, 0xec, 0xc0000000, 0x0);
	}
	//LOK1		
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);	
	odm_set_bb_reg(dm, 0x1b10, 0x000000ff, 0x00);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x00);
	odm_set_bb_reg(dm, 0x1bcc, 0x00000fc0, 0x09);
	odm_set_bb_reg(dm, 0x1b2c, 0x00000fff, 0x038);
	kfail = _iqk_one_shot_8822e(dm, path, LOK1);
	if(kfail) {
		iqk_info->fail_step |= BIT(0);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]2G LOK1 Fail \n");
	}
	
	//LOK2
	odm_set_bb_reg(dm, 0x1b10, 0x000000ff, 0x00);
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x00);
	odm_set_bb_reg(dm, 0x1bcc, 0x00000fc0, 0x09);
	odm_set_bb_reg(dm, 0x1b2c, 0x00000fff, 0x038);
	kfail = _iqk_one_shot_8822e(dm, path, LOK2);

	if(kfail) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]2G LOK2 Fail \n");
	}
	if (path == RF_PATH_A)
		odm_set_mac_reg(dm, 0x1c, 0xc0000000, reg_tmp);
	else
		odm_set_mac_reg(dm, 0xec, 0xc0000000, reg_tmp);
		
	odm_set_rf_reg(dm, path, 0xef, 0x00010, 0x0);

	//LOK summary
	iqk_info->lok_fail[path] = _lok1_check_8822e(dm, path);
	RF_DBG(dm, DBG_RF_IQK, "[IQK] RF 0x8 : 0x%x\n", odm_get_rf_reg(dm, path, 0x08, 0xfffff));
	RF_DBG(dm, DBG_RF_IQK, "[IQK] RF 0x9 : 0x%x\n", odm_get_rf_reg(dm, path, 0x09, 0xfffff));
	RF_DBG(dm, DBG_RF_IQK, "[IQK] RF 0xa : 0x%x\n", odm_get_rf_reg(dm, path, 0x0a, 0xfffff));
		
	//TXK	
	odm_set_rf_reg(dm, path, 0x56, 0x003e0, 0x07);
	odm_set_rf_reg(dm, path, 0x56, 0x0001f, 0x0c);
	odm_set_rf_reg(dm, path, 0x53, 0x000e0, 0x01);

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x00);
	odm_set_bb_reg(dm, 0x1bcc, 0x00000fc0, 0x12);
	if(iqk_info->is_nbiqk) {
		kfail = _iqk_one_shot_8822e(dm, path, NBTXK);		
	} else {
		kfail = _iqk_one_shot_8822e(dm, path, TXIQK);		
	}
	
	//TXK summary
	RF_DBG(dm, DBG_RF_IQK, "[IQK]TXK S%x, 0x1b38 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b38, MASKDWORD));
	iqk_info->iqk_fail_report[0][path][0] = kfail;

	if(kfail) {
		odm_set_bb_reg(dm, 0x1b38, MASKDWORD, 0x40000000);
		_iqk_txxym_dump_8822e(dm, path);		
		iqk_info->fail_step |= BIT(1);		
		odm_set_bb_reg(dm, 0x1b70, BIT(8), 0x0);
		RF_DBG(dm, DBG_RF_IQK, "[IQK] 2G TXK Fail \n");
	}
	return kfail;
}

__odm_func_aon__
void _iqk_sram_dump_8822e(struct dm_struct *dm, u8 path)
{
	u32 i;
	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]============ START S%d SRAM DUMP============\n", path);
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x12);
	odm_set_bb_reg(dm, 0x1b2c, 0x0fff0000, 0x018);
	odm_set_bb_reg(dm, 0x1be8, 0x00000080, 0x0);
	odm_set_bb_reg(dm, 0x1b34, 0x00000001, 0x1);
	odm_set_bb_reg(dm, 0x1b34, 0x00000001, 0x0);
	ODM_delay_us(500);
	odm_set_bb_reg(dm, 0x1bd4, 0x00200000, 0x1);
	odm_set_bb_reg(dm, 0x1bd4, 0x001f0000, 0x01);
	odm_set_bb_reg(dm, 0x1be8, 0x00000080, 0x1);
	odm_set_bb_reg(dm, 0x1bd8, 0x00000001, 0x1);
	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]===== I Q path data ======\n");
	for(i = 0; i< 0x80;i++) {
		odm_set_bb_reg(dm, 0x1be8, 0x0000007f, i);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, i = 0x%x, get 0x1bfc = %d\n", path, i, odm_get_bb_reg(dm, 0x1bfc, 0x00fff000));
	}
	
	for(i = 0; i< 0x80;i++) {
		odm_set_bb_reg(dm, 0x1be8, 0x0000007f, i);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, i = 0x%x, get 0x1bfc = %d\n", path, i, odm_get_bb_reg(dm, 0x1bfc, 0x00000fff));
	}
	odm_set_bb_reg(dm, R_0x1bd8, 0x00000001, 0x0);
	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]============ END S%d SRAM DUMP============\n", path);
}

//tune lna
__odm_func_aon__
boolean _iqk_5g_rx_gain_search1_8822e(struct dm_struct *dm, u8 path, boolean force)
{
	boolean fail = true, isbnd = true;
	u32 IQK_CMD = 0x0, rf_reg0 = 0x0;
	u32 lna = 0x0, rxbb = 0x0;
	u8 kcount = 0x0;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]============ S%d RXAGC============\n", path);
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1b18, 0x00000002, 0x1);
	odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x70408); //LNA=1	
	//odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x70008); //LNA=0
	//odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x70808); //LNA=2
	//odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x71008); //LNA=4
	//odm_set_bb_reg(dm, 0x1b18, 0x00000002, 0x1);

	if (force == true)
		return false;
	
	while(isbnd) {
		kcount++;
		odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x1b);
		odm_set_bb_reg(dm, 0x1b2c, 0x0fff0000, 0x018);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, set RXK2 0x1b24 = 0x%x, lna =%x, rxbb =%x\n", 
			path, odm_get_bb_reg(dm, 0x1b24, MASKDWORD), 
			odm_get_bb_reg(dm, 0x1b24, 0x00001c00),
			odm_get_bb_reg(dm, 0x1b24, 0x000003e0));
		IQK_CMD = (0xf << 8) | (1 << (path + 4)) | 0x8;

#if 0		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump kip reg (before)!!!\n");
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump page 0x0!!!\n");
		odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x0);
		for(i =0; i< 0x100/4;i++) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, rxk1 0x%x = 0x%x,\n", path, 0x1b00+ i*4, odm_get_bb_reg(dm, 0x1b00+ i*4, MASKDWORD));
		}	
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump page 0x1!!!\n");
		odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x1);
		for(i =0; i< 0x100/4;i++) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x,  rxk1 0x%x = 0x%x,\n", 
				path, 0x1b00+ i*4, odm_get_bb_reg(dm, 0x1b00+ i*4, MASKDWORD));
		}
#endif
		odm_set_bb_reg(dm, 0x1b00, MASKDWORD, IQK_CMD);
		odm_set_bb_reg(dm, 0x1b00, MASKDWORD, IQK_CMD + 1);
		ODM_delay_us(20);
		
		fail = _iqk_check_cal_8822e(dm, path, 0x1);

		odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
		rf_reg0 = odm_get_rf_reg(dm, path, 0x0, MASK20BITS);		
		lna  = (rf_reg0 & 0x01c00) >> 10; //rf0[12:10]
		rxbb = (rf_reg0 & 0x003e0) >> 5;  // rf0[9:5]
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, get lna = 0x%x, rxbb = 0x%x\n", path, lna, rxbb);
	
		if (rxbb > 0x9) {
			lna++;
			isbnd = true;
		} else if (rxbb > 0x1) {
			isbnd = false;
		} else {
			lna--;
			isbnd = true;
		}
#if 0
		if(!((lna < 8) && (lna > 1)))
			lna = 1;
#else
		if(lna < 1)
			lna = 0;
		if(lna >= 7)
			lna = 7;
#endif
	
		
		if(isbnd){
			ODM_delay_us(10);
			odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
			odm_set_bb_reg(dm, 0x1b24, 0x00001c00, lna);
			odm_set_bb_reg(dm, 0x1b24, 0x000003e0, rxbb);
			//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, new 0x1b00 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));
			//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, new 0x1b24 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b24, MASKDWORD));
		} else {	
			odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
			//odm_set_bb_reg(dm, 0x1b18, 0x00000002, 0x0);
			odm_set_bb_reg(dm, 0x1b24, 0x00001c00, lna);
			odm_set_bb_reg(dm, 0x1b24, 0x000003e0, rxbb);			
			//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, fix 0x1b00 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));
			//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, fix 0x1b24 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b24, MASKDWORD));
		}

		if (kcount >= 4) {
			fail = true;
			isbnd = false;
			RF_DBG(dm, DBG_RF_IQK, "[IQK]RXK1 timeout!!!\n");
		}
	}
#if 0	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump kip reg (after)!!!\n");
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump page 0x0!!!\n");
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x0);
	for(i =0; i< 0x100/4;i++) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, rxk1 0x%x = 0x%x,\n", 
			path, 0x1b00+ i*4, odm_get_bb_reg(dm, 0x1b00+ i*4, MASKDWORD));
	}	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump page 0x1!!!\n");
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x1);
	for(i =0; i< 0x100/4;i++) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x,  rxk1 0x%x = 0x%x,\n", 
			path, 0x1b00+ i*4, odm_get_bb_reg(dm, 0x1b00+ i*4, MASKDWORD));
	}
#endif
	return fail;
}

__odm_func_aon__
boolean _iqk_2g_rx_gain_search1_8822e(struct dm_struct *dm, u8 path, boolean force)
{
	boolean fail = true, isbnd = true;
	u32 IQK_CMD = 0x0, rf_reg0 = 0x0;
	u32 lna = 0x0, rxbb = 0x0;
	u8 kcount = 0x0;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]============ S%d RXAGC============\n", path);
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
	odm_set_bb_reg(dm, 0x1b18, 0x00000002, 0x1);
	odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x70408); //LNA=1	
	//odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x70008); //LNA=0
	//odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x70808); //LNA=2
	//odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x71008); //LNA=4
	//odm_set_bb_reg(dm, 0x1b18, 0x00000002, 0x1);

	if (force == true)
		return false;
	
	while(isbnd) {
		kcount++;
		odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x12);
		odm_set_bb_reg(dm, 0x1b2c, 0x0fff0000, 0x018);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, set RXK2 0x1b24 = 0x%x, lna =%x, rxbb =%x\n", 
			path, odm_get_bb_reg(dm, 0x1b24, MASKDWORD), 
			odm_get_bb_reg(dm, 0x1b24, 0x00001c00),
			odm_get_bb_reg(dm, 0x1b24, 0x000003e0));
		IQK_CMD = (0xf << 8) | (1 << (path + 4)) | 0x8;

#if 0		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump kip reg (before)!!!\n");
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump page 0x0!!!\n");
		odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x0);
		for(i =0; i< 0x100/4;i++) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, rxk1 0x%x = 0x%x,\n", path, 0x1b00+ i*4, odm_get_bb_reg(dm, 0x1b00+ i*4, MASKDWORD));
		}	
		RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump page 0x1!!!\n");
		odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x1);
		for(i =0; i< 0x100/4;i++) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x,  rxk1 0x%x = 0x%x,\n", 
				path, 0x1b00+ i*4, odm_get_bb_reg(dm, 0x1b00+ i*4, MASKDWORD));
		}
#endif
		odm_set_bb_reg(dm, 0x1b00, MASKDWORD, IQK_CMD);
		odm_set_bb_reg(dm, 0x1b00, MASKDWORD, IQK_CMD + 1);
		ODM_delay_us(20);
		
		fail = _iqk_check_cal_8822e(dm, path, 0x1);

		odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
		rf_reg0 = odm_get_rf_reg(dm, path, 0x0, MASK20BITS);		
		lna  = (rf_reg0 & 0x01c00) >> 10; //rf0[12:10]
		rxbb = (rf_reg0 & 0x003e0) >> 5;  // rf0[9:5]
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, get lna = 0x%x, rxbb = 0x%x\n", path, lna, rxbb);
	
		if (rxbb > 0x9) {
			lna++;
			isbnd = true;
		} else if (rxbb > 0x1) {
			isbnd = false;
		} else {
			lna--;
			isbnd = true;
		}
#if 0
		if(!((lna < 8) && (lna > 1)))
			lna = 1;
#else
		if(lna < 1)
			lna = 0;
		if(lna >= 7)
			lna = 7;
#endif
	
		
		if(isbnd){
			ODM_delay_us(10);
			odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
			odm_set_bb_reg(dm, 0x1b24, 0x00001c00, lna);
			odm_set_bb_reg(dm, 0x1b24, 0x000003e0, rxbb);
			//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, new 0x1b00 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));
			//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, new 0x1b24 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b24, MASKDWORD));
		} else {	
			odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
			//odm_set_bb_reg(dm, 0x1b18, 0x00000002, 0x0);
			odm_set_bb_reg(dm, 0x1b24, 0x00001c00, lna);
			odm_set_bb_reg(dm, 0x1b24, 0x000003e0, rxbb);			
			//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, fix 0x1b00 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));
			//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, fix 0x1b24 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b24, MASKDWORD));
		}

		if (kcount >= 5) {
			fail = true;
			isbnd = false;
			RF_DBG(dm, DBG_RF_IQK, "[IQK]RXK1 timeout!!!\n");
		}
	}
#if 0	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump kip reg (after)!!!\n");
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump page 0x0!!!\n");
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x0);
	for(i =0; i< 0x100/4;i++) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, rxk1 0x%x = 0x%x,\n", 
			path, 0x1b00+ i*4, odm_get_bb_reg(dm, 0x1b00+ i*4, MASKDWORD));
	}	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Dump page 0x1!!!\n");
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x1);
	for(i =0; i< 0x100/4;i++) {
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x,  rxk1 0x%x = 0x%x,\n", 
			path, 0x1b00+ i*4, odm_get_bb_reg(dm, 0x1b00+ i*4, MASKDWORD));
	}
#endif
	return fail;
}

__odm_func_aon__
void _iqk_5g_rxk_iqk_8822e(struct dm_struct *dm, u8 path)
{
#ifdef HALRF_DZ_LOG	
		struct _hal_rf_ *rf = &(dm->rf_table);	
		struct halrf_rfk_dz_rpt *rfk_dz = &(rf->rfk_dz_rpt);
#endif

	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean kfail = false;
	
	odm_set_rf_reg(dm, path, 0x9e, 0x00020, 0x0);
	odm_set_rf_reg(dm, path, 0x9e, 0x00400, 0x0);
	odm_set_rf_reg(dm, path, 0x56, 0x003e0, 0x00);
	kfail = _iqk_5g_rx_gain_search1_8822e(dm, path, false);

	if(kfail) {
		_iqk_sram_dump_8822e(dm, path);
		iqk_info->fail_step |= BIT(2);		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, 5G RXK1 Fail \n", path);
	} else {
		odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
		//odm_set_bb_reg(dm, 0x1b18, 0x00000002, 0x0);
		odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x1b);

		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, RXK2 0x1b00 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, RXK2 0x1b24 = 0x%x, lna =%x, rxbb =%x\n", 
			path, odm_get_bb_reg(dm, 0x1b24, MASKDWORD), 
			odm_get_bb_reg(dm, 0x1b24, 0x00001c00),
			odm_get_bb_reg(dm, 0x1b24, 0x000003e0));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, RXK2 0x1bcc = 0x%x\n", path, odm_get_bb_reg(dm, 0x1bcc, MASKDWORD));
	
		if(iqk_info->is_nbiqk) {
			kfail = _iqk_one_shot_8822e(dm, path, NBRXK);	
		} else	{
			kfail = _iqk_one_shot_8822e(dm, path, RXIQK2);
		}
	}
	// RX Summary
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RXK S%x, 0x1b3c = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b3c, MASKDWORD));
	iqk_info->iqk_fail_report[0][path][1] = kfail;

	if(kfail) {		
		_iqk_txxym_dump_8822e(dm, path); //trx share for dbg
		odm_set_bb_reg(dm, 0x1b3c, MASKDWORD, 0x40000000);
		iqk_info->fail_step |= BIT(3);
		odm_set_bb_reg(dm, 0x1b70, BIT(0), 0x0);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, 5G RXK Fail \n", path);
#ifdef HALRF_DZ_LOG
		rfk_dz->iqk_dz_code |= BIT(1 * path);
#endif

	}

}

__odm_func_aon__
void _iqk_2g_rxk_iqk_8822e(struct dm_struct *dm, u8 path)
{
#ifdef  HALRF_DZ_LOG	
		struct _hal_rf_ *rf = &(dm->rf_table);	
		struct halrf_rfk_dz_rpt *rfk_dz = &(rf->rfk_dz_rpt);
#endif
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean kfail = false;
	
	odm_set_rf_reg(dm, path, 0x9e, 0x00020, 0x0);
	odm_set_rf_reg(dm, path, 0x9e, 0x00400, 0x0);
	odm_set_rf_reg(dm, path, 0x56, 0x003e0, 0x04);
	kfail = _iqk_2g_rx_gain_search1_8822e(dm, path, false);
	
	if(kfail) {	
		_iqk_sram_dump_8822e(dm, path);	
		odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x70108); //LNA=0, RXBB = 0x8
		iqk_info->fail_step |= BIT(2);		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x,2G RXK1 Fail \n", path);
	}
	//_iqk_sram_dump_8822e(dm, path);		

	 	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
		//odm_set_bb_reg(dm, 0x1b18, 0x00000002, 0x0);
		odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x12);
		
		//RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, RXK2 0x1b00 = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, RXK2 0x1b24 = 0x%x, lna =%x, rxbb =%x\n", 
			path, odm_get_bb_reg(dm, 0x1b24, MASKDWORD), 
			odm_get_bb_reg(dm, 0x1b24, 0x00001c00),
			odm_get_bb_reg(dm, 0x1b24, 0x000003e0));
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, RXK2 0x1bcc = 0x%x\n", path, odm_get_bb_reg(dm, 0x1bcc, MASKDWORD));

		if(iqk_info->is_nbiqk) {
			kfail = _iqk_one_shot_8822e(dm, path, NBRXK);	
		} else	{
			kfail = _iqk_one_shot_8822e(dm, path, RXIQK2);			
		}
	
	// RX Summary
	RF_DBG(dm, DBG_RF_IQK, "[IQK]RXK S%x, 0x1b3c = 0x%x\n", path, odm_get_bb_reg(dm, 0x1b3c, MASKDWORD));
	iqk_info->iqk_fail_report[0][path][1] = kfail;

	if(kfail) {		
		odm_set_bb_reg(dm, 0x1b3c, MASKDWORD, 0x40000000);
		iqk_info->fail_step |= BIT(3);
		odm_set_bb_reg(dm, 0x1b70, BIT(0), 0x0);
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, 2G RXK Fail \n", path);
#ifdef HALRF_DZ_LOG
		rfk_dz->iqk_dz_code |= BIT(1 * path);
#endif
	}

}

__odm_func_aon__
void _iqk_iqk_by_path_8822e(
	struct dm_struct *dm,
	boolean segment_iqk)
{
#ifdef  HALRF_DZ_LOG	
		struct _hal_rf_ *rf = &(dm->rf_table);	
		struct halrf_rfk_dz_rpt *rfk_dz = &(rf->rfk_dz_rpt);
#endif

	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 path = 0x0;	
	boolean txkfail = false;
//	u8 i, j, k, q;

	//RF_DBG(dm, DBG_RF_IQK, "[IQK]===>%s\n", __func__);
/*
#if 1
	if ((iqk_info->iqk_bw == CHANNEL_WIDTH_5) || (iqk_info->iqk_bw == CHANNEL_WIDTH_10))
		iqk_info->is_nbiqk = true;
	else
		iqk_info->is_nbiqk = false;
#else
	iqk_info->is_nbiqk = true;
#endif
*/
	for(path = 0x0; path < SS_8822E; path++) {
		if(path == RF_PATH_A){		
			odm_set_rf_reg(dm, path, RF_PATH_B, 0xf0000, 0x1);	
		} else {		
			odm_set_rf_reg(dm, path, RF_PATH_A, 0xf0000, 0x1);	
		}


		if (iqk_info->iqk_band == ODM_BAND_2_4G) {
			iqk_info->is_nbiqk = true;
			//halrf_dack_reset_8822e(dm);
			txkfail = _iqk_2g_txk_iqk_8822e(dm, path);
			if(!txkfail) {
				//halrf_dack_reset_8822e(dm);
			_iqk_2g_rxk_iqk_8822e(dm, path);
			}
		} else {		
			iqk_info->is_nbiqk = false;
			//halrf_dack_reset_8822e(dm);
			txkfail = _iqk_5g_txk_iqk_8822e(dm, path);
			if(!txkfail) {
				//halrf_dack_reset_8822e(dm);
			_iqk_5g_rxk_iqk_8822e(dm, path);
		}
	}
		if(txkfail) {
#ifdef  HALRF_DZ_LOG
			rfk_dz->iqk_dz_code |= BIT(0 * path);
#endif
		}
	}
#if 1
	for(path = 0x0; path < SS_8822E; path++) {
		
		RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, BK CFIR\n", path);
		if(!iqk_info->lok_fail[path])
			iqk_info->lok_idac[0][path] = odm_get_rf_reg(dm, path, 0x58, 0xfffff);
		else
			iqk_info->lok_idac[0][path] = 0x84220;

		if(! iqk_info->iqk_fail_report[0][path][0]) {
			_iqk_backup_cfir_8822e(dm, path, TXIQK);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, Enable WBTX IQK\n", path);
		} else {	
			odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
			odm_set_bb_reg(dm, 0x1b38, MASKDWORD, 0x40000000);
			odm_set_bb_reg(dm, 0x1b70, BIT(8), 0x0);
			odm_set_bb_reg(dm, 0x1b3c, MASKDWORD, 0x40000000);
			odm_set_bb_reg(dm, 0x1b70, BIT(0), 0x0);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, Disable TX/RX IQK\n", path);
		}
		
		if(! iqk_info->iqk_fail_report[0][path][1]) {
			_iqk_backup_cfir_8822e(dm, path, RXIQK2);
	    } else {
	    	odm_set_bb_reg(dm, 0x1b00, 0x00000006, path);
			odm_set_bb_reg(dm, 0x1b3c, MASKDWORD, 0x40000000);
			odm_set_bb_reg(dm, 0x1b70, BIT(0), 0x0);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]S%x, Disable RX IQK\n", path);
	    }
	}
#endif
#if 0
		RF_DBG(dm, DBG_RF_IQK, "[IQK](2)_iqk_iqk_by_path_8822e !!!\n");
		for(q = 0; q < 1; q++) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->iqk_tab[%x]						 = %2x \n", q, iqk_info->iqk_tab[q]);
		for(i = 0; i < 2; i++) {//path	
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->cfir_en[%x][%x]				 = %2x \n", q, i, iqk_info->cfir_en[q][i]);		
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->txxy[%x][%x]					 = %2x \n", q, i, iqk_info->txxy[q][i]);		
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->rxxy[%x][%x]					 = %2x \n", q, i, iqk_info->rxxy[q][i]);
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->lok_idac[%x][%x]				 = %2x \n", q, i, iqk_info->lok_idac[q][i]);
			for(j = 0; j < 2; j++) { //trx
							
				for (k=0 ; k < 17; k++){ //real
			//RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->iqk_cfir_real[%x][%x][%x][%x] = %2x \n", q, i, j, k, iqk_info->iqk_cfir_real[q][i][j][k]);
				}
				RF_DBG(dm, DBG_RF_IQK, "[IQK]\n");	
				for (k = 0 ; k<17; k++){ //imag
			//RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->iqk_cfir_imag[%x][%x][%x][%x] = %2x \n", q, i, j, k, iqk_info->iqk_cfir_imag[q][i][j][k]);
				}
				RF_DBG(dm, DBG_RF_IQK, "[IQK]\n");	
			}
		}
		}
#endif
}

__odm_func_aon__
void _iqk_information_8822e(
	struct dm_struct *dm)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	
	iqk_info->rf_reg18 = odm_get_rf_reg(dm, RF_PATH_A, 0x18, RFREGOFFSETMASK);	
	iqk_info->iqk_band = (u8)((iqk_info->rf_reg18 & BIT(16)) >> 16); /*0/1:G/A*/
	iqk_info->iqk_ch = (u8)iqk_info->rf_reg18 & 0xff;
	//iqk_info->iqk_bw = (u8)((iqk_info->rf_reg18 & 0x3000) >> 12); /*3/2/1:20/40/80*/

    switch ((iqk_info->rf_reg18 & 0x3000) >> 12) {
    case 0x3: //20M
        iqk_info->iqk_bw = 0x0;		
		//RT_TRACE(DBGMSG_INIT, DBG_LOUD, ("2M \n"));
        break;
    case 0x2: //40M
        iqk_info->iqk_bw = 0x1;
		//RT_TRACE(DBGMSG_INIT, DBG_LOUD, ("4M \n"));
        break;
    case 0x1: //80M
        iqk_info->iqk_bw = 0x2;		
		//RT_TRACE(DBGMSG_INIT, DBG_LOUD, ("8M \n"));
        break;
    default:
        iqk_info->iqk_bw = 0x0;		
		iqk_info->is_nbiqk = true;
        break;
    }

	
	if ((*dm->band_width == CHANNEL_WIDTH_5) ||(*dm->band_width == CHANNEL_WIDTH_10))
		iqk_info->is_nbiqk = true;
	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]\n");	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]NBIQK/ Band/ CH/ BW = %x / %s / %d / %s\n",
	       iqk_info->is_nbiqk, iqk_info->iqk_band == 0 ? "2G" : "5G",
	       iqk_info->iqk_ch,
	       iqk_info->iqk_bw == 0 ? "20M" : (iqk_info->iqk_bw == 1 ? "40M" : "80M"));
}

__odm_func_aon__
void _iqk_start_iqk_8822e(
	struct dm_struct *dm,
	boolean segment_iqk)
{	
	_iqk_set_gnt_wl_gnt_bt_8822e(dm, true);
	_iqk_iqk_by_path_8822e(dm, segment_iqk);
	_iqk_set_gnt_wl_gnt_bt_8822e(dm, false);

	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0);
	RF_DBG(dm, DBG_RF_IQK, "[IQK](2)TXK S%x, 0x1b00 = 0x%x\n", 0x0, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));	
	RF_DBG(dm, DBG_RF_IQK, "[IQK](2)TXK S%x, 0x1b38 = 0x%x\n", 0x0, odm_get_bb_reg(dm, 0x1b38, MASKDWORD));	
	RF_DBG(dm, DBG_RF_IQK, "[IQK](2)TXK S%x, 0x1b70 = 0x%x\n", 0x0, odm_get_bb_reg(dm, 0x1b70, MASKDWORD));	
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 1);
	RF_DBG(dm, DBG_RF_IQK, "[IQK](2)TXK S%x, 0x1b00 = 0x%x\n", 0x1, odm_get_bb_reg(dm, 0x1b00, MASKDWORD));	
	RF_DBG(dm, DBG_RF_IQK, "[IQK](2)TXK S%x, 0x1b38 = 0x%x\n", 0x1, odm_get_bb_reg(dm, 0x1b38, MASKDWORD));	
	RF_DBG(dm, DBG_RF_IQK, "[IQK](2)TXK S%x, 0x1b70 = 0x%x\n", 0x1, odm_get_bb_reg(dm, 0x1b70, MASKDWORD));	

}

__odm_func_aon__
void _iq_calibrate_8822e_init(
	struct dm_struct *dm)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i, j, k, m;
	static boolean firstrun = true;

#if 0
	dm->n_iqk_fail_cnt = 0;	
	dm->n_iqk_cnt = 0;	
	dm->n_iqk_cnt++;
#endif
	iqk_info->is_nbiqk = false;
	iqk_info->fail_step = 0;
	iqk_info->iqk_times = 0;
	iqk_info->kcount = 0;
	iqk_info->fail_count = 0; 

	if (firstrun) {
		firstrun = false;
		RF_DBG(dm, DBG_RF_IQK, "[IQK]=====>PHY_IQCalibrate_8822e_Init\n");

		for (i = 0; i < SS_8822E; i++) {
			for (j = 0; j < 2; j++) {
				iqk_info->lok_fail[i] = true;
				iqk_info->iqk_fail[j][i] = true;
				iqk_info->iqc_matrix[j][i] = 0x20000000;
			}
		}

		for (i = 0; i < 2; i++) {
			iqk_info->iqk_tab[i] = 0x0;

			for (j = 0; j < SS_8822E; j++) {
				iqk_info->lok_idac[i][j] = 0x84220;

				for (k = 0; k < 2; k++) {
					iqk_info->iqk_fail_report[i][j][k] = true;
					for (m = 0; m < 17; m++) {
						iqk_info->iqk_cfir_real[i][j][k][m] = 0x0;
						iqk_info->iqk_cfir_imag[i][j][k][m] = 0x0;
					}
				}
			}
		}
	}

}

__odm_func_aon__
void _iqk_reload_iqk_setting_8822e(
	struct dm_struct *dm,
	u8 ch)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 path;

	for (path = 0; path < SS_8822E; path++) {
		//LOK
		odm_set_rf_reg(dm, path, 0xdf, BIT(4), 0x1);
		odm_set_rf_reg(dm, path, 0x58, RFREGOFFSETMASK, iqk_info->lok_idac[ch][path]);
		//TXK
		_iqk_reload_cfir_8822e(dm, ch, path, TX_IQK);		
		//RXK
		_iqk_reload_cfir_8822e(dm, ch, path, RX_IQK);				
	}
	
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x0);
}

__odm_func_aon__
boolean _iqk_reload_iqk_8822e(
	struct dm_struct *dm,
	boolean reset)
{
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 i = 0, ch = 0;	
//	u8 j, k, q;
	iqk_info->is_reload = false;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]==========IQK Reload!!!!!==========\n");

	if (reset) {
		for (i = 0; i < 2; i++)
			iqk_info->iqk_tab[i] = 0x0;
	} else {
		for (ch = 0; ch < 2; ch++) {			
			if (iqk_info->rf_reg18 == iqk_info->iqk_tab[ch]) {				
				RF_DBG(dm, DBG_RF_IQK, "[IQK]reload IQK result !!!!\n");
				_iqk_reload_iqk_setting_8822e(dm, ch);
				iqk_info->is_reload = true;
				break; //break for loop 
			}
		}
	}
	/*report*/
	odm_set_bb_reg(dm, 0x1bf0, BIT(16), (u8)iqk_info->is_reload);
	
	RF_DBG(dm, DBG_RF_IQK, "[IQK]==========End IQK Reload!!!!!==========\n");
#if 0	
	if(iqk_info->is_reload) {
		for(q = 1; q < 2; q++) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK](1)iqk_info->iqk_tab[%x]				 = %2x \n", q, iqk_info->iqk_tab[q]);
		for(i = 0; i < 2; i++) {//path	
			RF_DBG(dm, DBG_RF_IQK, "[IQK](1)iqk_info->cfir_en[%x][%x]				 = %2x \n", q, i, iqk_info->cfir_en[q][i]);		
			RF_DBG(dm, DBG_RF_IQK, "[IQK](1)iqk_info->txxy[%x][%x]					 = %2x \n", q, i, iqk_info->txxy[q][i]);		
			RF_DBG(dm, DBG_RF_IQK, "[IQK](1)iqk_info->rxxy[%x][%x]					 = %2x \n", q, i, iqk_info->rxxy[q][i]);
			RF_DBG(dm, DBG_RF_IQK, "[IQK](1)iqk_info->lok_idac[%x][%x]				 = %2x \n", q, i, iqk_info->lok_idac[q][i]);
			for(j = 0; j < 2; j++) { //trx
				for (k=0 ; k < 17; k++){ //real
			//RF_DBG(dm, DBG_RF_IQK, "[IQK](1)iqk_info->iqk_cfir_real[%x][%x][%x][%x] = %2x \n", q, i, j, k, iqk_info->iqk_cfir_real[q][i][j][k]);
				}
				RF_DBG(dm, DBG_RF_IQK, "[IQK]\n");				
				for (k = 0 ; k<17; k++){ //imag
			//RF_DBG(dm, DBG_RF_IQK, "[IQK](1)iqk_info->iqk_cfir_imag[%x][%x][%x][%x] = %2x \n", q, i, j, k, iqk_info->iqk_cfir_imag[q][i][j][k]);
				}				
				RF_DBG(dm, DBG_RF_IQK, "[IQK]\n");				
			}
		}
		}
	}
#endif
	return iqk_info->is_reload;
}

__odm_func_aon__
void _phy_iq_calibrate_8822e(
	struct dm_struct *dm,
	boolean reset,
	boolean segment_iqk)
{
	u32 MAC_backup[MAC_REG_NUM_8822E] = {0};
	u32 BB_backup[BB_REG_NUM_8822E] = {0};
	u32 RF_backup[RF_REG_NUM_8822E][SS_8822E] = {{0}};
	u32 backup_mac_reg[MAC_REG_NUM_8822E] = {0x520, 0x1c, 0xec, 0x70};
	u32 backup_bb_reg[BB_REG_NUM_8822E] = {
		0x0820, 0x0824, 0x1c38, 0x1c68,
		0x1d60, 0x180c, 0x410c, 0x1c3c,
		0x1a14, 0x1d58, 0x1d70, 0x1864,
		R_0x4164, 0x186c, 0x416c, 0x1a14,
		R_0x1e70, 0x80c, 0x1e7c, 0x18a4, 
		R_0x41a4};
	u32 backup_rf_reg[RF_REG_NUM_8822E] = {0x19, 0x9e};
//	boolean is_mp = false;
	u8 i = 0;

	struct dm_iqk_info *iqk_info = &dm->IQK_info;

#if 0
	if (*dm->mp_mode)
		is_mp = true;
	else
		is_mp = false;

	_iqk_information_8822e(dm);
	if (!is_mp) {
		if (_iqk_reload_iqk_8822e(dm, reset))
			return;
	}
#else
	_iqk_information_8822e(dm);
	//_iqk_reload_iqk_8822e(dm, reset);
#endif

	RF_DBG(dm, DBG_RF_IQK, "[IQK]==========IQK strat!!!!!==========\n");
	RF_DBG(dm, DBG_RF_IQK, "[IQK]band_type = %s, band_width = %d, ExtPA2G = %d, ext_pa_5g = %d\n", (*dm->band_type == ODM_BAND_5G) ? "5G" : "2G", *dm->band_width, dm->ext_pa, dm->ext_pa_5g);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Interface = %d, Cv = %x\n", dm->support_interface, dm->cut_version);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]Test V35 \n");
	iqk_info->iqk_step = 0;
	iqk_info->fail_step = 0;
	iqk_info->iqk_times++;	
	iqk_info->kcount++;
	iqk_info->tmp_gntwl = _iqk_btc_read_indirect_reg_8822e(dm, 0x38);

	_iqk_backup_mac_bb_8822e(dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg);
	_iqk_backup_rf_8822e(dm, RF_backup, backup_rf_reg);

	_iqk_switch_table_8822e(dm);

	for(i =0; i < 2; i++){
	_iqk_preset_8822e(dm);
	_iqk_macbb_setting_8822e(dm);
	_iqk_afe_setting_8822e(dm);
	_iqk_start_iqk_8822e(dm, segment_iqk);
	_iqk_afe_restore_8822e(dm);	
	_iqk_macbb_restore_8822e(dm);
		if((iqk_info->iqk_fail_report[0][0][0] == false) && (iqk_info->iqk_fail_report[0][1][0] == false))
			break;
	}
	_iqk_restore_rf_8822e(dm, backup_rf_reg, RF_backup);
	_iqk_restore_mac_bb_8822e(dm, MAC_backup, BB_backup, backup_mac_reg, backup_bb_reg);
	//halrf_dack_reset_8822e(dm);
	//halrf_delay_10us(1);
#if 0
	for(q = 0; q < 2; q++) {
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->iqk_tab[%x]			  	     = %2x \n", q, iqk_info->iqk_tab[q]);
		for(i = 0; i < 2; i++) {//path	
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->cfir_en[%x][%x]				 = %2x \n", q, i, iqk_info->cfir_en[q][i]); 	
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->txxy[%x][%x]					 = %2x \n", q, i, iqk_info->txxy[q][i]);		
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->rxxy[%x][%x]					 = %2x \n", q, i, iqk_info->rxxy[q][i]);
			RF_DBG(dm, DBG_RF_IQK, "[IQK](2)iqk_info->lok_idac[%x][%x]				 = %2x \n", q, i, iqk_info->lok_idac[q][i]);
			RF_DBG(dm, DBG_RF_IQK, "[IQK]\n");				
		}
	}
#endif
	if(iqk_info->fail_step != 0x0)
		iqk_info->fail_count++;
	RF_DBG(dm, DBG_RF_IQK, "[IQK]i = %d, iqk_times/pass/fail_id = %d/ %d /%d\n", i, iqk_info->iqk_times, iqk_info->kcount, iqk_info->fail_step);
	RF_DBG(dm, DBG_RF_IQK, "[IQK]==========IQK end!!!!!==========\n");
}

__odm_func_aon__
void iqk_reload_iqk_8822e(void *dm_void, boolean reset)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	_iqk_reload_iqk_8822e(dm, reset);

}
__odm_func_aon__
void _check_fwiqk_done_8822e(struct dm_struct *dm)
{
	u32 counter = 0x0;
#if 1
	while (1) {
		if (odm_read_1byte(dm, 0x2d9c) == 0xaa  || counter > 300)
			break;
		counter++;
		halrf_delay_10us(100);
	};
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x1);
	odm_write_1byte(dm, 0x1b10, 0x0);
	odm_set_bb_reg(dm, 0x1b00, 0x00000006, 0x0);
	odm_write_1byte(dm, 0x1b10, 0x0);	

	RF_DBG(dm, DBG_RF_IQK, "[IQK]counter = %d\n", counter);
#else
	ODM_delay_ms(50);
	RF_DBG(dm, DBG_RF_IQK, "[IQK] delay 50ms\n");

#endif
}

__odm_func_aon__
void _phy_iq_calibrate_by_fw_8822e(
	void *dm_void,
	u8 clear,
	u8 segment_iqk)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	enum hal_status status = HAL_STATUS_FAILURE;

	if (*dm->mp_mode)
		clear = 0x1;
	//	else if (dm->is_linked)
	//		segment_iqk = 0x1;

	iqk_info->iqk_times++;
	status = odm_iq_calibrate_by_fw(dm, clear, segment_iqk);

	if (status == HAL_STATUS_SUCCESS)
		RF_DBG(dm, DBG_RF_IQK, "[IQK]FWIQK OK!!!\n");
	else
		RF_DBG(dm, DBG_RF_IQK, "[IQK]FWIQK fail!!!\n");
}

/*IQK_version:0x1, NCTL:0x0*/
/*1.max tx pause while IQK*/
/*2.CCK off while IQK*/
__odm_func_aon__
void phy_iq_calibrate_8822e(
	void *dm_void,
	boolean clear,
	boolean segment_iqk)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;

	if (!(rf->rf_supportability & HAL_RF_IQK))
		return;

	//if (!(*dm->mp_mode))
	//	_iqk_check_coex_status(dm, true);

	dm->rf_calibrate_info.is_iqk_in_progress = true;
	/*FW IQK*/
	if (dm->fw_offload_ability & PHYDM_RF_IQK_OFFLOAD) {
		_phy_iq_calibrate_by_fw_8822e(dm, clear, (u8)(segment_iqk));
		_check_fwiqk_done_8822e(dm);
		_iqk_check_if_reload(dm);
		RF_DBG(dm, DBG_RF_IQK, "!!!!!  FW IQK   !!!!!\n");
	} else {
		_iq_calibrate_8822e_init(dm);
		_phy_iq_calibrate_8822e(dm, clear, segment_iqk);
	}
#if (DM_ODM_SUPPORT_TYPE & ODM_AP)
	_iqk_iqk_fail_report_8822e(dm);
#endif

	dm->rf_calibrate_info.is_iqk_in_progress = false;

}




__odm_func_aon__
void phy_iqk_dbg_cfir_backup_8822e(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 path, idx, i;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]%-20s\n", "backup TX/RX CFIR");

	for (path = 0; path < SS_8822E; path++)
		for (idx = 0; idx < 2; idx++)
			phydm_get_iqk_cfir(dm, idx, path, true);

	for (path = 0; path < SS_8822E; path++) {
		for (idx = 0; idx < 2; idx++) {
			for (i = 0; i <= 16; i++) {
				RF_DBG(dm, DBG_RF_IQK,
				       "[IQK]%-7s %-3s CFIR_real: %-2d: 0x%x\n",
				       (path == 0) ? "PATH A" : "PATH B",
				       (idx == 0) ? "TX" : "RX", i,
				       iqk_info->iqk_cfir_real[2][path][idx][i])
				       ;
			}
			for (i = 0; i <= 16; i++) {
				RF_DBG(dm, DBG_RF_IQK,
				       "[IQK]%-7s %-3s CFIR_img:%-2d: 0x%x\n",
				       (path == 0) ? "PATH A" : "PATH B",
				       (idx == 0) ? "TX" : "RX", i,
				       iqk_info->iqk_cfir_imag[2][path][idx][i])
				       ;
			}
		}
	}
}

__odm_func_aon__
void phy_iqk_dbg_cfir_write_8822e(void *dm_void, u8 type, u32 path, u32 idx,
			      u32 i, u32 data)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	if (type == 0)
		iqk_info->iqk_cfir_real[2][path][idx][i] = (u16)data;
	else
		iqk_info->iqk_cfir_imag[2][path][idx][i] = (u16)data;
}

__odm_func_aon__
void phy_iqk_dbg_cfir_backup_show_8822e(void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	u8 path, idx, i;

	RF_DBG(dm, DBG_RF_IQK, "[IQK]%-20s\n", "backup TX/RX CFIR");

	for (path = 0; path < SS_8822E; path++) {
		for (idx = 0; idx < 2; idx++) {
			for (i = 0; i <= 16; i++) {
				RF_DBG(dm, DBG_RF_IQK,
				       "[IQK]%-10s %-3s CFIR_real:%-2d: 0x%x\n",
				       (path == 0) ? "PATH A" : "PATH B",
				       (idx == 0) ? "TX" : "RX", i,
				       iqk_info->iqk_cfir_real[2][path][idx][i])
				       ;
			}
			for (i = 0; i <= 16; i++) {
				RF_DBG(dm, DBG_RF_IQK,
				       "[IQK]%-10s %-3s CFIR_img:%-2d: 0x%x\n",
				       (path == 0) ? "PATH A" : "PATH B",
				       (idx == 0) ? "TX" : "RX", i,
				       iqk_info->iqk_cfir_imag[2][path][idx][i])
				       ;
			}
		}
	}
}

__odm_func_aon__
void iqk_power_save_8822e(
	struct dm_struct *dm,
	boolean is_power_save)
{
	u8 path  = 0;

	for(path = 0; path < SS_8822E; path++) {
		odm_set_bb_reg(dm, 0x1b00, BIT(2)| BIT(1), path);
		if (is_power_save)
			odm_set_bb_reg(dm, 0x1b08, BIT(7), 0x0);
		else
			odm_set_bb_reg(dm, 0x1b08, BIT(7), 0x1);
		}
}

void iqk_info_rsvd_page_8822e(
	void *dm_void,
	u8 *buf,
	u32 *buf_size)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk = &dm->IQK_info;
	u32 i = 0;
	
	if (buf) {
		odm_move_memory(dm, buf, iqk->iqk_tab,
				sizeof(iqk->iqk_tab));
		i += sizeof(iqk->iqk_tab);
		odm_move_memory(dm, buf + i, &iqk->iqk_cfir_real[0][0],
				sizeof(iqk->iqk_cfir_real[0][0]));
		i += sizeof(iqk->iqk_cfir_real[0][0]);
		odm_move_memory(dm, buf + i, &iqk->iqk_cfir_real[0][1],
				sizeof(iqk->iqk_cfir_real[0][1]));
		i += sizeof(iqk->iqk_cfir_real[0][1]);
		odm_move_memory(dm, buf + i, &iqk->iqk_cfir_real[1][0],
				sizeof(iqk->iqk_cfir_real[1][0]));
		i += sizeof(iqk->iqk_cfir_real[1][0]);
		odm_move_memory(dm, buf + i, &iqk->iqk_cfir_real[1][1],
				sizeof(iqk->iqk_cfir_real[1][1]));
		i += sizeof(iqk->iqk_cfir_real[1][1]);
		odm_move_memory(dm, buf + i, &iqk->iqk_cfir_imag[0][0],
				sizeof(iqk->iqk_cfir_imag[0][0]));
		i += sizeof(iqk->iqk_cfir_imag[0][0]);
		odm_move_memory(dm, buf + i, &iqk->iqk_cfir_imag[0][1],
				sizeof(iqk->iqk_cfir_imag[0][1]));
		i += sizeof(iqk->iqk_cfir_imag[0][1]);
		odm_move_memory(dm, buf + i, &iqk->iqk_cfir_imag[1][0],
				sizeof(iqk->iqk_cfir_imag[1][0]));
		i += sizeof(iqk->iqk_cfir_imag[1][0]);
		odm_move_memory(dm, buf + i, &iqk->iqk_cfir_imag[1][1],
				sizeof(iqk->iqk_cfir_imag[1][1]));
		i += sizeof(iqk->iqk_cfir_imag[1][1]);
		odm_move_memory(dm, buf + i, &iqk->lok_idac[0][0],
				sizeof(iqk->lok_idac[0][0]));
		i += sizeof(iqk->lok_idac[0][0]);
		odm_move_memory(dm, buf + i, &iqk->lok_idac[0][1],
				sizeof(iqk->lok_idac[0][1]));
		i += sizeof(iqk->lok_idac[0][1]);
		odm_move_memory(dm, buf + i, &iqk->lok_idac[1][0],
				sizeof(iqk->lok_idac[1][0]));
		i += sizeof(iqk->lok_idac[1][0]);
		odm_move_memory(dm, buf + i, &iqk->lok_idac[1][1],
				sizeof(iqk->lok_idac[1][1]));
		i += sizeof(iqk->lok_idac[1][1]);
		odm_move_memory(dm, buf + i, &iqk->txxy[0][0],
				sizeof(iqk->txxy[0][0]));
		i += sizeof(iqk->txxy[0][0]);
		odm_move_memory(dm, buf + i, &iqk->txxy[0][1],
				sizeof(iqk->txxy[0][1]));
		i += sizeof(iqk->txxy[0][1]);
		odm_move_memory(dm, buf + i, &iqk->txxy[1][0],
				sizeof(iqk->txxy[1][0]));
		i += sizeof(iqk->txxy[1][0]);
		odm_move_memory(dm, buf + i, &iqk->txxy[1][1],
				sizeof(iqk->txxy[1][1]));
		i += sizeof(iqk->txxy[1][1]);
		odm_move_memory(dm, buf + i, &iqk->rxxy[0][0],
				sizeof(iqk->rxxy[0][0]));
		i += sizeof(iqk->rxxy[0][0]);
		odm_move_memory(dm, buf + i, &iqk->rxxy[0][1],
				sizeof(iqk->rxxy[0][1]));
		i += sizeof(iqk->rxxy[0][1]);
		odm_move_memory(dm, buf + i, &iqk->rxxy[1][0],
				sizeof(iqk->rxxy[1][0]));
		i += sizeof(iqk->rxxy[1][0]);
		odm_move_memory(dm, buf + i, &iqk->rxxy[1][1],
				sizeof(iqk->rxxy[1][1]));
		i += sizeof(iqk->rxxy[1][1]);
		odm_move_memory(dm, buf + i, &iqk->cfir_en[0][0],
				sizeof(iqk->cfir_en[0][0]));
		i += sizeof(iqk->cfir_en[0][0]);
		odm_move_memory(dm, buf + i, &iqk->cfir_en[0][1],
				sizeof(iqk->cfir_en[0][1]));
		i += sizeof(iqk->cfir_en[0][1]);
		odm_move_memory(dm, buf + i, &iqk->cfir_en[1][0],
				sizeof(iqk->cfir_en[1][0]));
		i += sizeof(iqk->cfir_en[1][0]);
		odm_move_memory(dm, buf + i, &iqk->cfir_en[1][1],
				sizeof(iqk->cfir_en[1][1]));
		i += sizeof(iqk->cfir_en[1][1]);
	}

	if (buf_size)
		*buf_size = IQK_INFO_RSVD_LEN_8822E;
}

#if !(DM_ODM_SUPPORT_TYPE & ODM_AP)
void do_iqk_8822e(
	void *dm_void,
	u8 delta_thermal_index,
	u8 thermal_value,
	u8 threshold)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;

	dm->rf_calibrate_info.thermal_value_iqk = thermal_value;
	halrf_segment_iqk_trigger(dm, true, false);
}
#else
/*Originally config->do_iqk is hooked phy_iq_calibrate_8822E, but do_iqk_8822E and phy_iq_calibrate_8822E have different arguments*/
void do_iqk_8822e(
	void *dm_void,
	u8 delta_thermal_index,
	u8 thermal_value,
	u8 threshold)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_iqk_info *iqk_info = &dm->IQK_info;
	boolean is_recovery = (boolean)delta_thermal_index;

	halrf_segment_iqk_trigger(dm, true, false);
}
#endif

#endif
