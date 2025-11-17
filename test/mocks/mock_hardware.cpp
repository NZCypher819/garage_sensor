#include "mock_hardware.h"

#ifdef UNIT_TEST

#include <cstring>

// MockGPIO implementation
int MockGPIO::mock_values[22] = {0};
bool MockGPIO::initialized = false;

void MockGPIO::pinMode(uint8_t pin, uint8_t mode) {
    if (pin < 22) {
        // Pin mode set - no action needed in mock
    }
}

int MockGPIO::digitalRead(uint8_t pin) {
    if (pin < 22) {
        return mock_values[pin];
    }
    return 0;
}

void MockGPIO::digitalWrite(uint8_t pin, uint8_t value) {
    if (pin < 22) {
        mock_values[pin] = value;
    }
}

void MockGPIO::setMockValue(uint8_t pin, int value) {
    if (pin < 22) {
        mock_values[pin] = value;
    }
}

int MockGPIO::getMockValue(uint8_t pin) {
    if (pin < 22) {
        return mock_values[pin];
    }
    return 0;
}

void MockGPIO::reset() {
    memset(mock_values, 0, sizeof(mock_values));
}

// MockPWM implementation
uint32_t MockPWM::mock_duty[16] = {0};
bool MockPWM::channel_attached[16] = {false};

void MockPWM::ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution) {
    if (channel < 16) {
        // PWM setup - no action needed in mock
    }
}

void MockPWM::ledcAttachPin(uint8_t pin, uint8_t channel) {
    if (channel < 16) {
        channel_attached[channel] = true;
    }
}

void MockPWM::ledcWrite(uint8_t channel, uint32_t duty) {
    if (channel < 16) {
        mock_duty[channel] = duty;
    }
}

uint32_t MockPWM::getMockDuty(uint8_t channel) {
    if (channel < 16) {
        return mock_duty[channel];
    }
    return 0;
}

void MockPWM::reset() {
    memset(mock_duty, 0, sizeof(mock_duty));
    memset(channel_attached, false, sizeof(channel_attached));
}

// MockTiming implementation
unsigned long MockTiming::mock_time = 0;

unsigned long MockTiming::millis() {
    return mock_time;
}

unsigned long MockTiming::micros() {
    return mock_time * 1000;
}

void MockTiming::delay(unsigned long ms) {
    mock_time += ms;
}

void MockTiming::delayMicroseconds(unsigned int us) {
    // For mock, treat as milliseconds for simplicity
    mock_time += (us / 1000);
}

void MockTiming::setMockTime(unsigned long time_ms) {
    mock_time = time_ms;
}

void MockTiming::advanceTime(unsigned long delta_ms) {
    mock_time += delta_ms;
}

void MockTiming::reset() {
    mock_time = 0;
}

// MockHardware implementation
void MockHardware::initialize() {
    MockGPIO::reset();
    MockPWM::reset();
    MockTiming::reset();
}

void MockHardware::cleanup() {
    // Nothing to cleanup for mocks
}

void MockHardware::reset_all() {
    MockGPIO::reset();
    MockPWM::reset();
    MockTiming::reset();
}

#endif // UNIT_TEST