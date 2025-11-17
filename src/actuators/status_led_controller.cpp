#include "status_led_controller.h"
#include "../config/hardware.h"

namespace Actuators {

StatusLEDController::StatusLEDController(uint8_t pin, bool active_high)
    : pin_(pin)
    , active_high_(active_high)
    , current_pattern_(StatusPattern::OFF)
    , pattern_start_time_(0)
    , last_toggle_time_(0)
    , led_state_(false)
    , blink_count_(0)
    , pwm_channel_(STATUS_LED_PWM_CH)
{
}

bool StatusLEDController::begin() {
    // Setup PWM channel for brightness control
    ledcSetup(pwm_channel_, LED_PWM_FREQUENCY, LED_PWM_RESOLUTION);
    ledcAttachPin(pin_, pwm_channel_);
    
    // Initialize LED to off state
    setPhysicalState(false);
    
    Serial.println("✓ Status LED controller initialized");
    return true;
}

void StatusLEDController::setPattern(StatusPattern pattern) {
    if (pattern != current_pattern_) {
        current_pattern_ = pattern;
        pattern_start_time_ = millis();
        last_toggle_time_ = pattern_start_time_;
        blink_count_ = 0;
        
        // Immediate pattern setup
        switch (pattern) {
            case StatusPattern::OFF:
                setPhysicalState(false);
                break;
                
            case StatusPattern::STARTUP:
                setPhysicalState(true);  // Start with LED on
                led_state_ = true;
                Serial.println("Status LED: Starting startup sequence");
                break;
                
            case StatusPattern::NORMAL:
                setPhysicalState(true, 64);  // 25% brightness (64/255)
                Serial.println("Status LED: Normal operation mode");
                break;
                
            case StatusPattern::ERROR:
                setPhysicalState(true);  // Start with LED on
                led_state_ = true;
                Serial.println("Status LED: Error pattern activated");
                break;
        }
    }
}

void StatusLEDController::update() {
    switch (current_pattern_) {
        case StatusPattern::OFF:
            // Nothing to update
            break;
            
        case StatusPattern::STARTUP:
            updateStartupPattern();
            break;
            
        case StatusPattern::NORMAL:
            updateNormalPattern();
            break;
            
        case StatusPattern::ERROR:
            updateErrorPattern();
            break;
    }
}

void StatusLEDController::forceOff() {
    current_pattern_ = StatusPattern::OFF;
    setPhysicalState(false);
}

void StatusLEDController::setPhysicalState(bool on, uint8_t brightness) {
    uint8_t duty_cycle;
    
    if (on) {
        duty_cycle = active_high_ ? brightness : (255 - brightness);
    } else {
        duty_cycle = active_high_ ? 0 : 255;
    }
    
    ledcWrite(pwm_channel_, duty_cycle);
}

void StatusLEDController::updateStartupPattern() {
    // Startup pattern: 2x blinks (200ms on, 200ms off)
    unsigned long current_time = millis();
    unsigned long elapsed = current_time - last_toggle_time_;
    
    if (elapsed >= 200) {  // 200ms intervals
        if (led_state_) {
            // LED is on, turn it off
            setPhysicalState(false);
            led_state_ = false;
            last_toggle_time_ = current_time;
        } else {
            // LED is off, turn it on
            blink_count_++;
            if (blink_count_ <= 2) {  // Only 2 blinks total
                setPhysicalState(true);
                led_state_ = true;
                last_toggle_time_ = current_time;
            } else {
                // Startup sequence complete, switch to normal
                setPattern(StatusPattern::NORMAL);
            }
        }
    }
}

void StatusLEDController::updateNormalPattern() {
    // Normal pattern: Steady 25% brightness
    // Already set in setPattern(), no updates needed
}

void StatusLEDController::updateErrorPattern() {
    // Error pattern: Rapid blinks (100ms on, 100ms off)
    unsigned long current_time = millis();
    unsigned long elapsed = current_time - last_toggle_time_;
    
    if (elapsed >= 100) {  // 100ms intervals
        led_state_ = !led_state_;
        setPhysicalState(led_state_);
        last_toggle_time_ = current_time;
    }
}

} // namespace Actuators