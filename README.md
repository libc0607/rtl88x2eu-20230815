# rtl88x2eu-20230815
Linux Driver for WiFi Adapters that are based on the RTL8812EU and RTL8822EU Chipsets - v5.15.0.1  
``` rtl88x2EU_rtl88x2CU-VE_WiFi_linux_v5.15.0.1-186-g768722062.20230815_COEX20230616-330a-beta.tar.gz ```  

I've asked LB-LINK (a Wi-Fi module vendor) for any RTL8812EU driver. Then he send me this tar.   
So, it should work with RTL8812EU and RTL8822EU.   

According to the file name, it may work with RTL8812CU or RTL8822CU (if exists). But you should use in-kernel driver instead.

However, it's not been tested yet. Will update this readme till I get my RTL8812EU hardware.   

My personal to-do list: 
- Figure out how to make the packet injection works correctly (idk if i'm doing it correctly -- needs help)
- Test with any OpenIPC camera
- Figure out how to transmit in 5M/10M bandwidth (The feature is claimed to be supported in the module's product page. see [CONFIG_NARROWBAND_SUPPORTING](https://github.com/search?q=repo%3Alibc0607%2Frtl88x2eu-20230815+CONFIG_NARROWBAND_SUPPORTING&type=code) and [hal8822e_fw_10M.c](https://github.com/libc0607/rtl88x2eu-20230815/blob/v5.15.0.1/hal/rtl8822e/hal8822e_fw_10M.c))
- Build with dkms
- Test on more architecture, kernels, and distributions
- An open source hardware design using the LB-LINK module, then share it somewhere else

PRs welcome.
