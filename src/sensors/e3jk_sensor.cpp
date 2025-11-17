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
    
    bool gpio_reading = digitalRead(pin_);
    
    // NO Contact Logic: Contact closes (LOW) when beam is blocked
    // NC Contact Logic: Contact opens (HIGH) when beam is blocked
    bool beam_blocked;
    if (use_no_contact_) {
        beam_blocked = (gpio_reading == LOW);  // NO contact closed
    } else {
        beam_blocked = (gpio_reading == HIGH); // NC contact open
    }
    
    current_state_ = beam_blocked ? BeamState::BLOCKED : BeamState::CLEAR;
    return current_state_;
}

} // namespace Sensors