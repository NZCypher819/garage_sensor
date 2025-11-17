#include "power_manager.h"
#include "../config/hardware.h"

namespace Power {

PowerManager::PowerManager(uint8_t wake_gpio, unsigned long idle_timeout_ms)
    : wake_gpio_(wake_gpio)
    , idle_timeout_ms_(idle_timeout_ms)
    , current_state_(PowerState::ACTIVE)
    , last_activity_time_(0)
    , force_stay_awake_(false)
    , sleep_configured_(false)
    , sleep_start_time_(0)
    , power_stats_{0, 0, 0, 0.0f}
{
}

bool PowerManager::begin() {
    // Initialize power management
    last_activity_time_ = millis();
    current_state_ = PowerState::ACTIVE;
    force_stay_awake_ = false;
    
    // Configure wake-up source
    configureWakeUpSource();
    
    Serial.println("✓ Power manager initialized");
    Serial.printf("Idle timeout: %lu ms, Wake GPIO: %d\n", idle_timeout_ms_, wake_gpio_);
    
    return true;
}

void PowerManager::update() {
    unsigned long current_time = millis();
    
    // Update power statistics
    updatePowerStatistics();
    
    // Check for state transitions
    switch (current_state_) {
        case PowerState::ACTIVE:
            if (shouldEnterSleep()) {
                transitionToIdle();
            }
            break;
            
        case PowerState::IDLE:
            // Brief idle period before sleep to ensure no immediate activity
            if (current_time - last_activity_time_ > (idle_timeout_ms_ + 1000)) {
                transitionToSleep();
            } else if (!shouldEnterSleep()) {
                transitionToActive();
            }
            break;
            
        case PowerState::LIGHT_SLEEP:
            // Should not be in this state during normal update calls
            // This would happen after wake-up
            handleWakeUp();
            break;
            
        case PowerState::ERROR:
            // Stay in error state until manually reset
            break;
    }
}

void PowerManager::reportActivity() {
    last_activity_time_ = millis();
    
    // If we were sleeping or idle, wake up
    if (current_state_ != PowerState::ACTIVE) {
        transitionToActive();
    }
}

bool PowerManager::isInLowPowerMode() const {
    return current_state_ == PowerState::IDLE || current_state_ == PowerState::LIGHT_SLEEP;
}

void PowerManager::forceStayAwake(bool stay_awake) {
    force_stay_awake_ = stay_awake;
    
    if (stay_awake && current_state_ != PowerState::ACTIVE) {
        transitionToActive();
    }
}

float PowerManager::getCurrentConsumptionMA() const {
    // Estimate current consumption based on power state
    switch (current_state_) {
        case PowerState::ACTIVE:
            return 45.0f;  // Full ESP32 operation with WiFi, LEDs, sensors
            
        case PowerState::IDLE:
            return 25.0f;  // Reduced activity but still awake
            
        case PowerState::LIGHT_SLEEP:
            return 8.5f;   // Light sleep with GPIO wake-up enabled
            
        case PowerState::ERROR:
            return 50.0f;  // Error state may have higher consumption
            
        default:
            return 50.0f;
    }
}

void PowerManager::configureWakeUpSource() {
    if (!sleep_configured_) {
        // Configure GPIO wake-up source
        esp_sleep_enable_ext0_wakeup((gpio_num_t)wake_gpio_, 1); // Wake on HIGH (beam broken)
        
        // Optional: Configure timer wake-up as backup (every 60 seconds)
        esp_sleep_enable_timer_wakeup(60 * 1000000ULL); // 60 seconds in microseconds
        
        sleep_configured_ = true;
        Serial.printf("✓ Sleep wake-up configured: GPIO %d\n", wake_gpio_);
    }
}

void PowerManager::enterLightSleep() {
    Serial.println("Entering light sleep mode...");
    Serial.printf("Target current: <10mA (estimated: %.1fmA)\n", getCurrentConsumptionMA());
    
    sleep_start_time_ = millis();
    current_state_ = PowerState::LIGHT_SLEEP;
    
    // Flush serial output before sleep
    Serial.flush();
    
    // Enter light sleep (this function will block until wake-up)
    esp_err_t sleep_result = esp_light_sleep_start();
    
    // Execution resumes here after wake-up
    handleWakeUp();
}

void PowerManager::handleWakeUp() {
    unsigned long wake_time = millis();
    unsigned long sleep_duration = wake_time - sleep_start_time_;
    
    // Update sleep statistics
    power_stats_.total_sleep_time_ms += sleep_duration;
    power_stats_.wake_count++;
    power_stats_.last_wake_reason = esp_sleep_get_wakeup_cause();
    
    Serial.println("Waking from light sleep...");
    Serial.printf("Sleep duration: %lu ms, Wake reason: %lu\n", 
                 sleep_duration, power_stats_.last_wake_reason);
    
    // Transition back to active state
    transitionToActive();
    
    // Report activity to reset timeout
    reportActivity();
}

void PowerManager::updatePowerStatistics() {
    static unsigned long last_stats_update = 0;
    unsigned long current_time = millis();
    
    // Update statistics every 10 seconds
    if (current_time - last_stats_update > 10000) {
        // Calculate average consumption
        float current_consumption = getCurrentConsumptionMA();
        power_stats_.average_consumption_ma = 
            (power_stats_.average_consumption_ma * 0.9f) + (current_consumption * 0.1f);
        
        last_stats_update = current_time;
    }
}

bool PowerManager::shouldEnterSleep() const {
    if (force_stay_awake_) {
        return false;
    }
    
    unsigned long current_time = millis();
    unsigned long idle_time = current_time - last_activity_time_;
    
    return idle_time >= idle_timeout_ms_;
}

void PowerManager::transitionToIdle() {
    if (current_state_ != PowerState::IDLE) {
        Serial.println("Power state: ACTIVE → IDLE");
        current_state_ = PowerState::IDLE;
    }
}

void PowerManager::transitionToSleep() {
    if (current_state_ != PowerState::LIGHT_SLEEP) {
        Serial.println("Power state: IDLE → LIGHT_SLEEP");
        enterLightSleep();
    }
}

void PowerManager::transitionToActive() {
    if (current_state_ != PowerState::ACTIVE) {
        Serial.println("Power state: → ACTIVE");
        current_state_ = PowerState::ACTIVE;
        last_activity_time_ = millis();
    }
}

} // namespace Power