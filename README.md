# Merge v5.15.0.1-249-g9245f8bd9.20241218_COEX20240913-390e

Got a newer driver from LB-LINK support yesterday. And just merged changes to the main branch here  
NEEDS TEST, use it at your own risk!

Original driver tar: [rtl88x2EU_rtl88x2CU-VE_WiFi_linux_v5.15.0.1-249-g9245f8bd9.20241218_COEX20240913-390e.tar.gz](https://github.com/user-attachments/files/20148280/rtl88x2EU_rtl88x2CU-VE_WiFi_linux_v5.15.0.1-249-g9245f8bd9.20241218_COEX20240913-390e.tar.gz) 

TX 5MHz: 
 - `iw 5MHz` + `wfb_tx -B 20`, not working, single tone output

TX 40MHz:
 - `iw HT40-/+` + `wfb_tx -B 40`, not good, glitches
 - `iw 80MHz` + `wfb_tx -B 40`, **seems good, needs more test**

TX 80MHz:
 - `iw 80MHz` + `wfb_tx -B 80`, **seems good, needs more test**

# Release History (by Realtek):  
v5.15.0.1-197-ge90764eb9.20231128_COEX20230616-330a
1. Fix WOW download FW fail
2. Fix kernel panics after driver removed
3. Fix SA query timer always reset
4. Update BB parameter v49
5. Update RF parameter v33
6. Support SRRC Certification

v5.15.0.1-229-g8d59e1617.20240528_COEX20240327-340c
1. Support Android 14
2. Support kernel 6.6
3. Update BB parameter v49
4. Update RF parameter v33
5. Update FW version v1.22
6. Fix kernel panic during connecting
7. Fix WPA3 auth TX fail
8. Fix receive unexpected AMSDU packet

v5.15.0.1-249-g9245f8bd9.20241218_COEX20240913-390e
1. Update BB parameter v57
2. Update RF parameter v40
3. Update FW version v1.27
4. Fix magic pakct wakeup fail with PMF AP
5. Fix high power consumption for WoW
6. Fix unexpected deauth for WoW
7. Add efem setting
8. Fix BTC setting to avoid RX fail for efem
9. Fix kernel panic for cfg80211_connect_done
10. Support Ubuntu 24.10 (Kernel 6.11)
