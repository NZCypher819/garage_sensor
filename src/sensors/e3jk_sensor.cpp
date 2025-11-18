#include "e3jk_sensor.h"

namespace Sensors {

E3JKSensor::E3JKSensor(int pin, bool use_no_contact) 
    : pin_(pin), use_no_contact_(use_no_contact), enabled_(false), current_state_(BeamState::UNKNOWN) {
    Serial.printf("E3JK-RR11 sensor initialized on GPIO %d (%s contact)\n", 
                 pin, use_no_contact ? "NO" : "NC");
}

bool E3JKSensor::begin() {
    // Configure GPIO with pull-up resistor for reliable contact detection
    pinMode(pin_, INPUT_PULLUP);
    enabled_ = true;
    current_state_ = readRawState();
    Serial.printf("E3JK-RR11 sensor started, initial state: %s\n", 
                 current_state_ == BeamState::CLEAR ? "CLEAR" : 
                 current_state_ == BeamState::BLOCKED ? "BLOCKED" : "UNKNOWN");
    return true;
}

void E3JKSensor::setEnabled(bool enabled) {
    enabled_ = enabled;
}

BeamState E3JKSensor::readRawState() {
    if (!enabled_) {
        return BeamState::UNKNOWN;
    }
    
    // Use analog threshold detection with hysteresis and multiple samples
    // Take 5 samples and average them to reduce noise
    int adc_sum = 0;
    for (int i = 0; i < 5; i++) {
        adc_sum += analogRead(pin_);
        delayMicroseconds(100);  // Small delay between samples
    }
    int adc_value = adc_sum / 5;
    float voltage = (adc_value / 4095.0) * 3.3;
    
    // Simple threshold: 0V (ADC > 0) to detect BLOCKED
    // This sensor outputs 2-3V when blocked, 0V when clear
    bool beam_blocked = (adc_value > 0);
    
    // For logging, also read digital state
    bool gpio_reading = digitalRead(pin_);
    
    BeamState new_state = beam_blocked ? BeamState::BLOCKED : BeamState::CLEAR;
    
    // Debug logging - log every state read AND log state changes immediately
    static unsigned long last_debug_log = 0;
    static BeamState last_logged_state = BeamState::UNKNOWN;
    unsigned long now = millis();
    
    // Log immediately on state change
    if (new_state != last_logged_state) {
        Serial.printf("*** [SENSOR] STATE CHANGE! GPIO %d: %s (raw: %d, ADC: %d, %.2fV) -> Beam: %s ***\n", 
                     pin_, 
                     gpio_reading ? "HIGH" : "LOW",
                     gpio_reading,
                     adc_value,
                     voltage,
                     new_state == BeamState::BLOCKED ? "BLOCKED" : "CLEAR");
        last_logged_state = new_state;
        last_debug_log = now;
    }
    // Also log periodically even if no change
    else if (now - last_debug_log > 1000) {  // Log every second
        Serial.printf("[SENSOR] GPIO %d reading: %s (raw: %d, ADC: %d, %.2fV), Beam: %s [Contact type: %s]\n", 
                     pin_, 
                     gpio_reading ? "HIGH" : "LOW",
                     gpio_reading,
                     adc_value,
                     voltage,
                     new_state == BeamState::BLOCKED ? "BLOCKED" : "CLEAR",
                     use_no_contact_ ? "NO (white wire)" : "NC (grey wire)");
        last_debug_log = now;
    }
    
    current_state_ = new_state;
    return current_state_;
}

} // namespace Sensors