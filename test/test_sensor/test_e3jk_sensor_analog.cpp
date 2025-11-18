/**
 * Unit Tests for E3JK-RR11 Analog Detection Algorithm
 * 
 * Tests the analog ADC detection implementation including:
 * - Multi-sample averaging (5 samples @ 100μs intervals)
 * - Voltage threshold detection (configurable, default >0V)
 * - ADC-to-voltage conversion (12-bit, 0-3.3V)
 * - Noise filtering and voltage oscillation handling
 * 
 * Constitutional Principle VI: 80% minimum test coverage requirement
 * 
 * NOTE: These tests verify the algorithm logic used in the sensor driver.
 * The actual E3JK sensor implementation is validated through integration tests
 * due to tight coupling with Arduino hardware functions (analogRead, digitalRead, etc.)
 */

#include <unity.h>

/**
 * Test helper: Simulate multi-sample ADC averaging algorithm
 * This replicates the logic from e3jk_sensor.cpp lines 30-42
 */
int simulate_averaged_adc_reading(const int samples[], int sample_count) {
    int adc_sum = 0;
    for (int i = 0; i < sample_count; i++) {
        adc_sum += samples[i];
    }
    return adc_sum / sample_count;
}

/**
 * Test helper: Simulate voltage conversion
 * This replicates the logic from e3jk_sensor.cpp line 44
 */
float simulate_adc_to_voltage(int adc_value) {
    return (adc_value / 4095.0f) * 3.3f;
}

/**
 * Test helper: Simulate threshold detection
 * This replicates the logic from e3jk_sensor.cpp line 48
 */
bool simulate_beam_blocked_detection(int adc_value) {
    return (adc_value > 0);
}

void setUp(void) {
    // Setup before each test
}

void tearDown(void) {
    // Cleanup after each test
}

// ===== MULTI-SAMPLE AVERAGING TESTS =====

void test_averaging_all_zeros(void) {
    // Test case: All ADC readings are zero (beam clear)
    int samples[5] = {0, 0, 0, 0, 0};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    TEST_ASSERT_EQUAL(0, averaged);
    TEST_ASSERT_FALSE(simulate_beam_blocked_detection(averaged));
}

void test_averaging_all_high_values(void) {
    // Test case: All ADC readings are high (beam blocked)
    int samples[5] = {3000, 2950, 3100, 3050, 2900};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    TEST_ASSERT_EQUAL(3000, averaged);
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(averaged));
}

void test_averaging_noisy_signal(void) {
    // Test case: Noisy signal with spikes
    // Average = (0 + 100 + 0 + 50 + 0) / 5 = 30
    int samples[5] = {0, 100, 0, 50, 0};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    TEST_ASSERT_EQUAL(30, averaged);
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(averaged)); // 30 > 0
}

void test_averaging_filters_single_spike(void) {
    // Test case: Single spike in otherwise zero signal
    // Average = (0 + 0 + 0 + 0 + 1) / 5 = 0 (integer division)
    int samples[5] = {0, 0, 0, 0, 1};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    TEST_ASSERT_EQUAL(0, averaged);
    TEST_ASSERT_FALSE(simulate_beam_blocked_detection(averaged));
}

void test_averaging_high_frequency_noise(void) {
    // Test case: Alternating high/low (relay bounce simulation)
    // Average = (4095 + 0 + 4095 + 0 + 4095) / 5 = 2457
    int samples[5] = {4095, 0, 4095, 0, 4095};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    TEST_ASSERT_EQUAL(2457, averaged);
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(averaged));
}

void test_averaging_gradual_transition(void) {
    // Test case: Gradual voltage rise (slow relay closure)
    // Average = (0 + 500 + 1000 + 1500 + 2000) / 5 = 1000
    int samples[5] = {0, 500, 1000, 1500, 2000};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    TEST_ASSERT_EQUAL(1000, averaged);
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(averaged));
}

// ===== VOLTAGE THRESHOLD DETECTION TESTS =====

void test_threshold_exactly_zero(void) {
    // Test case: Threshold boundary - exactly 0V
    int adc_value = 0;
    TEST_ASSERT_FALSE(simulate_beam_blocked_detection(adc_value));
}

void test_threshold_one_adc_unit(void) {
    // Test case: Threshold boundary - 1 ADC unit above zero
    int adc_value = 1;
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(adc_value));
}

void test_threshold_very_low_voltage(void) {
    // Test case: Very low voltage (~0.05V = 62 ADC units)
    int adc_value = 62;
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(adc_value));
    
    float voltage = simulate_adc_to_voltage(adc_value);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.05f, voltage);
}

void test_threshold_typical_blocked_voltage(void) {
    // Test case: Typical blocked state voltage (2.5V)
    int adc_value = (int)((2.5f / 3.3f) * 4095.0f); // ≈ 3102
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(adc_value));
    
    float voltage = simulate_adc_to_voltage(adc_value);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 2.5f, voltage);
}

void test_threshold_maximum_voltage(void) {
    // Test case: Maximum ADC value (3.3V saturated)
    int adc_value = 4095;
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(adc_value));
    
    float voltage = simulate_adc_to_voltage(adc_value);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.3f, voltage);
}

// ===== ADC TO VOLTAGE CONVERSION TESTS =====

void test_voltage_conversion_zero(void) {
    // Test case: 0 ADC units = 0.0V
    float voltage = simulate_adc_to_voltage(0);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, voltage);
}

void test_voltage_conversion_midpoint(void) {
    // Test case: 2047 ADC units ≈ 1.65V (half of 3.3V)
    float voltage = simulate_adc_to_voltage(2047);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.65f, voltage);
}

void test_voltage_conversion_maximum(void) {
    // Test case: 4095 ADC units = 3.3V
    float voltage = simulate_adc_to_voltage(4095);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.3f, voltage);
}

void test_voltage_conversion_quarter_scale(void) {
    // Test case: ~1024 ADC units ≈ 0.825V (quarter scale)
    float voltage = simulate_adc_to_voltage(1024);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.825f, voltage);
}

void test_voltage_conversion_three_quarter_scale(void) {
    // Test case: ~3072 ADC units ≈ 2.475V (three-quarter scale)
    float voltage = simulate_adc_to_voltage(3072);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.475f, voltage);
}

// ===== SENSOR BEHAVIOR SIMULATION TESTS =====

void test_clear_beam_scenario(void) {
    // Test case: Simulate clear beam (relay open, 0V on all samples)
    int samples[5] = {0, 0, 0, 0, 0};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    float voltage = simulate_adc_to_voltage(averaged);
    bool blocked = simulate_beam_blocked_detection(averaged);
    
    TEST_ASSERT_EQUAL(0, averaged);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, voltage);
    TEST_ASSERT_FALSE(blocked);
}

void test_blocked_beam_scenario(void) {
    // Test case: Simulate blocked beam (relay closed, 2.5V typical)
    int typical_blocked_adc = 3102; // ~2.5V
    int samples[5] = {typical_blocked_adc, typical_blocked_adc, typical_blocked_adc, typical_blocked_adc, typical_blocked_adc};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    float voltage = simulate_adc_to_voltage(averaged);
    bool blocked = simulate_beam_blocked_detection(averaged);
    
    TEST_ASSERT_EQUAL(typical_blocked_adc, averaged);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 2.5f, voltage);
    TEST_ASSERT_TRUE(blocked);
}

void test_relay_bounce_scenario(void) {
    // Test case: Simulate relay bounce during transition
    // Relay bounces between 0V and 3V rapidly
    int samples[5] = {0, 3724, 0, 3724, 3724}; // 0V, 3V, 0V, 3V, 3V
    int averaged = simulate_averaged_adc_reading(samples, 5);
    bool blocked = simulate_beam_blocked_detection(averaged);
    
    // Average should smooth out the bounce
    TEST_ASSERT_GREATER_THAN(0, averaged);
    TEST_ASSERT_TRUE(blocked); // Averaging should detect as blocked
}

void test_variable_sensor_voltage_scenario(void) {
    // Test case: Different sensors output different voltages (2V, 2.5V, 3V all valid)
    int samples_2v[5] = {2483, 2483, 2483, 2483, 2483}; // 2.0V
    int samples_25v[5] = {3102, 3102, 3102, 3102, 3102}; // 2.5V
    int samples_3v[5] = {3724, 3724, 3724, 3724, 3724}; // 3.0V
    
    int avg_2v = simulate_averaged_adc_reading(samples_2v, 5);
    int avg_25v = simulate_averaged_adc_reading(samples_25v, 5);
    int avg_3v = simulate_averaged_adc_reading(samples_3v, 5);
    
    // All should be detected as blocked
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(avg_2v));
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(avg_25v));
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(avg_3v));
}

// ===== EDGE CASE TESTS =====

void test_all_samples_one_adc_unit(void) {
    // Test case: All samples are exactly 1 ADC unit (edge of threshold)
    int samples[5] = {1, 1, 1, 1, 1};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    TEST_ASSERT_EQUAL(1, averaged);
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(averaged));
}

void test_mixed_zero_and_one(void) {
    // Test case: Mix of 0 and 1 ADC units
    int samples[5] = {0, 1, 0, 1, 0};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    TEST_ASSERT_EQUAL(0, averaged); // (0+1+0+1+0)/5 = 2/5 = 0 (integer)
    TEST_ASSERT_FALSE(simulate_beam_blocked_detection(averaged));
}

void test_saturation_handling(void) {
    // Test case: All samples at maximum (4095)
    int samples[5] = {4095, 4095, 4095, 4095, 4095};
    int averaged = simulate_averaged_adc_reading(samples, 5);
    TEST_ASSERT_EQUAL(4095, averaged);
    TEST_ASSERT_TRUE(simulate_beam_blocked_detection(averaged));
    
    float voltage = simulate_adc_to_voltage(averaged);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.3f, voltage);
}

#ifdef UNIT_TEST
int main(int argc, char** argv) {
    UNITY_BEGIN();
    
    // Multi-sample averaging tests
    RUN_TEST(test_averaging_all_zeros);
    RUN_TEST(test_averaging_all_high_values);
    RUN_TEST(test_averaging_noisy_signal);
    RUN_TEST(test_averaging_filters_single_spike);
    RUN_TEST(test_averaging_high_frequency_noise);
    RUN_TEST(test_averaging_gradual_transition);
    
    // Voltage threshold detection tests
    RUN_TEST(test_threshold_exactly_zero);
    RUN_TEST(test_threshold_one_adc_unit);
    RUN_TEST(test_threshold_very_low_voltage);
    RUN_TEST(test_threshold_typical_blocked_voltage);
    RUN_TEST(test_threshold_maximum_voltage);
    
    // ADC to voltage conversion tests
    RUN_TEST(test_voltage_conversion_zero);
    RUN_TEST(test_voltage_conversion_midpoint);
    RUN_TEST(test_voltage_conversion_maximum);
    RUN_TEST(test_voltage_conversion_quarter_scale);
    RUN_TEST(test_voltage_conversion_three_quarter_scale);
    
    // Sensor behavior simulation tests
    RUN_TEST(test_clear_beam_scenario);
    RUN_TEST(test_blocked_beam_scenario);
    RUN_TEST(test_relay_bounce_scenario);
    RUN_TEST(test_variable_sensor_voltage_scenario);
    
    // Edge case tests
    RUN_TEST(test_all_samples_one_adc_unit);
    RUN_TEST(test_mixed_zero_and_one);
    RUN_TEST(test_saturation_handling);
    
    return UNITY_END();
}
#else
// For embedded testing (Arduino framework)
void setup() {
    UNITY_BEGIN();
    
    // Multi-sample averaging tests
    RUN_TEST(test_averaging_all_zeros);
    RUN_TEST(test_averaging_all_high_values);
    RUN_TEST(test_averaging_noisy_signal);
    RUN_TEST(test_averaging_filters_single_spike);
    RUN_TEST(test_averaging_high_frequency_noise);
    RUN_TEST(test_averaging_gradual_transition);
    
    // Voltage threshold detection tests
    RUN_TEST(test_threshold_exactly_zero);
    RUN_TEST(test_threshold_one_adc_unit);
    RUN_TEST(test_threshold_very_low_voltage);
    RUN_TEST(test_threshold_typical_blocked_voltage);
    RUN_TEST(test_threshold_maximum_voltage);
    
    // ADC to voltage conversion tests
    RUN_TEST(test_voltage_conversion_zero);
    RUN_TEST(test_voltage_conversion_midpoint);
    RUN_TEST(test_voltage_conversion_maximum);
    RUN_TEST(test_voltage_conversion_quarter_scale);
    RUN_TEST(test_voltage_conversion_three_quarter_scale);
    
    // Sensor behavior simulation tests
    RUN_TEST(test_clear_beam_scenario);
    RUN_TEST(test_blocked_beam_scenario);
    RUN_TEST(test_relay_bounce_scenario);
    RUN_TEST(test_variable_sensor_voltage_scenario);
    
    // Edge case tests
    RUN_TEST(test_all_samples_one_adc_unit);
    RUN_TEST(test_mixed_zero_and_one);
    RUN_TEST(test_saturation_handling);
    
    UNITY_END();
}

void loop() {
    // Tests run once in setup()
}
#endif
