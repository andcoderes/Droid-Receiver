#include "Arduino.h"
#include "WiFi.h"
#include "esp_now.h"

unsigned long mock_millis_value = 0;

HardwareSerial Serial;
HardwareSerial Serial1;
HardwareSerial Serial2;

WiFiClass WiFi;

EspNowSendRecord mock_espnow_last_send = {};
esp_now_recv_cb_t mock_espnow_recv_cb = nullptr;
