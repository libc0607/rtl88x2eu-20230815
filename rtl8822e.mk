EXTRA_CFLAGS += -DCONFIG_RTL8822E

ifeq ($(CONFIG_MP_INCLUDED), y)
### 8822E Default Enable VHT MP HW TX MODE ###
#EXTRA_CFLAGS += -DCONFIG_MP_VHT_HW_TX_MODE
#CONFIG_MP_VHT_HW_TX_MODE = y
endif

_HAL_INTFS_FILES +=	hal/rtl8822e/rtl8822e_halinit.o \
			hal/rtl8822e/rtl8822e_mac.o \
			hal/rtl8822e/rtl8822e_cmd.o \
			hal/rtl8822e/rtl8822e_phy.o \
			hal/rtl8822e/rtl8822e_ops.o \
			hal/rtl8822e/hal8822e_fw.o

ifeq ($(CONFIG_USB_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8822e/$(HCI_NAME)/rtl8822eu_halinit.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822eu_halmac.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822eu_io.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822eu_xmit.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822eu_recv.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822eu_led.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822eu_ops.o

_HAL_INTFS_FILES +=hal/efuse/rtl8822e/HalEfuseMask8822E_USB.o
endif
ifeq ($(CONFIG_PCI_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8822e/$(HCI_NAME)/rtl8822ee_halinit.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822ee_halmac.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822ee_io.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822ee_xmit.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822ee_recv.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822ee_led.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822ee_ops.o

_HAL_INTFS_FILES +=hal/efuse/rtl8822e/HalEfuseMask8822E_PCIE.o
endif
ifeq ($(CONFIG_SDIO_HCI), y)
_HAL_INTFS_FILES +=	hal/rtl8822e/$(HCI_NAME)/rtl8822es_halinit.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822es_halmac.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822es_io.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822es_xmit.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822es_recv.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822es_led.o \
			hal/rtl8822e/$(HCI_NAME)/rtl8822es_ops.o

_HAL_INTFS_FILES +=hal/efuse/rtl8822e/HalEfuseMask8822E_SDIO.o

_HAL_INTFS_FILES +=hal/hal_hci/hal_sdio_coex.o
endif

include $(src)/halmac.mk

_BTC_FILES += hal/btc/halbtc8822ewifionly.o
ifeq ($(CONFIG_BT_COEXIST), y)
_BTC_FILES += hal/btc/halbtccommon.o \
				hal/btc/halbtc8822e.o
endif
