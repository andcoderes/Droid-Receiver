#pragma once
#include <cstdint>

typedef int esp_err_t;
#ifndef ESP_OK
#define ESP_OK 0
#endif

#define WIFI_PS_NONE 0

inline esp_err_t esp_wifi_set_mac(int ifx, const uint8_t* mac) {
    (void)ifx; (void)mac;
    return ESP_OK;
}

inline esp_err_t esp_wifi_set_ps(int type) {
    (void)type;
    return ESP_OK;
}
