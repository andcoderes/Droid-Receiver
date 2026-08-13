#include "CommandParser.h"
#include <string.h>
#include <stdlib.h>

uint8_t CommandParser::mapVolume(uint8_t appVol) {
    return (uint8_t)((uint16_t)appVol * 30 / 255);
}

bool CommandParser::isNumeric(const char* s) {
    if (!s || !*s) return false;
    for (const char* p = s; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
    }
    return true;
}

void CommandParser::addMacro(BodyCommand& bcmd, HeadCommand& hcmd, int16_t id, int& idx) {
    if (idx >= 4) return;
    bcmd.macro[idx] = id;
    hcmd.macro[idx] = id;
    idx++;
}

BodyCommand CommandParser::makeBodyCmd(int8_t status) const {
    BodyCommand cmd = {};
    cmd.msgType = 3;
    cmd.status  = status;
    cmd.bubbles = bubbles_ ? 1 : 0;
    cmd.volume  = volume_;
    return cmd;
}

HeadCommand CommandParser::makeHeadCmd(int8_t status) const {
    HeadCommand cmd = {};
    cmd.msgType  = 1;
    cmd.status   = status;
    cmd.volume   = volume_;
    cmd.idleMode = idleMode_ ? 1 : 0;
    return cmd;
}

ParseResult CommandParser::parseMessage(const char* json, int len,
                                        bool bodyConnected,
                                        bool headConnected) {
    ParseResult result = {};

    doc_.clear();
    if (deserializeJson(doc_, json, len) != DeserializationError::Ok) {
        return result;
    }
    JsonDocument& doc = doc_;

    JsonVariant statusField = doc["s"];
    if (statusField.isNull() || !statusField.is<int>()) {
        return result;  // missing/malformed status — reject rather than guess
    }
    int status = statusField.as<int>();

    switch (status) {

    // ---- Movement (left stick + right stick) -> body only ----
    case 1: {
        BodyCommand cmd = makeBodyCmd(STATUS_MOVEMENT);
        cmd.lx        = constrain((int)(doc["l"][0] | 0), -100, 100);
        cmd.ly        = constrain((int)(doc["l"][1] | 0), -100, 100);
        cmd.domeSpeed = constrain((int)(doc["r"][0] | 0), -100, 100);
        result.bodyCmd = cmd;
        result.sendBody = true;
        break;
    }

    // ---- Buttons / macros -> body + head ----
    case 0: {
        BodyCommand bcmd = makeBodyCmd(STATUS_BUTTONS);
        HeadCommand hcmd = makeHeadCmd(STATUS_BUTTONS);

        JsonArray pArr = doc["p"].as<JsonArray>();
        int macroIdx = 0;
        for (JsonVariant v : pArr) {
            const char* s = v.as<const char*>();
            if (!s) continue;

            if (isNumeric(s)) {
                addMacro(bcmd, hcmd, (int16_t)atoi(s), macroIdx);
            } else {
                strncpy(bcmd.button, s, sizeof(bcmd.button) - 1);
                memcpy(hcmd.button, bcmd.button, sizeof(hcmd.button));

                if (strcmp(s, "l2") == 0) {
                    bubbles_ = !bubbles_;
                    bcmd.bubbles = bubbles_ ? 1 : 0;
                }
            }
        }

        JsonArray mArr = doc["m"].as<JsonArray>();
        for (JsonVariant v : mArr) {
            int16_t id = v.as<int16_t>();
            if (id != 0) addMacro(bcmd, hcmd, id, macroIdx);
        }

        result.bodyCmd = bcmd;
        result.headCmd = hcmd;
        result.sendBody = true;
        result.sendHead = true;
        break;
    }

    // ---- Settings (volume, idle mode) -> body + head ----
    case 2: {
        uint8_t appVol = doc["v"] | 0;
        volume_   = mapVolume(appVol);
        idleMode_ = doc["i"] | false;

        BodyCommand bcmd = makeBodyCmd(STATUS_SETTINGS);
        result.bodyCmd = bcmd;
        result.sendBody = true;

        HeadCommand hcmd = makeHeadCmd(STATUS_SETTINGS);
        result.headCmd = hcmd;
        result.sendHead = true;
        break;
    }

    // ---- Audio (track playback) -> body only ----
    case 3: {
        BodyCommand cmd = makeBodyCmd(STATUS_BUTTONS);
        JsonArray mArr = doc["m"].as<JsonArray>();
        if (mArr.size() > 0) {
            cmd.audioTrack = mArr[0].as<uint8_t>();
        }
        result.bodyCmd = cmd;
        result.sendBody = true;
        break;
    }

    // ---- Ping -> respond locally ----
    case 4: {
        result.isPing = true;
        snprintf(result.pingResponse, sizeof(result.pingResponse),
                 "{\"s\":4,\"body\":%d,\"head\":%d}",
                 bodyConnected ? 1 : 0,
                 headConnected ? 1 : 0);
        break;
    }

    default:
        break;
    }

    return result;
}

ParseResult CommandParser::setConnected(bool connected) {
    ParseResult result = {};

    if (!connected) {
        bubbles_ = false;
    }

    int8_t connStatus = connected ? 1 : 0;

    BodyCommand bcmd = makeBodyCmd(STATUS_CONNECTION);
    bcmd.connectionStatus = connStatus;
    result.bodyCmd = bcmd;
    result.sendBody = true;

    HeadCommand hcmd = makeHeadCmd(STATUS_CONNECTION);
    hcmd.connectionStatus = connStatus;
    result.headCmd = hcmd;
    result.sendHead = true;

    return result;
}
