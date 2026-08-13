#pragma once
#include <Arduino.h>

// --- BLE ---
#define BLE_DEVICE_NAME     "Chopper Droid"
#define BLE_MTU_SIZE        128
// SERVICE_UUID / CHARACTERISTIC_UUID come from secrets.h (must match the
// DroidController app's copies) — see README.

// --- Secrets (ESP-NOW keys, peer MACs, BLE UUIDs) ---
// CONTROLLER_MAC: this device's WiFi STA MAC for ESP-NOW. Body and head
//   firmware have this hardcoded too; the receiver overwrites its own WiFi
//   STA MAC with this value on startup.
// BODY_MAC / HEAD_MAC: the actual MACs printed by body/head firmware on
//   boot. Run each board, open its Serial monitor, and look for:
//     "MAC Address: XX:XX:XX:XX:XX:XX"
#ifdef UNIT_TEST
// Dummy values for native unit tests — no real ESP-NOW traffic off-device.
static const uint8_t PMK_KEY[16] = {0};
static const uint8_t LMK_KEY[16] = {0};
static const uint8_t CONTROLLER_MAC[6] = {0};
static const uint8_t BODY_MAC[6] = {0};
static const uint8_t HEAD_MAC[6] = {0};
#define SERVICE_UUID        "00000000-0000-0000-0000-000000000000"
#define CHARACTERISTIC_UUID "00000000-0000-0000-0000-000000000000"
#else
#include "secrets.h"  // defines PMK_KEY, LMK_KEY, CONTROLLER_MAC, BODY_MAC,
                       // HEAD_MAC, SERVICE_UUID, CHARACTERISTIC_UUID —
                       // generated from .env, see README
#endif

// --- Timing ---
#define CONNECTION_TIMEOUT_MS         5000
#define TELEMETRY_NOTIFY_INTERVAL_MS  1000
