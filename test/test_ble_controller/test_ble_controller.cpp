#include <unity.h>
#include <cstring>
#include "communication/BleController.h"

static BleController ble;

void setUp(void) {
    ble = BleController();
}

void tearDown(void) {}

// ---------- Connection state ----------

void test_starts_disconnected() {
    TEST_ASSERT_FALSE(ble.isConnected());
}

void test_on_connect_sets_connected() {
    ble.callOnConnect();
    TEST_ASSERT_TRUE(ble.isConnected());
}

void test_on_disconnect_clears_connected() {
    ble.callOnConnect();
    TEST_ASSERT_TRUE(ble.isConnected());
    ble.callOnDisconnect();
    TEST_ASSERT_FALSE(ble.isConnected());
}

// ---------- Message buffering ----------

void test_on_write_buffers_message() {
    const char* msg = "{\"s\":1}";
    ble.callOnWrite(msg, strlen(msg));

    TEST_ASSERT_TRUE(ble.testMsgReady());
    TEST_ASSERT_EQUAL(strlen(msg), ble.testMsgLen());
    TEST_ASSERT_EQUAL_STRING(msg, ble.testMsgBuf());
}

void test_loop_dispatches_callback() {
    static bool called = false;
    static char receivedBuf[256] = {};
    static int receivedLen = 0;
    called = false;

    ble.setMessageCallback([](const char* json, int len) {
        called = true;
        strncpy(receivedBuf, json, sizeof(receivedBuf) - 1);
        receivedLen = len;
    });

    const char* msg = "{\"s\":0,\"p\":[\"y\"],\"m\":[]}";
    ble.callOnWrite(msg, strlen(msg));
    ble.loop();

    TEST_ASSERT_TRUE(called);
    TEST_ASSERT_EQUAL_STRING(msg, receivedBuf);
    TEST_ASSERT_EQUAL(strlen(msg), receivedLen);
    TEST_ASSERT_FALSE(ble.testMsgReady());  // cleared after dispatch
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_starts_disconnected);
    RUN_TEST(test_on_connect_sets_connected);
    RUN_TEST(test_on_disconnect_clears_connected);
    RUN_TEST(test_on_write_buffers_message);
    RUN_TEST(test_loop_dispatches_callback);
    return UNITY_END();
}
