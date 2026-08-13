#include <unity.h>
#include <cstring>
#include "communication/CommandParser.h"

static CommandParser parser;

void setUp(void) {
    // Fresh parser for each test
    parser = CommandParser();
}

void tearDown(void) {}

// ---------- Movement ----------

void test_movement_basic() {
    const char* json = "{\"s\":1,\"l\":[50,-30],\"r\":[20,0],\"lr\":[0,0]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_TRUE(r.sendBody);
    TEST_ASSERT_FALSE(r.sendHead);
    TEST_ASSERT_EQUAL(3, r.bodyCmd.msgType);
    TEST_ASSERT_EQUAL(STATUS_MOVEMENT, r.bodyCmd.status);
    TEST_ASSERT_EQUAL(50, r.bodyCmd.lx);
    TEST_ASSERT_EQUAL(-30, r.bodyCmd.ly);
    TEST_ASSERT_EQUAL(20, r.bodyCmd.domeSpeed);
}

void test_movement_clamps_out_of_range_values() {
    const char* json = "{\"s\":1,\"l\":[200,-300],\"r\":[500,0]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_TRUE(r.sendBody);
    TEST_ASSERT_EQUAL(100, r.bodyCmd.lx);
    TEST_ASSERT_EQUAL(-100, r.bodyCmd.ly);
    TEST_ASSERT_EQUAL(100, r.bodyCmd.domeSpeed);
}

void test_movement_zeros() {
    const char* json = "{\"s\":1,\"l\":[0,0],\"r\":[0,0]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_TRUE(r.sendBody);
    TEST_ASSERT_EQUAL(0, r.bodyCmd.lx);
    TEST_ASSERT_EQUAL(0, r.bodyCmd.ly);
    TEST_ASSERT_EQUAL(0, r.bodyCmd.domeSpeed);
}

// ---------- Buttons ----------

void test_button_press() {
    const char* json = "{\"s\":0,\"p\":[\"y\"],\"m\":[]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_TRUE(r.sendBody);
    TEST_ASSERT_TRUE(r.sendHead);
    TEST_ASSERT_EQUAL_STRING("y", r.bodyCmd.button);
    TEST_ASSERT_EQUAL_STRING("y", r.headCmd.button);
}

void test_multiple_buttons_takes_first_alpha() {
    const char* json = "{\"s\":0,\"p\":[\"y\",\"x\"],\"m\":[]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    // Last alpha string wins (strncpy overwrites)
    TEST_ASSERT_EQUAL_STRING("x", r.bodyCmd.button);
    TEST_ASSERT_EQUAL(0, r.bodyCmd.macro[0]);  // no numerics
}

void test_numeric_macro_in_p_array() {
    const char* json = "{\"s\":0,\"p\":[\"100\"],\"m\":[]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_EQUAL(100, r.bodyCmd.macro[0]);
    TEST_ASSERT_EQUAL(100, r.headCmd.macro[0]);
    TEST_ASSERT_EQUAL_STRING("", r.bodyCmd.button);
}

void test_mixed_p_array() {
    const char* json = "{\"s\":0,\"p\":[\"du\",\"103\"],\"m\":[]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_EQUAL_STRING("du", r.bodyCmd.button);
    TEST_ASSERT_EQUAL(103, r.bodyCmd.macro[0]);
    TEST_ASSERT_EQUAL(103, r.headCmd.macro[0]);
}

void test_m_array_macros() {
    const char* json = "{\"s\":0,\"p\":[],\"m\":[101,103]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_EQUAL(101, r.bodyCmd.macro[0]);
    TEST_ASSERT_EQUAL(103, r.bodyCmd.macro[1]);
    TEST_ASSERT_EQUAL(101, r.headCmd.macro[0]);
    TEST_ASSERT_EQUAL(103, r.headCmd.macro[1]);
}

// ---------- Audio ----------

void test_audio_track() {
    const char* json = "{\"s\":3,\"p\":[],\"m\":[5]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_TRUE(r.sendBody);
    TEST_ASSERT_FALSE(r.sendHead);
    TEST_ASSERT_EQUAL(5, r.bodyCmd.audioTrack);
}

// ---------- Settings ----------

void test_settings_volume_max() {
    const char* json = "{\"s\":2,\"v\":255,\"i\":true}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_TRUE(r.sendBody);
    TEST_ASSERT_TRUE(r.sendHead);
    TEST_ASSERT_EQUAL(30, r.bodyCmd.volume);
    TEST_ASSERT_EQUAL(30, r.headCmd.volume);
    TEST_ASSERT_EQUAL(1, r.headCmd.idleMode);
}

void test_settings_volume_zero() {
    const char* json = "{\"s\":2,\"v\":0}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_EQUAL(0, r.bodyCmd.volume);
    TEST_ASSERT_EQUAL(0, r.headCmd.volume);
}

void test_settings_volume_mid() {
    const char* json = "{\"s\":2,\"v\":128}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    // 128 * 30 / 255 = 15.06 -> 15
    TEST_ASSERT_EQUAL(15, r.bodyCmd.volume);
    TEST_ASSERT_EQUAL(15, r.headCmd.volume);
}

// ---------- Bubble toggle ----------

void test_bubble_toggle_on() {
    const char* json = "{\"s\":0,\"p\":[\"l2\"],\"m\":[]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_EQUAL(1, r.bodyCmd.bubbles);
    TEST_ASSERT_TRUE(parser.getBubbles());
}

void test_bubble_toggle_off() {
    // First toggle on
    const char* json1 = "{\"s\":0,\"p\":[\"l2\"],\"m\":[]}";
    parser.parseMessage(json1, strlen(json1), false, false);
    TEST_ASSERT_TRUE(parser.getBubbles());

    // Second toggle off
    const char* json2 = "{\"s\":0,\"p\":[\"l2\"],\"m\":[]}";
    ParseResult r = parser.parseMessage(json2, strlen(json2), false, false);
    TEST_ASSERT_EQUAL(0, r.bodyCmd.bubbles);
    TEST_ASSERT_FALSE(parser.getBubbles());
}

void test_bubble_state_persists_in_movement() {
    // Turn bubbles on
    const char* json1 = "{\"s\":0,\"p\":[\"l2\"],\"m\":[]}";
    parser.parseMessage(json1, strlen(json1), false, false);
    TEST_ASSERT_TRUE(parser.getBubbles());

    // Movement command should carry bubble state
    const char* json2 = "{\"s\":1,\"l\":[10,20],\"r\":[0,0]}";
    ParseResult r = parser.parseMessage(json2, strlen(json2), false, false);
    TEST_ASSERT_EQUAL(1, r.bodyCmd.bubbles);
}

// ---------- Connection ----------

void test_connection_connected() {
    ParseResult r = parser.setConnected(true);

    TEST_ASSERT_TRUE(r.sendBody);
    TEST_ASSERT_TRUE(r.sendHead);
    TEST_ASSERT_EQUAL(1, r.bodyCmd.connectionStatus);
    TEST_ASSERT_EQUAL(1, r.headCmd.connectionStatus);
    TEST_ASSERT_EQUAL(STATUS_CONNECTION, r.bodyCmd.status);
    TEST_ASSERT_EQUAL(STATUS_CONNECTION, r.headCmd.status);
}

void test_connection_disconnected_resets_bubbles() {
    // Turn bubbles on
    const char* json = "{\"s\":0,\"p\":[\"l2\"],\"m\":[]}";
    parser.parseMessage(json, strlen(json), false, false);
    TEST_ASSERT_TRUE(parser.getBubbles());

    // Disconnect resets bubbles
    ParseResult r = parser.setConnected(false);
    TEST_ASSERT_FALSE(parser.getBubbles());
    TEST_ASSERT_EQUAL(0, r.bodyCmd.connectionStatus);
    TEST_ASSERT_EQUAL(0, r.headCmd.connectionStatus);
}

// ---------- Invalid JSON ----------

void test_invalid_json() {
    const char* json = "not json at all{{{";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_FALSE(r.sendBody);
    TEST_ASSERT_FALSE(r.sendHead);
    TEST_ASSERT_FALSE(r.isPing);
}

void test_missing_status_key_is_rejected() {
    const char* json = "{\"p\":[\"y\"]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_FALSE(r.sendBody);
    TEST_ASSERT_FALSE(r.sendHead);
    TEST_ASSERT_FALSE(r.isPing);
}

void test_non_numeric_status_key_is_rejected() {
    const char* json = "{\"s\":\"buttons\",\"p\":[\"y\"]}";
    ParseResult r = parser.parseMessage(json, strlen(json), false, false);

    TEST_ASSERT_FALSE(r.sendBody);
    TEST_ASSERT_FALSE(r.sendHead);
    TEST_ASSERT_FALSE(r.isPing);
}

// ---------- Ping ----------

void test_ping() {
    const char* json = "{\"s\":4}";
    ParseResult r = parser.parseMessage(json, strlen(json), true, false);

    TEST_ASSERT_TRUE(r.isPing);
    TEST_ASSERT_FALSE(r.sendBody);
    TEST_ASSERT_FALSE(r.sendHead);
    TEST_ASSERT_EQUAL_STRING("{\"s\":4,\"body\":1,\"head\":0}", r.pingResponse);
}

// ---------- Volume persistence ----------

void test_volume_persists_in_movement() {
    // Set volume via settings
    const char* json1 = "{\"s\":2,\"v\":255}";
    parser.parseMessage(json1, strlen(json1), false, false);

    // Movement should carry the persisted volume
    const char* json2 = "{\"s\":1,\"l\":[10,0],\"r\":[0,0]}";
    ParseResult r = parser.parseMessage(json2, strlen(json2), false, false);
    TEST_ASSERT_EQUAL(30, r.bodyCmd.volume);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_movement_basic);
    RUN_TEST(test_movement_clamps_out_of_range_values);
    RUN_TEST(test_movement_zeros);
    RUN_TEST(test_button_press);
    RUN_TEST(test_multiple_buttons_takes_first_alpha);
    RUN_TEST(test_numeric_macro_in_p_array);
    RUN_TEST(test_mixed_p_array);
    RUN_TEST(test_m_array_macros);
    RUN_TEST(test_audio_track);
    RUN_TEST(test_settings_volume_max);
    RUN_TEST(test_settings_volume_zero);
    RUN_TEST(test_settings_volume_mid);
    RUN_TEST(test_bubble_toggle_on);
    RUN_TEST(test_bubble_toggle_off);
    RUN_TEST(test_bubble_state_persists_in_movement);
    RUN_TEST(test_connection_connected);
    RUN_TEST(test_connection_disconnected_resets_bubbles);
    RUN_TEST(test_invalid_json);
    RUN_TEST(test_missing_status_key_is_rejected);
    RUN_TEST(test_non_numeric_status_key_is_rejected);
    RUN_TEST(test_ping);
    RUN_TEST(test_volume_persists_in_movement);
    return UNITY_END();
}
