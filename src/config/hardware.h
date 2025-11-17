#pragma once

/**
 * Hardware Pin Definitions for ESP32-S3-NANO
 * Garage Parking Position Sensor
 * 
 * E3JK-RR11 Sensor: 24V powered, NO/NC relay outputs
 * Constitutional Compliance: Hardware Standards IP65 rating, -20°C to 60°C operation
 */

// GPIO Pin Assignments
#define SENSOR_GPIO_PIN      2    // E3JK-RR11 NO (Normally Open) contact input
#define PARKING_LED_GPIO     8    // Parking indication LED output 
#define STATUS_LED_GPIO      9    // System status LED output

// PWM Channel Assignments
#define PARKING_LED_PWM_CH   0    // PWM channel for parking LED brightness control
#define STATUS_LED_PWM_CH    1    // PWM channel for status LED brightness control

// PWM Configuration
#define LED_PWM_FREQUENCY    5000 // 5kHz PWM frequency
#define LED_PWM_RESOLUTION   8    // 8-bit resolution (0-255)

// Sensor Configuration
#define SENSOR_DEBOUNCE_MS   50   // Hardware debouncing delay
#define SENSOR_ACTIVE_LOW    false // E3JK-RR11 NO contact: closed (LOW) when beam broken

// Timing Requirements (Constitutional: <100ms response)
#define MAX_RESPONSE_TIME_MS 100  // Maximum allowed response time
#define TIMING_PRECISION_US  1000 // Microsecond precision for timing measurements

// Power Management
#define POWER_SAVE_TIMEOUT_MS 30000 // 30 seconds idle before sleep mode
#define WAKE_UP_GPIO         SENSOR_GPIO_PIN // GPIO for wake-up from sleep

// Hardware Validation
#if SENSOR_GPIO_PIN == PARKING_LED_GPIO || SENSOR_GPIO_PIN == STATUS_LED_GPIO
#error "GPIO pin conflict detected - sensor and LED pins must be different"
#endif

#if PARKING_LED_GPIO == STATUS_LED_GPIO
#error "GPIO pin conflict detected - parking and status LEDs must use different pins"
#endif

// ESP32-S3-NANO specific validations
#if SENSOR_GPIO_PIN > 21
#error "Invalid sensor GPIO pin - ESP32-S3-NANO GPIO limit exceeded"
#endif

#if PARKING_LED_GPIO > 21 || STATUS_LED_GPIO > 21
#error "Invalid LED GPIO pin - ESP32-S3-NANO GPIO limit exceeded"
#endif