#pragma once
#include <cstdint>
#include <cstring>

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1

typedef int esp_now_send_status_t;
#define ESP_NOW_SEND_SUCCESS 0
#define ESP_NOW_SEND_FAIL    1

// IDF 5.x recv callback info struct
typedef struct {
    uint8_t* src_addr;
    uint8_t* des_addr;
    int      rssi;
} esp_now_recv_info_t;

typedef void (*esp_now_recv_cb_t)(const esp_now_recv_info_t* info,
                                   const uint8_t* data, int len);
typedef void (*esp_now_send_cb_t)(const uint8_t* mac,
                                   esp_now_send_status_t status);

struct esp_now_peer_info_t {
    uint8_t peer_addr[6];
    uint8_t lmk[16];
    uint8_t channel;
    int     ifidx;
    bool    encrypt;
};

// --- Send inspection for tests ---
struct EspNowSendRecord {
    uint8_t dest[6];
    uint8_t data[64];
    int     len;
    int     callCount;
    void clear() { memset(this, 0, sizeof(*this)); }
};

extern EspNowSendRecord mock_espnow_last_send;
extern esp_now_recv_cb_t mock_espnow_recv_cb;

inline esp_err_t esp_now_init()   { return ESP_OK; }
inline esp_err_t esp_now_deinit() { return ESP_OK; }
inline esp_err_t esp_now_register_recv_cb(esp_now_recv_cb_t cb) {
    mock_espnow_recv_cb = cb;
    return ESP_OK;
}
inline esp_err_t esp_now_register_send_cb(esp_now_send_cb_t) { return ESP_OK; }
inline esp_err_t esp_now_add_peer(const esp_now_peer_info_t*) { return ESP_OK; }
inline esp_err_t esp_now_del_peer(const uint8_t*)              { return ESP_OK; }
inline esp_err_t esp_now_set_pmk(const uint8_t*)               { return ESP_OK; }

inline esp_err_t esp_now_send(const uint8_t* peer_addr,
                               const uint8_t* data, int len) {
    if (peer_addr) memcpy(mock_espnow_last_send.dest, peer_addr, 6);
    if (data && len > 0 && len <= (int)sizeof(mock_espnow_last_send.data))
        memcpy(mock_espnow_last_send.data, data, len);
    mock_espnow_last_send.len = len;
    mock_espnow_last_send.callCount++;
    return ESP_OK;
}
