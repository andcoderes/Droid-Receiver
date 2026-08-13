#pragma once
#include "Arduino.h"
#include <cstring>
#include <string>

// Forward declarations
class BLEServer;
class BLEService;
class BLECharacteristic;
class BLEAdvertising;

// --- BLE constants ---
#define BLE_HS_IO_NO_INPUT_OUTPUT 0

// --- Callback interfaces ---
class BLEServerCallbacks {
public:
    virtual ~BLEServerCallbacks() {}
    virtual void onConnect(BLEServer* pServer) {}
    virtual void onDisconnect(BLEServer* pServer) {}
};

class BLECharacteristicCallbacks {
public:
    virtual ~BLECharacteristicCallbacks() {}
    virtual void onWrite(BLECharacteristic* pCharacteristic) {}
    virtual void onRead(BLECharacteristic* pCharacteristic) {}
};

// --- BLECharacteristic ---
class BLECharacteristic {
public:
    static const int PROPERTY_READ     = 0x01;
    static const int PROPERTY_WRITE    = 0x02;
    static const int PROPERTY_NOTIFY   = 0x04;
    static const int PROPERTY_INDICATE = 0x08;

    void setCallbacks(BLECharacteristicCallbacks* cb) { callbacks_ = cb; }
    void notify() { notifyCount_++; }

    void setValue(const uint8_t* data, int len) {
        if (len > 0 && len < (int)sizeof(valueBuf_)) {
            memcpy(valueBuf_, data, len);
            valueBuf_[len] = '\0';
            valueLen_ = len;
        }
    }
    void setValue(const char* s) {
        int len = (int)strlen(s);
        setValue((const uint8_t*)s, len);
    }

    std::string getValue() const {
        return std::string(valueBuf_, valueLen_);
    }

    // Test inspection
    int notifyCount_ = 0;
    char valueBuf_[256] = {};
    int valueLen_ = 0;
    BLECharacteristicCallbacks* callbacks_ = nullptr;
};

// --- BLEService ---
class BLEService {
public:
    BLECharacteristic* createCharacteristic(const char* uuid, int props) {
        (void)uuid; (void)props;
        return &char_;
    }
    void start() {}
    BLECharacteristic char_;
};

// --- BLEAdvertising ---
class BLEAdvertising {
public:
    void addServiceUUID(const char*) {}
    void setScanResponse(bool) {}
    void setMinPreferred(uint8_t) {}
    void start() {}
};

// --- BLEServer ---
class BLEServer {
public:
    void setCallbacks(BLEServerCallbacks* cb) { callbacks_ = cb; }
    BLEService* createService(const char* uuid) {
        (void)uuid;
        return &service_;
    }
    BLEService service_;
    BLEServerCallbacks* callbacks_ = nullptr;
};

// --- BLEDevice (static interface) ---
class BLEDevice {
public:
    static void init(const char*) {}
    static void setMTU(int) {}
    static BLEServer* createServer() {
        static BLEServer server;
        return &server;
    }
    static BLEAdvertising* getAdvertising() {
        static BLEAdvertising adv;
        return &adv;
    }
    static void startAdvertising() {}
    static void stopAdvertising() {}
    static void setSecurityAuth(bool, bool, bool) {}
    static void setSecurityIOCap(int) {}
};

// Additional includes the real code expects
// BLEServer.h, BLEUtils.h are included via BLEDevice.h in real code
#define BLEServer_h
#define BLEUtils_h
