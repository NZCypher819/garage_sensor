#pragma once

/**
 * Hardware Abstraction Layer for Testing
 * Constitutional Principle VI: Test-Driven Development
 * 
 * Provides mock interfaces for unit testing ESP32 hardware peripherals
 */

#ifdef UNIT_TEST

#include <stdint.h>
#include <stdbool.h>

// Mock GPIO interface
class MockGPIO {
public:
    static void pinMode(uint8_t pin, uint8_t mode);
    static int digitalRead(uint8_t pin);
    static void digitalWrite(uint8_t pin, uint8_t value);
    static void setMockValue(uint8_t pin, int value);
    static int getMockValue(uint8_t pin);
    static void reset();
    
private:
    static int mock_values[22]; // ESP32-S3-NANO has pins 0-21
    static bool initialized;
};

// Mock PWM interface  
class MockPWM {
public:
    static void ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution);
    static void ledcAttachPin(uint8_t pin, uint8_t channel);
    static void ledcWrite(uint8_t channel, uint32_t duty);
    static uint32_t getMockDuty(uint8_t channel);
    static void reset();
    
private:
    static uint32_t mock_duty[16]; // ESP32 has 16 PWM channels
    static bool channel_attached[16];
};

// Mock timing interface
class MockTiming {
public:
    static unsigned long millis();
    static unsigned long micros();
    static void delay(unsigned long ms);
    static void delayMicroseconds(unsigned int us);
    static void setMockTime(unsigned long time_ms);
    static void advanceTime(unsigned long delta_ms);
    static void reset();
    
private:
    static unsigned long mock_time;
};

// Global mock state management
class MockHardware {
public:
    static void initialize();
    static void cleanup();
    static void reset_all();
};

// Replace Arduino functions with mocks during testing
#define pinMode MockGPIO::pinMode
#define digitalRead MockGPIO::digitalRead  
#define digitalWrite MockGPIO::digitalWrite
#define ledcSetup MockPWM::ledcSetup
#define ledcAttachPin MockPWM::ledcAttachPin
#define ledcWrite MockPWM::ledcWrite
#define millis MockTiming::millis
#define micros MockTiming::micros
#define delay MockTiming::delay
#define delayMicroseconds MockTiming::delayMicroseconds

#endif // UNIT_TEST