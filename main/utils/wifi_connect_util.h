
#ifndef WIFI_CONNECT_UTIL_H
#define WIFI_CONNECT_UTIL_H
#include "esp_wifi.h"


bool connect_to_wifi(wifi_config_t wifiConfig);
bool isWifiConnected();
void setWifiConnected(bool connected);


#endif // WIFI_CONNECT_UTIL_H