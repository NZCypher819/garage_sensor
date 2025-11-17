#include "parking_detector.h"

namespace Core {

ParkingDetector::ParkingDetector(Sensors::E3JKSensor& sensor, Actuators::ParkingLEDController& led)
    : sensor_(sensor), led_(led), current_state_(ParkingState::EMPTY), last_sensor_check_(0) {
    Serial.println("ParkingDetector initialized");
}

bool ParkingDetector::begin() {
    if (!sensor_.begin()) {
        Serial.println("ERROR: Sensor initialization failed");
        return false;
    }
    
    if (!led_.begin()) {
        Serial.println("ERROR: LED initialization failed"); 
        return false;
    }
    
    sensor_.setEnabled(true);
    Serial.println("ParkingDetector started");
    return true;
}

void ParkingDetector::update() {
    unsigned long current_time = millis();
    
    // Check sensor every 50ms for responsiveness
    if (current_time - last_sensor_check_ >= 50) {
        Sensors::BeamState beam_state = sensor_.getCurrentState();
        processBeamStateChange(beam_state);
        last_sensor_check_ = current_time;
    }
}

void ParkingDetector::processBeamStateChange(Sensors::BeamState beam_state) {
    ParkingState new_state = current_state_;
    
    switch (beam_state) {
        case Sensors::BeamState::BLOCKED:
            new_state = ParkingState::OCCUPIED;
            break;
        case Sensors::BeamState::CLEAR:
            new_state = ParkingState::EMPTY;
            break;
        case Sensors::BeamState::UNKNOWN:
            new_state = ParkingState::ERROR;
            break;
    }
    
    if (new_state != current_state_) {
        current_state_ = new_state;
        updateLED();
        
        Serial.print("Parking State: ");
        switch (current_state_) {
            case ParkingState::EMPTY: Serial.println("EMPTY"); break;
            case ParkingState::OCCUPIED: Serial.println("OCCUPIED"); break;
            case ParkingState::ERROR: Serial.println("ERROR"); break;
        }
    }
}

void ParkingDetector::updateLED() {
    bool show_parking = (current_state_ == ParkingState::OCCUPIED);
    led_.showParkingDetected(show_parking);
}

bool ParkingDetector::performSelfTest() {
    Serial.println("Performing self-test...");
    
    // Test LED
    led_.showParkingDetected(true);
    delay(250);
    led_.showParkingDetected(false);
    delay(250);
    
    // Check sensor
    if (!sensor_.isEnabled()) {
        Serial.println("Self-test FAILED: Sensor not enabled");
        return false;
    }
    
    Serial.println("Self-test PASSED");
    return true;
}

} // namespace Core