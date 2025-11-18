#pragma once

#include <Arduino.h>

namespace Actuators {

/**
 * Status LED Controller for User Story 2 - System Health Indication
 * 
 * Provides visual indication of system operational status:
 * - Startup: 2x blinks (200ms on, 200ms off)
 * - Normal: Dimly lit (25% brightness) 
 * - Error: Rapid blinks (100ms on, 100ms off)
 * 
 * Constitutional Compliance:
 * - I. Reliability-First: Clear status indication for fault detection
 * - V. Observability: Visual system health monitoring
 */
class StatusLEDController {
public:
    enum class StatusPattern {
        OFF = 0,
        STARTUP,     // 2x blinks for initialization complete
        NORMAL,      // 25% brightness for healthy operation
        ERROR,       // Rapid blinks for system faults
        
        // WiFi Status Patterns (Feature: 002-secure-wifi-storage)
        WIFI_SETUP,      // Blue flashing - setup mode active
        WIFI_CONNECTING, // Blue solid - attempting connection
        WIFI_CONNECTED,  // Green solid - WiFi connected  
        WIFI_ERROR       // Red flashing - connection failed
    };

    /**
     * Constructor
     * @param pin GPIO pin for status LED
     * @param active_high true if LED is active high, false if active low
     */
    StatusLEDController(uint8_t pin, bool active_high = true);

    /**
     * Initialize status LED controller
     * @return true if initialization successful
     */
    bool begin();

    /**
     * Set status LED pattern
     * @param pattern Status pattern to display
     */
    void setPattern(StatusPattern pattern);

    /**
     * Update LED patterns (call from main loop)
     * Must be called regularly for pattern timing
     */
    void update();

    /**
     * Get current pattern
     * @return Current status pattern
     */
    StatusPattern getCurrentPattern() const { return current_pattern_; }

    /**
     * Force LED off (for emergency shutdown)
     */
    void forceOff();

private:
    uint8_t pin_;
    bool active_high_;
    StatusPattern current_pattern_;
    
    // Pattern timing state
    unsigned long pattern_start_time_;
    unsigned long last_toggle_time_;
    bool led_state_;
    uint8_t blink_count_;
    
    // PWM channel for brightness control
    uint8_t pwm_channel_;
    
    // Internal methods
    void setPhysicalState(bool on, uint8_t brightness = 255);
    void updateStartupPattern();
    void updateNormalPattern();
    void updateErrorPattern();
    
    // WiFi pattern methods (Feature: 002-secure-wifi-storage)
    void updateWiFiSetupPattern();     // Blue flashing for setup mode
    void updateWiFiConnectingPattern(); // Blue solid for connecting
    void updateWiFiConnectedPattern();  // Green solid for connected
    void updateWiFiErrorPattern();      // Red flashing for error
};

} // namespace Actuators