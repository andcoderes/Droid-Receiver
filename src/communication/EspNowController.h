#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "MessageTypes.h"
#include "../config.h"

class EspNowController {
public:
    using BodyTelemetryCallback = void (*)(const BodyTelemetry&);
    using HeadTelemetryCallback = void (*)(const HeadTelemetry&);

    void setup();
    void loop();

    bool sendToBody(const BodyCommand& cmd);
    bool sendToHead(const HeadCommand& cmd);

    bool isBodyConnected() const;
    bool isHeadConnected() const;

    void setBodyTelemetryCallback(BodyTelemetryCallback cb) { bodyCb_ = cb; }
    void setHeadTelemetryCallback(HeadTelemetryCallback cb) { headCb_ = cb; }

private:
    // IDF 5.x receive callback uses esp_now_recv_info_t
    static void onDataRecv(const esp_now_recv_info_t* info,
                           const uint8_t* data, int len);
    static void onDataSent(const esp_now_send_info_t* tx_info, esp_now_send_status_t status);

    BodyTelemetryCallback bodyCb_ = nullptr;
    HeadTelemetryCallback headCb_ = nullptr;

    // Receive buffers (written from callback, read from loop)
    static BodyTelemetry bodyBuf_;
    static HeadTelemetry headBuf_;
    static volatile bool bodyReady_;
    static volatile bool headReady_;
    static unsigned long lastBodyRecv_;
    static unsigned long lastHeadRecv_;
    static volatile bool bodyEverRecv_;
    static volatile bool headEverRecv_;

#ifdef UNIT_TEST
public:
    // Expose internals for testing
    static BodyTelemetry& testBodyBuf()       { return bodyBuf_; }
    static HeadTelemetry& testHeadBuf()       { return headBuf_; }
    static volatile bool& testBodyReady()     { return bodyReady_; }
    static volatile bool& testHeadReady()     { return headReady_; }
    static unsigned long& testLastBodyRecv()  { return lastBodyRecv_; }
    static unsigned long& testLastHeadRecv()  { return lastHeadRecv_; }
    static volatile bool& testBodyEverRecv()  { return bodyEverRecv_; }
    static volatile bool& testHeadEverRecv()  { return headEverRecv_; }

    static void callOnDataRecv(const esp_now_recv_info_t* info,
                               const uint8_t* data, int len) {
        onDataRecv(info, data, len);
    }
#endif
};
