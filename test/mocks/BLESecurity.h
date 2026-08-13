#pragma once

// Minimal mock of NimBLE-Arduino's BLESecurity for native unit tests.
// Only the static methods BleController actually calls are stubbed.

#define ESP_IO_CAP_NONE 3

class BLESecurity {
public:
    static void setAuthenticationMode(bool bonding, bool mitm, bool sc) {}
    static void setCapability(int capability) {}
};
