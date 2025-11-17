#include <unity.h>
#include "../mocks/mock_hardware.h"

/**
 * Hardware Configuration Tests
 * Constitutional Principle VI: Test-Driven Development
 * 
 * Tests configuration validation functions and error handling
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

void test_validate_pin_configuration() {
    // Test that validatePinConfiguration returns true for valid pins
    TEST_ASSERT_TRUE(validatePinConfiguration());
    
    // Note: In a full implementation, we would test invalid pin scenarios
    // but this requires more sophisticated mock capabilities
}

void test_pwm_configuration_validation() {
    // Test PWM frequency and resolution validation
    TEST_ASSERT_GREATER_THAN(0, PWM_FREQUENCY_HZ);
    TEST_ASSERT_LESS_OR_EQUAL(20000, PWM_FREQUENCY_HZ); // Human hearing range
    
    TEST_ASSERT_GREATER_OR_EQUAL(8, PWM_RESOLUTION_BITS);
    TEST_ASSERT_LESS_OR_EQUAL(16, PWM_RESOLUTION_BITS);
}

void test_timing_configuration() {
    // Test timing configuration constants
    TEST_ASSERT_GREATER_THAN(0, SENSOR_READ_INTERVAL_MS);
    TEST_ASSERT_LESS_THAN(1000, SENSOR_READ_INTERVAL_MS); // Max 1 second
    
    TEST_ASSERT_GREATER_THAN(0, LED_FADE_DURATION_MS);
    TEST_ASSERT_LESS_THAN(5000, LED_FADE_DURATION_MS); // Max 5 seconds
}

void test_power_management_configuration() {
    // Test power management settings
    TEST_ASSERT_GREATER_THAN(0, POWER_SAVE_TIMEOUT_MS);
    TEST_ASSERT_GREATER_THAN(0, SLEEP_MODE_THRESHOLD_MS);
    
    // Power save timeout should be reasonable (not too short or long)
    TEST_ASSERT_GREATER_THAN(10000, POWER_SAVE_TIMEOUT_MS); // At least 10 seconds
    TEST_ASSERT_LESS_THAN(3600000, POWER_SAVE_TIMEOUT_MS); // At most 1 hour
}

#ifdef UNIT_TEST
void test_mock_power_states() {
    // Test mock power state transitions
    MockHardware::setPowerState(POWER_STATE_ACTIVE);
    TEST_ASSERT_EQUAL(POWER_STATE_ACTIVE, MockHardware::getPowerState());
    
    MockHardware::setPowerState(POWER_STATE_SLEEP);
    TEST_ASSERT_EQUAL(POWER_STATE_SLEEP, MockHardware::getPowerState());
}

void test_error_simulation() {
    // Test error condition simulation
    MockHardware::simulateError(ERROR_GPIO_FAILURE);
    TEST_ASSERT_TRUE(MockHardware::hasError());
    TEST_ASSERT_EQUAL(ERROR_GPIO_FAILURE, MockHardware::getLastError());
    
    MockHardware::clearErrors();
    TEST_ASSERT_FALSE(MockHardware::hasError());
}
#endif

int main() {
    UNITY_BEGIN();
    
    RUN_TEST(test_validate_pin_configuration);
    RUN_TEST(test_pwm_configuration_validation);
    RUN_TEST(test_timing_configuration);
    RUN_TEST(test_power_management_configuration);
    
    #ifdef UNIT_TEST
    RUN_TEST(test_mock_power_states);
    RUN_TEST(test_error_simulation);
    #endif
    
    return UNITY_END();
}