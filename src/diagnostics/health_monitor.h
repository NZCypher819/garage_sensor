#pragma once

#include <Arduino.h>
#include "../actuators/status_led_controller.h"

namespace Diagnostics {

/**
 * System Health Monitor for User Story 2 - System Health Indication
 * 
 * Monitors system status and controls status LED patterns based on:
 * - System initialization state
 * - Sensor health and connectivity
 * - Overall system operational status
 * 
 * Constitutional Compliance:
 * - I. Reliability-First: Proactive fault detection
 * - V. Observability: System health visibility
 */
class HealthMonitor {
public:
    enum class SystemHealth {
        INITIALIZING,    // System starting up
        HEALTHY,         // All systems operational
        SENSOR_FAULT,    // Sensor disconnected or faulty
        SYSTEM_ERROR     // Critical system error
    };

    /**
     * Constructor
     * @param status_led Reference to status LED controller
     */
    HealthMonitor(Actuators::StatusLEDController& status_led);

    /**
     * Initialize health monitoring
     * @return true if initialization successful
     */
    bool begin();

    /**
     * Update health monitoring (call from main loop)
     */
    void update();

    /**
     * Set system health state
     * @param health Current system health status
     */
    void setSystemHealth(SystemHealth health);

    /**
     * Get current system health
     * @return Current health state
     */
    SystemHealth getSystemHealth() const { return current_health_; }

    /**
     * Report sensor connectivity
     * @param connected true if sensor is responding properly
     */
    void reportSensorStatus(bool connected);

    /**
     * Check if system is healthy
     * @return true if system is operating normally
     */
    bool isSystemHealthy() const { return current_health_ == SystemHealth::HEALTHY; }

    /**
     * Force emergency state
     */
    void triggerEmergencyState();

private:
    Actuators::StatusLEDController& status_led_;
    SystemHealth current_health_;
    
    // Health monitoring state
    unsigned long last_sensor_check_;
    unsigned long sensor_fault_start_;
    bool sensor_connected_;
    
    // Health check intervals
    static const unsigned long SENSOR_CHECK_INTERVAL_MS = 5000;  // 5 seconds
    static const unsigned long FAULT_THRESHOLD_MS = 10000;       // 10 seconds to declare fault
    
    // Internal methods
    void updateStatusLED();
    bool performSensorHealthCheck();
};

} // namespace Diagnostics