#pragma once
#include "MessageTypes.h"
#include <ArduinoJson.h>

struct ParseResult {
    BodyCommand bodyCmd;
    HeadCommand headCmd;
    bool sendBody;
    bool sendHead;
    // For ping responses
    bool isPing;
    char pingResponse[80];
};

class CommandParser {
public:
    CommandParser() = default;

    // Parse a JSON message from BLE and produce commands to send.
    // bodyConnected/headConnected are needed for ping responses.
    ParseResult parseMessage(const char* json, int len,
                             bool bodyConnected, bool headConnected);

    // Produce connection-status commands when BLE connects/disconnects.
    // Also resets bubbles on disconnect.
    ParseResult setConnected(bool connected);

    // Accessors for persistent state
    bool     getBubbles()  const { return bubbles_; }
    uint8_t  getVolume()   const { return volume_; }
    bool     getIdleMode() const { return idleMode_; }

private:
    static uint8_t mapVolume(uint8_t appVol);
    static bool isNumeric(const char* s);
    // Appends id to both commands' macro arrays if there's room, advancing idx.
    static void addMacro(BodyCommand& bcmd, HeadCommand& hcmd, int16_t id, int& idx);
    BodyCommand makeBodyCmd(int8_t status) const;
    HeadCommand makeHeadCmd(int8_t status) const;

    bool    bubbles_  = false;
    uint8_t volume_   = 15;
    bool    idleMode_ = false;
    // Reused across calls (JsonDocument's pool grows once and stays sized,
    // avoiding a malloc/free cycle on every parseMessage() call).
    JsonDocument doc_;
};
