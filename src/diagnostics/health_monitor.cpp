#include "health_monitor.h"

namespace Diagnostics {

HealthMonitor::HealthMonitor(Actuators::StatusLEDController& status_led)
    : status_led_(status_led)
    , current_health_(SystemHealth::INITIALIZING)
    , last_sensor_check_(0)
    , sensor_fault_start_(0)
    , sensor_connected_(false)
{
}

bool HealthMonitor::begin() {
    // Initialize health monitoring
    current_health_ = SystemHealth::INITIALIZING;
    last_sensor_check_ = millis();
    sensor_connected_ = false;
    
    // Set startup pattern on status LED
    status_led_.setPattern(Actuators::StatusLEDController::StatusPattern::STARTUP);
    
    Serial.println("✓ Health monitor initialized");
    return true;
}

void HealthMonitor::update() {
    unsigned long current_time = millis();
    
    // Update status LED patterns
    status_led_.update();
    
    // Periodic sensor health checks
    if (current_time - last_sensor_check_ >= SENSOR_CHECK_INTERVAL_MS) {
        performSensorHealthCheck();
        last_sensor_check_ = current_time;
    }
    
    // Check for fault escalation
    if (!sensor_connected_ && sensor_fault_start_ == 0) {
        sensor_fault_start_ = current_time;
    } else if (sensor_connected_) {
        sensor_fault_start_ = 0;  // Reset fault timer
    }
    
    // Determine overall health state
    SystemHealth new_health = current_health_;
    
    if (current_health_ == SystemHealth::INITIALIZING) {
        // Wait for startup sequence to complete
        if (status_led_.getCurrentPattern() == Actuators::StatusLEDController::StatusPattern::NORMAL) {
            new_health = SystemHealth::HEALTHY;
        }
    } else if (sensor_fault_start_ > 0 && 
               (current_time - sensor_fault_start_) > FAULT_THRESHOLD_MS) {
        new_health = SystemHealth::SENSOR_FAULT;
    } else if (sensor_connected_) {
        new_health = SystemHealth::HEALTHY;
    }
    
    // Update health state if changed
    if (new_health != current_health_) {
        setSystemHealth(new_health);
    }
}

void HealthMonitor::setSystemHealth(SystemHealth health) {
    if (health != current_health_) {
        SystemHealth old_health = current_health_;
        current_health_ = health;
        
        Serial.printf("Health state change: %d -> %d\n", (int)old_health, (int)health);
        
        updateStatusLED();
    }
}

void HealthMonitor::reportSensorStatus(bool connected) {
    sensor_connected_ = connected;
}

void HealthMonitor::triggerEmergencyState() {
    setSystemHealth(SystemHealth::SYSTEM_ERROR);
}

void HealthMonitor::updateStatusLED() {
    switch (current_health_) {
        case SystemHealth::INITIALIZING:
            status_led_.setPattern(Actuators::StatusLEDController::StatusPattern::STARTUP);
            break;
            
        case SystemHealth::HEALTHY:
            status_led_.setPattern(Actuators::StatusLEDController::StatusPattern::NORMAL);
            break;
            
        case SystemHealth::SENSOR_FAULT:
        case SystemHealth::SYSTEM_ERROR:
            status_led_.setPattern(Actuators::StatusLEDController::StatusPattern::ERROR);
            break;
    }
}

bool HealthMonitor::performSensorHealthCheck() {
    // Simple health check - in a full implementation this would
    // communicate with sensor driver to verify connectivity
    // For now, assume sensor is connected if we can read from it
    
    // This will be updated when integrated with sensor driver
    sensor_connected_ = true;  // Default to connected for now
    
    return sensor_connected_;
}

} // namespace Diagnostics