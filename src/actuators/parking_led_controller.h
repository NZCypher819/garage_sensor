#pragma once

#include <Arduino.h>

namespace Actuators {

enum class LEDState {
    OFF = 0,
    ON = 1
};

class ParkingLEDController {
public:
    ParkingLEDController(int pin, bool activeHigh = true);
    bool begin();
    bool showParkingDetected(bool show_parking);
    LEDState getCurrentState() const { return current_state_; }

private:
    int pin_;
    bool active_high_;
    LEDState current_state_;
    bool current_physical_state_;
    
    bool setPhysicalState(bool on);
};

} // namespace Actuators