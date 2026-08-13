#include "EspNowController.h"
#include <esp_wifi.h>

// Static member definitions
BodyTelemetry  EspNowController::bodyBuf_       = {};
HeadTelemetry  EspNowController::headBuf_       = {};
volatile bool  EspNowController::bodyReady_     = false;
volatile bool  EspNowController::headReady_     = false;
unsigned long  EspNowController::lastBodyRecv_  = 0;
unsigned long  EspNowController::lastHeadRecv_  = 0;
volatile bool  EspNowController::bodyEverRecv_  = false;
volatile bool  EspNowController::headEverRecv_  = false;

// Cross-core critical sections guarding bodyBuf_/headBuf_. noInterrupts()/
// interrupts() only mask the current core on the dual-core ESP32 and do not
// protect against onDataRecv() (WiFi task, can run on either core) writing
// concurrently with loop() (Arduino task) reading.
static portMUX_TYPE bodyMux_ = portMUX_INITIALIZER_UNLOCKED;
static portMUX_TYPE headMux_ = portMUX_INITIALIZER_UNLOCKED;

// ---- ESP-NOW callbacks (run in the WiFi task) ----

void EspNowController::onDataRecv(const esp_now_recv_info_t* info,
                                   const uint8_t* data, int len) {
    if (!info || !info->src_addr || len < 1) return;

    int8_t msgType = (int8_t)data[0];

    if (msgType == 4 && len == sizeof(BodyTelemetry) &&
        memcmp(info->src_addr, BODY_MAC, 6) == 0) {
        portENTER_CRITICAL(&bodyMux_);
        memcpy(&bodyBuf_, data, sizeof(BodyTelemetry));
        portEXIT_CRITICAL(&bodyMux_);
        lastBodyRecv_ = millis();
        bodyEverRecv_ = true;
        bodyReady_    = true;
    }
    else if (msgType == 2 && len == sizeof(HeadTelemetry) &&
             memcmp(info->src_addr, HEAD_MAC, 6) == 0) {
        portENTER_CRITICAL(&headMux_);
        memcpy(&headBuf_, data, sizeof(HeadTelemetry));
        portEXIT_CRITICAL(&headMux_);
        lastHeadRecv_ = millis();
        headEverRecv_ = true;
        headReady_    = true;
    }
}

void EspNowController::onDataSent(const esp_now_send_info_t* tx_info,
                                   esp_now_send_status_t status) {
    // Silently ignore — peers may be powered off
}

// ---- Public API ----

void EspNowController::setup() {
    // WiFi STA mode is set in main.cpp before this call.

    // Override this board's WiFi MAC so body/head recognise us as CONTROLLER_MAC
    uint8_t mac[6];
    memcpy(mac, CONTROLLER_MAC, 6);
    esp_wifi_set_mac(WIFI_IF_STA, mac);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW: init FAILED");
        return;
    }

    esp_now_set_pmk(PMK_KEY);

    // Register body peer (encrypted)
    esp_now_peer_info_t peer = {};
    memcpy(peer.lmk, LMK_KEY, 16);
    peer.channel = 0;
    peer.encrypt = true;
    peer.ifidx   = WIFI_IF_STA;

    memcpy(peer.peer_addr, BODY_MAC, 6);
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("ESP-NOW: failed to add body peer");
    }

    // Register head peer (encrypted, same LMK)
    memcpy(peer.peer_addr, HEAD_MAC, 6);
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("ESP-NOW: failed to add head peer");
    }

    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);

    Serial.printf("ESP-NOW: ready  MAC=%s\n", WiFi.macAddress().c_str());
}

void EspNowController::loop() {
    if (bodyReady_) {
        bodyReady_ = false;
        BodyTelemetry t;
        portENTER_CRITICAL(&bodyMux_);
        memcpy(&t, (const void*)&bodyBuf_, sizeof(t));
        portEXIT_CRITICAL(&bodyMux_);
        if (bodyCb_) bodyCb_(t);
    }
    if (headReady_) {
        headReady_ = false;
        HeadTelemetry t;
        portENTER_CRITICAL(&headMux_);
        memcpy(&t, (const void*)&headBuf_, sizeof(t));
        portEXIT_CRITICAL(&headMux_);
        if (headCb_) headCb_(t);
    }
}

bool EspNowController::sendToBody(const BodyCommand& cmd) {
    return esp_now_send(BODY_MAC, (const uint8_t*)&cmd, sizeof(cmd)) == ESP_OK;
}

bool EspNowController::sendToHead(const HeadCommand& cmd) {
    return esp_now_send(HEAD_MAC, (const uint8_t*)&cmd, sizeof(cmd)) == ESP_OK;
}

static bool notTimedOut(bool everRecv, unsigned long lastRecv) {
    return everRecv && (millis() - lastRecv < CONNECTION_TIMEOUT_MS);
}

bool EspNowController::isBodyConnected() const {
    return notTimedOut(bodyEverRecv_, lastBodyRecv_);
}

bool EspNowController::isHeadConnected() const {
    return notTimedOut(headEverRecv_, lastHeadRecv_);
}
