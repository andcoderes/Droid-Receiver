#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <string>

// --- Arduino types ---
typedef uint8_t byte;
typedef bool boolean;

// --- Pin modes & levels ---
#define HIGH 1
#define LOW  0
#define INPUT  0
#define OUTPUT 1
#define INPUT_PULLUP 2

// --- WiFi constants ---
#define WIFI_IF_STA 0
#define WIFI_STA    1

// --- Misc ---
#define IRAM_ATTR
#define SERIAL_8N1 0x800001c

// --- Timing ---
extern unsigned long mock_millis_value;
inline unsigned long millis() { return mock_millis_value; }
inline void delay(unsigned long) {}

// --- min / max / constrain ---
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#define constrain(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))

// --- GPIO stubs ---
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int  digitalRead(int) { return LOW; }
inline int  analogRead(int)  { return 0; }

// --- Interrupt stubs ---
inline void noInterrupts() {}
inline void interrupts() {}

// --- Cross-core critical section stubs (single-threaded in native tests) ---
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
inline void portENTER_CRITICAL(portMUX_TYPE*) {}
inline void portEXIT_CRITICAL(portMUX_TYPE*) {}

// --- HardwareSerial ---
class HardwareSerial {
public:
    void begin(unsigned long) {}

    // Write buffer for inspection
    static const int BUF_SIZE = 256;
    char writeBuf[BUF_SIZE];
    int  writePos = 0;

    int write(uint8_t c) {
        if (writePos < BUF_SIZE - 1) writeBuf[writePos++] = (char)c;
        return 1;
    }
    int write(const uint8_t* buf, int len) {
        for (int i = 0; i < len; i++) write(buf[i]);
        return len;
    }

    void print(const char* s) {
        while (*s) write((uint8_t)*s++);
    }
    void println(const char* s = "") {
        print(s);
        write((uint8_t)'\n');
    }
    void printf(const char* fmt, ...) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        print(buf);
    }

    void clearBuf() { writePos = 0; memset(writeBuf, 0, BUF_SIZE); }
};

// --- String class ---
class String {
public:
    String() {}
    String(const char* s) : data_(s ? s : "") {}
    String(const std::string& s) : data_(s) {}
    const char* c_str() const { return data_.c_str(); }
    int length() const { return (int)data_.size(); }
    bool operator==(const String& o) const { return data_ == o.data_; }
    bool operator==(const char* s) const { return data_ == s; }
    String operator+(const String& o) const { return String(data_ + o.data_); }
    String& operator+=(const String& o) { data_ += o.data_; return *this; }
private:
    std::string data_;
};

// --- Global instances ---
extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;
