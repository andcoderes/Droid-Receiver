#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLESecurity.h>

class BleController : public BLECharacteristicCallbacks, public BLEServerCallbacks {
public:
    using MessageCallback = void (*)(const char* json, int len);

    void setup();
    void loop();
    bool isConnected() const { return connected_; }
    void sendNotification(const char* data);
    void setMessageCallback(MessageCallback cb) { callback_ = cb; }

private:
    void onWrite(BLECharacteristic* pCharacteristic) override;
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

    // Copies data into msgBuf_ under lock and marks a message ready.
    // Shared by onWrite() and (in UNIT_TEST) callOnWrite().
    void storeMessage(const char* data, int len) {
        if (len <= 0 || len >= MAX_MSG_LEN) return;
        portENTER_CRITICAL(&msgMux_);
        memcpy(msgBuf_, data, len);
        msgBuf_[len] = '\0';
        msgLen_ = len;
        portEXIT_CRITICAL(&msgMux_);
        msgReady_ = true;
    }

    BLEServer* pServer_ = nullptr;
    BLECharacteristic* pCharacteristic_ = nullptr;
    MessageCallback callback_ = nullptr;
    bool connected_ = false;

    static const int MAX_MSG_LEN = 256;
    char msgBuf_[MAX_MSG_LEN];
    volatile int msgLen_ = 0;
    volatile bool msgReady_ = false;
    // Guards msgBuf_/msgLen_ between onWrite() (NimBLE host task) and
    // loop() (Arduino task) — they can run on different cores.
    portMUX_TYPE msgMux_ = portMUX_INITIALIZER_UNLOCKED;

#ifdef UNIT_TEST
public:
    bool& testConnected()          { return connected_; }
    volatile bool& testMsgReady()  { return msgReady_; }
    char* testMsgBuf()             { return msgBuf_; }
    volatile int& testMsgLen()     { return msgLen_; }

    void callOnConnect()    { onConnect(pServer_); }
    void callOnDisconnect() { onDisconnect(pServer_); }
    void callOnWrite(const char* data, int len) { storeMessage(data, len); }
#endif
};
