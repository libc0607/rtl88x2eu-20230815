/******************************************************************************
 *
 * Copyright(c) 2016 - 2019 Realtek Corporation. All rights reserved.
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

#ifndef _HALMAC_EFUSE_8822E_H_
#define _HALMAC_EFUSE_8822E_H_

#include "../../halmac_api.h"

#if HALMAC_8822E_SUPPORT

#define SEC_CTRL_EFUSE_SIZE_8822E 4
#define UNLOCK_CODE 0x69
#define BT_START_OFFSET 0x600
#define BT_1B_ENTRY_SIZE 0x80
#define PG_VER_LEN_8822E 7

#define EFUSE_PART_1_END 1024

#define LIM_LOG_SIZE_PCIE_8822E 416
#define LIM_LOG_SIZE_USB_8822E	416
#define LIM_LOG_SIZE_SDIO_8822E 416

enum efuse_c2h_part{
	EFUSE_C2H_PART_1,
	EFUSE_C2H_PART_2,
};

enum halmac_ret_status
dump_efuse_map_8822e(struct halmac_adapter *adapter,
		    enum halmac_efuse_read_cfg cfg);

enum halmac_ret_status
eeprom_parser_8822e(struct halmac_adapter *adapter, u8 *phy_map, u8 *log_map);

enum halmac_ret_status
eeprom_mask_parser_8822e(struct halmac_adapter *adapter, u8 *phy_map,
			u8 *log_mask);

enum halmac_ret_status
dump_efuse_map_bt_8822e(struct halmac_adapter *adapter,
		       enum halmac_efuse_bank bank, u32 size, u8 *map);

enum halmac_ret_status
write_efuse_bt_8822e(struct halmac_adapter *adapter, u32 offset, u8 value,
		    enum halmac_efuse_bank bank);

enum halmac_ret_status
read_efuse_bt_8822e(struct halmac_adapter *adapter, u32 offset, u8 *value,
		   enum halmac_efuse_bank bank);

enum halmac_ret_status
get_efuse_available_size_8822e(struct halmac_adapter *adapter, u32 *size);

enum halmac_ret_status
dump_log_efuse_map_8822e(struct halmac_adapter *adapter,
			enum halmac_efuse_read_cfg cfg);

enum halmac_ret_status
dump_log_efuse_map_bt_8822e(struct halmac_adapter *adapter,
			   enum halmac_efuse_read_cfg cfg);

enum halmac_ret_status
dump_log_efuse_mask_8822e(struct halmac_adapter *adapter,
			 enum halmac_efuse_read_cfg cfg);

enum halmac_ret_status
read_logical_efuse_8822e(struct halmac_adapter *adapter, u32 offset, u8 *value);

enum halmac_ret_status
read_logical_efuse_bt_8822e(struct halmac_adapter *adapter,
			   u32 offset, u8 *value);

enum halmac_ret_status
write_log_efuse_8822e(struct halmac_adapter *adapter, u32 offset, u8 value);

enum halmac_ret_status
write_log_efuse_word_8822e(struct halmac_adapter *adapter, u32 offset,
			  u16 value);

enum halmac_ret_status
write_log_efuse_bt_8822e(struct halmac_adapter *adapter, u32 offset, u8 value);

enum halmac_ret_status
write_log_efuse_word_bt_8822e(struct halmac_adapter *adapter,
			     u32 offset, u16 value);

enum halmac_ret_status
pg_efuse_by_map_8822e(struct halmac_adapter *adapter,
		     struct halmac_pg_efuse_info *info,
		     enum halmac_efuse_read_cfg cfg);

enum halmac_ret_status
mask_log_efuse_8822e(struct halmac_adapter *adapter,
		    struct halmac_pg_efuse_info *info);

enum halmac_ret_status
read_efuse_8822e(struct halmac_adapter *adapter, u32 offset, u32 size, u8 *map);

enum halmac_ret_status
write_hw_efuse_8822e(struct halmac_adapter *adapter, u32 offset, u8 value);

enum halmac_ret_status
switch_efuse_bank_8822e(struct halmac_adapter *adapter,
		       enum halmac_efuse_bank bank);

enum halmac_ret_status
get_efuse_data_8822e(struct halmac_adapter *adapter, u8 *buf, u32 size);

enum halmac_ret_status
cnv_efuse_state_8822e(struct halmac_adapter *adapter,
		     enum halmac_cmd_construct_state dest_state);

enum halmac_ret_status
get_dump_phy_efuse_status_8822e(struct halmac_adapter *adapter,
			       enum halmac_cmd_process_status *proc_status,
			       u8 *data, u32 *size);

enum halmac_ret_status
get_dump_log_efuse_status_8822e(struct halmac_adapter *adapter,
			       enum halmac_cmd_process_status *proc_status,
			       u8 *data, u32 *size);

enum halmac_ret_status
get_dump_log_efuse_mask_status_8822e(struct halmac_adapter *adapter,
				    enum halmac_cmd_process_status *proc_status,
				    u8 *data, u32 *size);

enum halmac_ret_status
get_h2c_ack_phy_efuse_8822e(struct halmac_adapter *adapter, u8 *buf, u32 size);

enum halmac_ret_status
write_wifi_phy_efuse_8822e(struct halmac_adapter *adapter, u32 offset, u8 value);

enum halmac_ret_status
read_wifi_phy_efuse_8822e(struct halmac_adapter *adapter, u32 offset, u32 size,
			 u8 *value);
enum halmac_ret_status
switch_ctrl_reg_8822e(struct halmac_adapter *adapter, u8 switch_reg);

#endif /* HALMAC_8822E_SUPPORT */

#endif/* _HALMAC_EFUSE_8822E_H_ */
