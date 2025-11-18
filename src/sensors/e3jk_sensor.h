/**
 * E3JK-RR11 IR Beam Sensor Driver
 * 
 * Handles 24V powered sensor with NO/NC relay contacts
 * - NO Contact (White wire): Normally open, closes when beam is blocked
 * - NC Contact (Grey wire): Normally closed, opens when beam is blocked
 * 
 * This implementation uses NO contact for reliable detection
 */

#pragma once

#include <Arduino.h>

namespace Sensors {

enum class BeamState {
    CLEAR = 0,
    BLOCKED = 1,
    UNKNOWN = 255
};

class E3JKSensor {
public:
    /**
     * Constructor for E3JK-RR11 sensor
     * @param pin GPIO pin connected to NO contact (white wire)
     * @param use_no_contact true for NO contact (default), false for NC contact
     */
    E3JKSensor(int pin, bool use_no_contact = true);
    bool begin();
    BeamState getCurrentState() const { return current_state_; }
    BeamState readRawState();  // Read current GPIO state
    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled);

private:
    int pin_;
    bool use_no_contact_;  // true for NO contact, false for NC contact
    bool enabled_;
    BeamState current_state_;
};

} // namespace Sensors