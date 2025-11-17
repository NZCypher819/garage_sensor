#include <unity.h>
#include "../mocks/mock_hardware.h"

/**
 * Basic Project Setup Tests
 * Constitutional Principle VI: Test-Driven Development
 * 
 * Validates fundamental project configuration and hardware abstractions
 */

void setUp(void) {
    #ifdef UNIT_TEST
    MockHardware::initialize();
    #endif
}

void tearDown(void) {
    #ifdef UNIT_TEST
    MockHardware::cleanup();
    #endif
}

void test_hardware_pin_definitions() {
    // Test that GPIO pins are properly defined and non-conflicting
    TEST_ASSERT_NOT_EQUAL(SENSOR_GPIO_PIN, PARKING_LED_GPIO);
    TEST_ASSERT_NOT_EQUAL(SENSOR_GPIO_PIN, STATUS_LED_GPIO);
    TEST_ASSERT_NOT_EQUAL(PARKING_LED_GPIO, STATUS_LED_GPIO);
    
    // Test that pins are within ESP32-S3-NANO range
    TEST_ASSERT_LESS_OR_EQUAL(21, SENSOR_GPIO_PIN);
    TEST_ASSERT_LESS_OR_EQUAL(21, PARKING_LED_GPIO);
    TEST_ASSERT_LESS_OR_EQUAL(21, STATUS_LED_GPIO);
}

void test_pwm_channel_definitions() {
    // Test that PWM channels are properly defined and non-conflicting
    TEST_ASSERT_NOT_EQUAL(PARKING_LED_PWM_CH, STATUS_LED_PWM_CH);
    
    // Test that PWM channels are within ESP32 range
    TEST_ASSERT_LESS_THAN(16, PARKING_LED_PWM_CH);
    TEST_ASSERT_LESS_THAN(16, STATUS_LED_PWM_CH);
}

void test_timing_requirements() {
    // Test constitutional timing requirements
    TEST_ASSERT_EQUAL(100, MAX_RESPONSE_TIME_MS);
    TEST_ASSERT_GREATER_THAN(0, TIMING_PRECISION_US);
    TEST_ASSERT_GREATER_THAN(0, POWER_SAVE_TIMEOUT_MS);
}

#ifdef UNIT_TEST
void test_mock_gpio_functionality() {
    // Test mock GPIO read/write operations
    MockGPIO::setMockValue(SENSOR_GPIO_PIN, 1);
    TEST_ASSERT_EQUAL(1, MockGPIO::getMockValue(SENSOR_GPIO_PIN));
    
    MockGPIO::setMockValue(SENSOR_GPIO_PIN, 0);
    TEST_ASSERT_EQUAL(0, MockGPIO::getMockValue(SENSOR_GPIO_PIN));
}

void test_mock_pwm_functionality() {
    // Test mock PWM operations
    MockPWM::ledcWrite(PARKING_LED_PWM_CH, 255);
    TEST_ASSERT_EQUAL(255, MockPWM::getMockDuty(PARKING_LED_PWM_CH));
    
    MockPWM::ledcWrite(STATUS_LED_PWM_CH, 128);
    TEST_ASSERT_EQUAL(128, MockPWM::getMockDuty(STATUS_LED_PWM_CH));
}

void test_mock_timing_functionality() {
    // Test mock timing operations
    MockTiming::setMockTime(1000);
    TEST_ASSERT_EQUAL(1000, MockTiming::millis());
    TEST_ASSERT_EQUAL(1000000, MockTiming::micros());
    
    MockTiming::advanceTime(500);
    TEST_ASSERT_EQUAL(1500, MockTiming::millis());
}
#endif

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_hardware_pin_definitions);
    RUN_TEST(test_pwm_channel_definitions);
    RUN_TEST(test_timing_requirements);
    
    #ifdef UNIT_TEST
    RUN_TEST(test_mock_gpio_functionality);
    RUN_TEST(test_mock_pwm_functionality);
    RUN_TEST(test_mock_timing_functionality);
    #endif
    
    return UNITY_END();
}