#pragma once

#include "../sensors/e3jk_sensor.h"
#include "../actuators/parking_led_controller.h"

namespace Core {

enum class ParkingState {
    EMPTY = 0,
    OCCUPIED = 1,
    ERROR = 255
};

class ParkingDetector {
public:
    ParkingDetector(Sensors::E3JKSensor& sensor, Actuators::ParkingLEDController& led);
    bool begin();
    void update();
    ParkingState getCurrentState() const { return current_state_; }
    bool performSelfTest();

private:
    Sensors::E3JKSensor& sensor_;
    Actuators::ParkingLEDController& led_;
    ParkingState current_state_;
    unsigned long last_sensor_check_;
    
    void processBeamStateChange(Sensors::BeamState beam_state);
    void updateLED();
};

} // namespace Core