#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "config.h"
#include "communication/MessageTypes.h"
#include "communication/BleController.h"
#include "communication/EspNowController.h"
#include "communication/CommandParser.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
BleController    ble;
EspNowController espNow;
CommandParser    parser;

static bool     g_bleWasConnected = false;
static bool     g_bodyWasConnected = false;
static bool     g_headWasConnected = false;

// Telemetry forwarding
static unsigned long g_lastTelemetryNotify = 0;
static BodyTelemetry g_lastBodyTelem = {};
static HeadTelemetry g_lastHeadTelem = {};
static bool          g_telemDirty = false;  // set by either peer, sent together

// Logs a labeled connect/disconnect transition when `now` differs from `was`.
static void logTransition(const char* label, bool now, bool& was) {
    if (now == was) return;
    was = now;
    Serial.printf("%s %s\n", label, now ? "connected" : "disconnected");
}

// ---------------------------------------------------------------------------
// BLE -> ESP-NOW  command translation
// ---------------------------------------------------------------------------

static void dispatchResult(const ParseResult& r) {
    if (r.sendBody && !espNow.sendToBody(r.bodyCmd)) {
        Serial.println("ESP-NOW: send to body failed");
    }
    if (r.sendHead && !espNow.sendToHead(r.headCmd)) {
        Serial.println("ESP-NOW: send to head failed");
    }
    if (r.isPing) ble.sendNotification(r.pingResponse);
}

static void onBleMessage(const char* json, int len) {
    Serial.printf("BLE: recv %d bytes: %s\n", len, json);

    ParseResult r = parser.parseMessage(json, len,
                                        espNow.isBodyConnected(),
                                        espNow.isHeadConnected());
    Serial.printf("Parsed: status=%d button='%s' macro=[%d,%d,%d,%d] bubbles=%d\n",
                  r.bodyCmd.status, r.bodyCmd.button,
                  r.bodyCmd.macro[0], r.bodyCmd.macro[1],
                  r.bodyCmd.macro[2], r.bodyCmd.macro[3],
                  r.bodyCmd.bubbles);
    dispatchResult(r);
}

// ---------------------------------------------------------------------------
// ESP-NOW -> BLE  telemetry forwarding
// ---------------------------------------------------------------------------

static void onBodyTelemetry(const BodyTelemetry& t) {
    Serial.printf("ESP-NOW: body recv  connected=%u motors=%u audio=%u uptime=%lums\n",
                  t.connected, t.motorsActive, t.audioPlaying, (unsigned long)t.uptimeMs);
    g_lastBodyTelem = t;
    g_telemDirty = true;
}

static void onHeadTelemetry(const HeadTelemetry& t) {
    Serial.printf("ESP-NOW: head recv  connected=%u animation=%u uptime=%lums\n",
                  t.connected, t.animationRunning, (unsigned long)t.uptimeMs);
    g_lastHeadTelem = t;
    g_telemDirty = true;
}

static void notifyAppTelemetry() {
    if (!ble.isConnected()) return;
    if (!g_telemDirty) return;

    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"s\":2,\"v\":%u,\"p\":%u,\"hv\":%u,\"ha\":%u}",
             g_lastBodyTelem.connected,
             g_lastBodyTelem.motorsActive,
             g_lastHeadTelem.connected,
             g_lastHeadTelem.animationRunning);
    ble.sendNotification(buf);

    g_telemDirty = false;
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(500);

    // WiFi STA mode for ESP-NOW (no AP needed on the receiver)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_ps(WIFI_PS_NONE);

    // ESP-NOW (sets CONTROLLER_MAC internally)
    espNow.setup();
    espNow.setBodyTelemetryCallback(onBodyTelemetry);
    espNow.setHeadTelemetryCallback(onHeadTelemetry);

    // BLE
    ble.setup();
    ble.setMessageCallback(onBleMessage);

    Serial.println("Chopper V2 Receiver ready");
}

void loop() {
    // Process buffered BLE messages
    ble.loop();

    // Process buffered ESP-NOW telemetry
    espNow.loop();

    // Detect BLE connect / disconnect transitions
    bool bleNow = ble.isConnected();
    if (bleNow != g_bleWasConnected) {
        g_bleWasConnected = bleNow;
        ParseResult r = parser.setConnected(bleNow);
        dispatchResult(r);
    }

    // Detect ESP-NOW body / head connect / disconnect transitions
    logTransition("ESP-NOW: body", espNow.isBodyConnected(), g_bodyWasConnected);
    logTransition("ESP-NOW: head", espNow.isHeadConnected(), g_headWasConnected);

    // Periodically forward telemetry to the app
    unsigned long now = millis();
    if (now - g_lastTelemetryNotify >= TELEMETRY_NOTIFY_INTERVAL_MS) {
        notifyAppTelemetry();
        g_lastTelemetryNotify = now;
    }
}
