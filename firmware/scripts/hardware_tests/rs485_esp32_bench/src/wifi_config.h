// WiFi credentials and log-hub network config.
//
// Fill in your bench network values here, or pass them as PlatformIO build flags:
//   build_flags = ... -DWIFI_SSID="MyNet" -DWIFI_PASS="secret" -DLOG_HOST_IP="192.168.1.100"
//
// The placeholder defaults below let the firmware compile out of the box;
// the node will fail to connect at runtime until real values are set.

#pragma once

#ifndef WIFI_SSID
#define WIFI_SSID "CAP_P9"
#endif

#ifndef WIFI_PASS
#define WIFI_PASS "Pedepsit1!"
#endif

// PC / host IP where rs485_log_hub.py is listening.
#ifndef LOG_HOST_IP
#define LOG_HOST_IP "10.12.24.25"
#endif

// UDP port must match LOG_UDP_PORT in rs485_log_hub.py (default 9000).
#ifndef LOG_UDP_PORT
#define LOG_UDP_PORT 9000
#endif
