#pragma once

#include <stdint.h>
#include <stdarg.h>

bool WifiLogBegin(uint8_t node_id);
bool WifiLogConnected();
void WifiLog(const char* tag, const char* fmt, ...);
void WifiLogPoll();
