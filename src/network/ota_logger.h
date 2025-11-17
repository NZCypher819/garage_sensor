#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../diagnostics/logger.h"

namespace Network {

/**
 * @brief Comprehensive logging for OTA operations
 * 
 * Provides detailed logging and diagnostics for OTA update process:
 * - Update attempt tracking with detailed logs
 * - Performance metrics collection  
 * - Error analysis and debugging support
 * - Constitutional compliance with observability principle V
 */
class OTALogger {
public:
    enum class LogLevel {
        DEBUG,
        INFO, 
        WARNING,
        ERROR,
        CRITICAL
    };

    struct UpdateEvent {
        unsigned long timestamp;
        String stage;
        String message;
        LogLevel level;
        String details;
    };

    struct UpdateMetrics {
        String version_from;
        String version_to;
        unsigned long start_time;
        unsigned long end_time;
        unsigned long download_duration;
        unsigned long validation_duration;
        unsigned long installation_duration;
        size_t download_size;
        float download_speed_kbps;
        bool success;
        String failure_reason;
        int retry_count;
    };

    struct DiagnosticData {
        unsigned long system_uptime;
        size_t free_heap;
        float wifi_signal_strength;
        String wifi_ssid;
        unsigned long last_update_attempt;
        int total_update_attempts;
        int successful_updates;
        int failed_updates;
        std::vector<String> recent_errors;
    };

    OTALogger();
    ~OTALogger();

    /**
     * @brief Initialize OTA logger
     * @param log_file_path Path for persistent log storage
     * @param max_events Maximum events to keep in memory
     * @return true if initialization successful
     */
    bool begin(const String& log_file_path = "/ota_logs.json", size_t max_events = 100);

    /**
     * @brief Start logging for new update attempt
     * @param from_version Current version
     * @param to_version Target version
     */
    void startUpdateLogging(const String& from_version, const String& to_version);

    /**
     * @brief Log update stage progress
     * @param stage Current stage name
     * @param message Stage message
     * @param level Log level
     * @param details Optional detailed information
     */
    void logStage(const String& stage, const String& message, 
                  LogLevel level = LogLevel::INFO, const String& details = "");

    /**
     * @brief Log download progress
     * @param downloaded_bytes Bytes downloaded
     * @param total_bytes Total file size
     * @param speed_kbps Download speed in KB/s
     */
    void logDownloadProgress(size_t downloaded_bytes, size_t total_bytes, float speed_kbps);

    /**
     * @brief Log validation progress  
     * @param processed_bytes Bytes processed for validation
     * @param total_bytes Total bytes to validate
     */
    void logValidationProgress(size_t processed_bytes, size_t total_bytes);

    /**
     * @brief Log installation progress
     * @param written_bytes Bytes written to flash
     * @param total_bytes Total firmware size
     */
    void logInstallationProgress(size_t written_bytes, size_t total_bytes);

    /**
     * @brief Complete update logging
     * @param success Whether update was successful
     * @param result_message Final result message
     */
    void completeUpdateLogging(bool success, const String& result_message);

    /**
     * @brief Log error with context
     * @param error_message Error description
     * @param error_code Optional error code
     * @param context Optional context information
     */
    void logError(const String& error_message, int error_code = 0, const String& context = "");

    /**
     * @brief Get current update metrics
     */
    UpdateMetrics getCurrentMetrics() const { return current_metrics_; }

    /**
     * @brief Get recent update events
     * @param count Number of recent events to retrieve
     */
    std::vector<UpdateEvent> getRecentEvents(size_t count = 10) const;

    /**
     * @brief Generate diagnostic data for troubleshooting
     */
    DiagnosticData generateDiagnosticData() const;

    /**
     * @brief Export logs to JSON format
     * @param include_events Include event history
     * @param include_metrics Include performance metrics
     * @param include_diagnostics Include diagnostic data
     */
    String exportToJson(bool include_events = true, 
                       bool include_metrics = true,
                       bool include_diagnostics = true) const;

    /**
     * @brief Generate human-readable log report
     */
    String generateLogReport() const;

    /**
     * @brief Clear old log entries
     * @param older_than_ms Clear entries older than specified time
     */
    void clearOldLogs(unsigned long older_than_ms = 7 * 24 * 60 * 60 * 1000); // 7 days default

    /**
     * @brief Save logs to persistent storage
     */
    bool saveLogs();

    /**
     * @brief Load logs from persistent storage  
     */
    bool loadLogs();

    /**
     * @brief Set log level threshold
     * @param min_level Minimum level to log
     */
    void setLogLevel(LogLevel min_level) { min_log_level_ = min_level; }

    /**
     * @brief Enable/disable console output
     */
    void setConsoleOutput(bool enabled) { console_output_ = enabled; }

    /**
     * @brief Enable/disable persistent logging
     */
    void setPersistentLogging(bool enabled) { persistent_logging_ = enabled; }

private:
    std::vector<UpdateEvent> events_;
    UpdateMetrics current_metrics_;
    String log_file_path_;
    size_t max_events_;
    LogLevel min_log_level_;
    bool console_output_;
    bool persistent_logging_;
    bool update_in_progress_;

    void addEvent(const String& stage, const String& message, LogLevel level, const String& details);
    String formatLogLevel(LogLevel level) const;
    String formatTimestamp(unsigned long timestamp) const;
    void trimEventHistory();
    void updateSystemMetrics();
    size_t calculateLogSize() const;
    
    // Performance timing helpers
    unsigned long stage_start_time_;
    String current_stage_;
};

/**
 * @brief Global OTA logger instance
 */
extern OTALogger& GlobalOTALogger();

/**
 * @brief Convenience macros for OTA logging
 */
#define OTA_LOG_DEBUG(stage, message, ...) \
    GlobalOTALogger().logStage(stage, message, Network::OTALogger::LogLevel::DEBUG, ##__VA_ARGS__)

#define OTA_LOG_INFO(stage, message, ...) \
    GlobalOTALogger().logStage(stage, message, Network::OTALogger::LogLevel::INFO, ##__VA_ARGS__)

#define OTA_LOG_WARNING(stage, message, ...) \
    GlobalOTALogger().logStage(stage, message, Network::OTALogger::LogLevel::WARNING, ##__VA_ARGS__)

#define OTA_LOG_ERROR(stage, message, ...) \
    GlobalOTALogger().logStage(stage, message, Network::OTALogger::LogLevel::ERROR, ##__VA_ARGS__)

#define OTA_LOG_CRITICAL(stage, message, ...) \
    GlobalOTALogger().logStage(stage, message, Network::OTALogger::LogLevel::CRITICAL, ##__VA_ARGS__)

} // namespace Network