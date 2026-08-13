#pragma once
#include <Arduino.h>

// --- Status codes ---
#define STATUS_BUTTONS     0
#define STATUS_MOVEMENT    1
#define STATUS_SETTINGS    2
#define STATUS_AP_CONTROL  3
#define STATUS_CONNECTION -1
#define STATUS_DEBUG       99

// ========== Controller -> Body (msgType = 3, 26 bytes) ==========
struct BodyCommand {
    int8_t  msgType;           // always 3
    int8_t  status;            // 0=buttons, 1=movement, 2=settings, -1=connection, 3=AP
    int8_t  lx;                // drive X: -100 to 100
    int8_t  ly;                // drive Y: -100 to 100
    int8_t  domeSpeed;         // dome rotation: -100 to 100
    uint8_t audioTrack;        // 0=none, 1-255=play track
    uint8_t volume;            // 0-30
    uint8_t bubbles;           // 0=off, 1=on
    char    button[8];         // servo trigger (e.g. "y", "du")
    int16_t macro[4];          // macro IDs
    int8_t  connectionStatus;  // 0=disconnected, 1=connected
    uint8_t apRequested;       // 1=start AP
} __attribute__((packed));     // 26 bytes

// ========== Body -> Controller (msgType = 4, 8 bytes) ==========
struct BodyTelemetry {
    int8_t   msgType;          // always 4
    uint8_t  connected;
    uint8_t  motorsActive;
    uint8_t  audioPlaying;
    uint32_t uptimeMs;
} __attribute__((packed));     // 8 bytes

// ========== Controller -> Head (msgType = 1, 22 bytes) ==========
struct HeadCommand {
    int8_t  msgType;           // always 1
    int8_t  status;            // 0=buttons, 2=settings, -1=connection, 99=debug
    char    button[8];         // null-terminated button string
    int16_t macro[4];          // macro IDs
    uint8_t volume;
    uint8_t idleMode;
    int8_t  connectionStatus;  // 0=disconnected, 1=connected
} __attribute__((packed));

// ========== Head -> Controller (msgType = 2, 8 bytes) ==========
struct HeadTelemetry {
    int8_t   msgType;          // always 2
    uint8_t  connected;
    uint8_t  animationRunning;
    uint8_t  padding;
    uint32_t uptimeMs;
} __attribute__((packed));     // 8 bytes
