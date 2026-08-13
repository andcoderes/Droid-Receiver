#include <unity.h>
#include <cstring>
#include "communication/MessageTypes.h"

// ---------- Struct sizes ----------

void test_body_command_size() {
    TEST_ASSERT_EQUAL(26, sizeof(BodyCommand));
}

void test_body_telemetry_size() {
    TEST_ASSERT_EQUAL(8, sizeof(BodyTelemetry));
}

void test_head_command_size() {
    // 1+1+8+(4*2)+1+1+1 = 21 packed
    TEST_ASSERT_EQUAL(21, sizeof(HeadCommand));
}

void test_head_telemetry_size() {
    TEST_ASSERT_EQUAL(8, sizeof(HeadTelemetry));
}

// ---------- Field round-trips via memcpy ----------

void test_body_command_field_roundtrip() {
    BodyCommand src = {};
    src.msgType = 3;
    src.status = STATUS_MOVEMENT;
    src.lx = -100;
    src.ly = 100;
    src.domeSpeed = -50;
    src.audioTrack = 42;
    src.volume = 30;
    src.bubbles = 1;
    strncpy(src.button, "y", sizeof(src.button));
    src.macro[0] = 100;
    src.macro[1] = 200;
    src.macro[2] = 300;
    src.macro[3] = -1;
    src.connectionStatus = 1;
    src.apRequested = 1;

    BodyCommand dst;
    memcpy(&dst, &src, sizeof(BodyCommand));

    TEST_ASSERT_EQUAL(3, dst.msgType);
    TEST_ASSERT_EQUAL(STATUS_MOVEMENT, dst.status);
    TEST_ASSERT_EQUAL(-100, dst.lx);
    TEST_ASSERT_EQUAL(100, dst.ly);
    TEST_ASSERT_EQUAL(-50, dst.domeSpeed);
    TEST_ASSERT_EQUAL(42, dst.audioTrack);
    TEST_ASSERT_EQUAL(30, dst.volume);
    TEST_ASSERT_EQUAL(1, dst.bubbles);
    TEST_ASSERT_EQUAL_STRING("y", dst.button);
    TEST_ASSERT_EQUAL(100, dst.macro[0]);
    TEST_ASSERT_EQUAL(200, dst.macro[1]);
    TEST_ASSERT_EQUAL(300, dst.macro[2]);
    TEST_ASSERT_EQUAL(-1, dst.macro[3]);
    TEST_ASSERT_EQUAL(1, dst.connectionStatus);
    TEST_ASSERT_EQUAL(1, dst.apRequested);
}

void test_head_command_field_roundtrip() {
    HeadCommand src = {};
    src.msgType = 1;
    src.status = STATUS_BUTTONS;
    strncpy(src.button, "du", sizeof(src.button));
    src.macro[0] = 105;
    src.macro[1] = 106;
    src.volume = 20;
    src.idleMode = 1;
    src.connectionStatus = 1;

    HeadCommand dst;
    memcpy(&dst, &src, sizeof(HeadCommand));

    TEST_ASSERT_EQUAL(1, dst.msgType);
    TEST_ASSERT_EQUAL(STATUS_BUTTONS, dst.status);
    TEST_ASSERT_EQUAL_STRING("du", dst.button);
    TEST_ASSERT_EQUAL(105, dst.macro[0]);
    TEST_ASSERT_EQUAL(106, dst.macro[1]);
    TEST_ASSERT_EQUAL(20, dst.volume);
    TEST_ASSERT_EQUAL(1, dst.idleMode);
    TEST_ASSERT_EQUAL(1, dst.connectionStatus);
}

// ---------- Status constants ----------

void test_status_constants() {
    TEST_ASSERT_EQUAL(0, STATUS_BUTTONS);
    TEST_ASSERT_EQUAL(1, STATUS_MOVEMENT);
    TEST_ASSERT_EQUAL(2, STATUS_SETTINGS);
    TEST_ASSERT_EQUAL(3, STATUS_AP_CONTROL);
    TEST_ASSERT_EQUAL(-1, STATUS_CONNECTION);
    TEST_ASSERT_EQUAL(99, STATUS_DEBUG);
}

// ---------- Button buffer boundary ----------

void test_button_buffer_max_length() {
    BodyCommand cmd = {};
    // button is char[8], so 7 chars + null
    strncpy(cmd.button, "abcdefg", sizeof(cmd.button) - 1);
    cmd.button[sizeof(cmd.button) - 1] = '\0';
    TEST_ASSERT_EQUAL(7, strlen(cmd.button));
    TEST_ASSERT_EQUAL_STRING("abcdefg", cmd.button);
}

// ---------- Macro array capacity ----------

void test_macro_array_capacity() {
    BodyCommand cmd = {};
    cmd.macro[0] = 100;
    cmd.macro[1] = 200;
    cmd.macro[2] = 300;
    cmd.macro[3] = 400;
    TEST_ASSERT_EQUAL(4, sizeof(cmd.macro) / sizeof(cmd.macro[0]));
    TEST_ASSERT_EQUAL(100, cmd.macro[0]);
    TEST_ASSERT_EQUAL(400, cmd.macro[3]);
}

// ---------- Drive range boundaries ----------

void test_drive_range_boundaries() {
    BodyCommand cmd = {};
    cmd.lx = -100;
    cmd.ly = 100;
    cmd.domeSpeed = -100;
    TEST_ASSERT_EQUAL(-100, cmd.lx);
    TEST_ASSERT_EQUAL(100, cmd.ly);
    TEST_ASSERT_EQUAL(-100, cmd.domeSpeed);

    cmd.lx = 0;
    cmd.ly = 0;
    cmd.domeSpeed = 100;
    TEST_ASSERT_EQUAL(0, cmd.lx);
    TEST_ASSERT_EQUAL(0, cmd.ly);
    TEST_ASSERT_EQUAL(100, cmd.domeSpeed);
}

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_body_command_size);
    RUN_TEST(test_body_telemetry_size);
    RUN_TEST(test_head_command_size);
    RUN_TEST(test_head_telemetry_size);
    RUN_TEST(test_body_command_field_roundtrip);
    RUN_TEST(test_head_command_field_roundtrip);
    RUN_TEST(test_status_constants);
    RUN_TEST(test_button_buffer_max_length);
    RUN_TEST(test_macro_array_capacity);
    RUN_TEST(test_drive_range_boundaries);
    return UNITY_END();
}
