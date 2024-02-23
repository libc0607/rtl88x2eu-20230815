/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
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

#if 1

#endif

void odm_read_and_config_mp_8822e_cal_init(void *dm_void)
{
#if 1
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u32	i = 0;
	u32	array_len = sizeof(array_mp_8822e_cal_init)/sizeof(u32);
	u32	*array = array_mp_8822e_cal_init;
	u32	v1 = 0, v2 = 0;
//00_8822E_IQK_Phy_setting_20220221
	odm_set_bb_reg(dm, R_0x1cd0, 0xf0000000, 0x7);
	odm_set_bb_reg(dm, R_0x1e24, 0x00020000, 0x1);
	odm_set_bb_reg(dm, R_0x180c, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x410c, 0x80000000, 0x1);

	while ((i + 1) < array_len) {
		v1 = array[i];
		v2 = array[i + 1];
		odm_config_bb_phy_8822e(dm, v1, MASKDWORD, v2);		
		RF_DBG(dm, DBG_RF_IQK, "v1 = 0x%x, v2 = 0x%x \n",v1,v2);
		i = i + 2;
	}

	dpk_info->is_dpk_pwr_on = 1;
	dpk_info->is_dpk_enable = 1;
	dpk_info->is_dpk_by_channel= 1;
#endif
}

#endif
