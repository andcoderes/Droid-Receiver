#include <unity.h>
#include <cstring>
#include "communication/EspNowController.h"

static EspNowController espNow;

// Reset all static state before each test
void setUp(void) {
    mock_millis_value = 0;
    mock_espnow_last_send.clear();
    mock_espnow_recv_cb = nullptr;

    EspNowController::testBodyReady()    = false;
    EspNowController::testHeadReady()    = false;
    EspNowController::testBodyEverRecv() = false;
    EspNowController::testHeadEverRecv() = false;
    EspNowController::testLastBodyRecv() = 0;
    EspNowController::testLastHeadRecv() = 0;
    memset(&EspNowController::testBodyBuf(), 0, sizeof(BodyTelemetry));
    memset(&EspNowController::testHeadBuf(), 0, sizeof(HeadTelemetry));

    espNow = EspNowController();
}

void tearDown(void) {}

// ---------- isBodyConnected ----------

void test_body_not_connected_when_never_received() {
    TEST_ASSERT_FALSE(espNow.isBodyConnected());
}

void test_body_connected_within_timeout() {
    EspNowController::testBodyEverRecv() = true;
    EspNowController::testLastBodyRecv() = 1000;
    mock_millis_value = 4999;  // within 5000ms timeout
    TEST_ASSERT_TRUE(espNow.isBodyConnected());
}

void test_body_disconnected_after_timeout() {
    EspNowController::testBodyEverRecv() = true;
    EspNowController::testLastBodyRecv() = 1000;
    mock_millis_value = 6001;  // past 5000ms timeout
    TEST_ASSERT_FALSE(espNow.isBodyConnected());
}

// ---------- isHeadConnected ----------

void test_head_not_connected_when_never_received() {
    TEST_ASSERT_FALSE(espNow.isHeadConnected());
}

void test_head_connected_within_timeout() {
    EspNowController::testHeadEverRecv() = true;
    EspNowController::testLastHeadRecv() = 2000;
    mock_millis_value = 6999;
    TEST_ASSERT_TRUE(espNow.isHeadConnected());
}

void test_head_disconnected_after_timeout() {
    EspNowController::testHeadEverRecv() = true;
    EspNowController::testLastHeadRecv() = 2000;
    mock_millis_value = 7001;
    TEST_ASSERT_FALSE(espNow.isHeadConnected());
}

// ---------- onDataRecv dispatching ----------

void test_recv_body_telemetry() {
    // Call setup to register the recv callback
    espNow.setup();

    BodyTelemetry telem = {};
    telem.msgType = 4;
    telem.connected = 1;
    telem.motorsActive = 1;
    telem.uptimeMs = 12345;

    // UNIT_TEST config.h dummies BODY_MAC/HEAD_MAC to all-zero.
    uint8_t senderMac[6] = {0};
    esp_now_recv_info_t info = {};
    info.src_addr = senderMac;
    EspNowController::callOnDataRecv(&info,
        (const uint8_t*)&telem, sizeof(BodyTelemetry));

    TEST_ASSERT_TRUE(EspNowController::testBodyReady());
    TEST_ASSERT_TRUE(EspNowController::testBodyEverRecv());
    TEST_ASSERT_EQUAL(4, EspNowController::testBodyBuf().msgType);
    TEST_ASSERT_EQUAL(12345, EspNowController::testBodyBuf().uptimeMs);
}

void test_recv_head_telemetry() {
    HeadTelemetry telem = {};
    telem.msgType = 2;
    telem.connected = 1;
    telem.animationRunning = 0;
    telem.uptimeMs = 54321;

    uint8_t senderMac[6] = {0};
    esp_now_recv_info_t info = {};
    info.src_addr = senderMac;
    EspNowController::callOnDataRecv(&info,
        (const uint8_t*)&telem, sizeof(HeadTelemetry));

    TEST_ASSERT_TRUE(EspNowController::testHeadReady());
    TEST_ASSERT_TRUE(EspNowController::testHeadEverRecv());
    TEST_ASSERT_EQUAL(2, EspNowController::testHeadBuf().msgType);
    TEST_ASSERT_EQUAL(54321, EspNowController::testHeadBuf().uptimeMs);
}

void test_recv_ignores_unknown_msg_type() {
    uint8_t data[8] = {};
    data[0] = 99;  // unknown msgType

    uint8_t senderMac[6] = {0};
    esp_now_recv_info_t info = {};
    info.src_addr = senderMac;
    EspNowController::callOnDataRecv(&info, data, 8);

    TEST_ASSERT_FALSE(EspNowController::testBodyReady());
    TEST_ASSERT_FALSE(EspNowController::testHeadReady());
}

void test_recv_ignores_mismatched_sender() {
    // Correct msgType/length, but sender MAC doesn't match BODY_MAC.
    BodyTelemetry telem = {};
    telem.msgType = 4;
    telem.uptimeMs = 111;

    uint8_t wrongMac[6] = {1, 2, 3, 4, 5, 6};
    esp_now_recv_info_t info = {};
    info.src_addr = wrongMac;
    EspNowController::callOnDataRecv(&info,
        (const uint8_t*)&telem, sizeof(BodyTelemetry));

    TEST_ASSERT_FALSE(EspNowController::testBodyReady());
    TEST_ASSERT_FALSE(EspNowController::testBodyEverRecv());
}

void test_recv_ignores_null_src_addr() {
    BodyTelemetry telem = {};
    telem.msgType = 4;

    esp_now_recv_info_t info = {};  // src_addr left null
    EspNowController::callOnDataRecv(&info,
        (const uint8_t*)&telem, sizeof(BodyTelemetry));

    TEST_ASSERT_FALSE(EspNowController::testBodyReady());
}

// ---------- loop dispatching ----------

void test_loop_calls_body_callback() {
    bool called = false;
    BodyTelemetry received = {};

    espNow.setBodyTelemetryCallback([](const BodyTelemetry& t) {
        // Can't capture in C function pointer, use static
    });

    // Use a static variable approach
    static BodyTelemetry staticReceived;
    static bool staticCalled;
    staticCalled = false;
    memset(&staticReceived, 0, sizeof(staticReceived));

    espNow.setBodyTelemetryCallback([](const BodyTelemetry& t) {
        staticReceived = t;
        staticCalled = true;
    });

    // Simulate received data
    EspNowController::testBodyBuf().msgType = 4;
    EspNowController::testBodyBuf().uptimeMs = 9999;
    EspNowController::testBodyReady() = true;

    espNow.loop();

    TEST_ASSERT_TRUE(staticCalled);
    TEST_ASSERT_EQUAL(4, staticReceived.msgType);
    TEST_ASSERT_EQUAL(9999, staticReceived.uptimeMs);
}

void test_loop_calls_head_callback() {
    static HeadTelemetry staticReceived;
    static bool staticCalled;
    staticCalled = false;
    memset(&staticReceived, 0, sizeof(staticReceived));

    espNow.setHeadTelemetryCallback([](const HeadTelemetry& t) {
        staticReceived = t;
        staticCalled = true;
    });

    EspNowController::testHeadBuf().msgType = 2;
    EspNowController::testHeadBuf().uptimeMs = 7777;
    EspNowController::testHeadReady() = true;

    espNow.loop();

    TEST_ASSERT_TRUE(staticCalled);
    TEST_ASSERT_EQUAL(2, staticReceived.msgType);
    TEST_ASSERT_EQUAL(7777, staticReceived.uptimeMs);
}

void test_loop_no_crash_without_callbacks() {
    // No callbacks set, ready flags true — should not crash
    EspNowController::testBodyReady() = true;
    EspNowController::testHeadReady() = true;

    espNow.loop();

    // If we get here without segfault, test passes
    TEST_ASSERT_FALSE(EspNowController::testBodyReady());
    TEST_ASSERT_FALSE(EspNowController::testHeadReady());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_body_not_connected_when_never_received);
    RUN_TEST(test_body_connected_within_timeout);
    RUN_TEST(test_body_disconnected_after_timeout);
    RUN_TEST(test_head_not_connected_when_never_received);
    RUN_TEST(test_head_connected_within_timeout);
    RUN_TEST(test_head_disconnected_after_timeout);
    RUN_TEST(test_recv_body_telemetry);
    RUN_TEST(test_recv_head_telemetry);
    RUN_TEST(test_recv_ignores_unknown_msg_type);
    RUN_TEST(test_recv_ignores_mismatched_sender);
    RUN_TEST(test_recv_ignores_null_src_addr);
    RUN_TEST(test_loop_calls_body_callback);
    RUN_TEST(test_loop_calls_head_callback);
    RUN_TEST(test_loop_no_crash_without_callbacks);
    return UNITY_END();
}
