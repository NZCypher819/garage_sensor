#include "parking_led_controller.h"

namespace Actuators {

ParkingLEDController::ParkingLEDController(int pin, bool activeHigh) 
    : pin_(pin), active_high_(activeHigh), current_state_(LEDState::OFF), current_physical_state_(false) {
    Serial.println("ParkingLEDController initialized");
}

bool ParkingLEDController::begin() {
    pinMode(pin_, OUTPUT);
    setPhysicalState(false);
    Serial.println("ParkingLEDController started");
    return true;
}

bool ParkingLEDController::showParkingDetected(bool show_parking) {
    bool success = setPhysicalState(show_parking);
    if (success) {
        current_state_ = show_parking ? LEDState::ON : LEDState::OFF;
        Serial.print("Parking LED: ");
        Serial.println(show_parking ? "ON" : "OFF");
    }
    return success;
}

bool ParkingLEDController::setPhysicalState(bool on) {
    digitalWrite(pin_, (on && active_high_) || (!on && !active_high_) ? HIGH : LOW);
    current_physical_state_ = on;
    return true;
}

} // namespace Actuators