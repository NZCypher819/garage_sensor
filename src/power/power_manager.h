#pragma once

#include <Arduino.h>
#include <esp_sleep.h>

namespace Power {

/**
 * Power Manager for User Story 3 - Power Management
 * 
 * Implements intelligent power management with:
 * - <10mA idle consumption via ESP32 light sleep
 * - <100ms response time when waking from sleep
 * - GPIO wake-up from sensor interrupts
 * - 30 second timeout before sleep entry
 * 
 * Constitutional Compliance:
 * - II. Power-Aware Design: Aggressive power optimization for long-term deployment
 * - I. Reliability-First: Maintains sensor responsiveness during power management
 */
class PowerManager {
public:
    enum class PowerState {
        ACTIVE,       // Normal operation, all systems active
        IDLE,         // Waiting for activity, preparing for sleep
        LIGHT_SLEEP,  // ESP32 light sleep, GPIO wake-up enabled
        ERROR         // Power management fault state
    };

    /**
     * Constructor
     * @param wake_gpio GPIO pin that can wake system from sleep (sensor pin)
     * @param idle_timeout_ms Milliseconds of inactivity before sleep (default 30s)
     */
    PowerManager(uint8_t wake_gpio, unsigned long idle_timeout_ms = 30000);

    /**
     * Initialize power management
     * @return true if initialization successful
     */
    bool begin();

    /**
     * Update power management (call from main loop)
     * Handles state transitions and sleep management
     */
    void update();

    /**
     * Report activity to prevent sleep
     * Call when sensor detects vehicle or system activity occurs
     */
    void reportActivity();

    /**
     * Get current power state
     * @return Current power management state
     */
    PowerState getCurrentState() const { return current_state_; }

    /**
     * Check if system is in low power mode
     * @return true if system is sleeping or about to sleep
     */
    bool isInLowPowerMode() const;

    /**
     * Force system to stay awake (disable power management)
     * @param stay_awake true to disable sleep, false to re-enable
     */
    void forceStayAwake(bool stay_awake);

    /**
     * Get current consumption estimate in milliamps
     * @return Estimated current consumption in mA
     */
    float getCurrentConsumptionMA() const;

    /**
     * Get power statistics
     */
    struct PowerStats {
        unsigned long total_sleep_time_ms;
        unsigned long wake_count;
        unsigned long last_wake_reason;
        float average_consumption_ma;
    };
    
    PowerStats getPowerStats() const { return power_stats_; }

private:
    uint8_t wake_gpio_;
    unsigned long idle_timeout_ms_;
    PowerState current_state_;
    
    // Activity tracking
    unsigned long last_activity_time_;
    bool force_stay_awake_;
    
    // Sleep management
    bool sleep_configured_;
    unsigned long sleep_start_time_;
    
    // Power statistics
    PowerStats power_stats_;
    
    // Internal methods
    void configureWakeUpSource();
    void enterLightSleep();
    void handleWakeUp();
    void updatePowerStatistics();
    bool shouldEnterSleep() const;
    
    // Power state transitions
    void transitionToIdle();
    void transitionToSleep();
    void transitionToActive();
};

} // namespace Power