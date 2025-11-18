#include "logger.h"

namespace Diagnostics {

// Static member initialization
Logger::LogLevel Logger::current_log_level_ = Logger::LogLevel::INFO;

void Logger::setLogLevel(LogLevel level) {
    current_log_level_ = level;
}

void Logger::debug(const String& message) {
    debug("", message);
}

void Logger::info(const String& message) {
    info("", message);
}

void Logger::warn(const String& message) {
    warn("", message);
}

void Logger::error(const String& message) {
    error("", message);
}

void Logger::debug(const String& component, const String& message) {
    log(LogLevel::DEBUG, component, message);
}

void Logger::info(const String& component, const String& message) {
    log(LogLevel::INFO, component, message);
}

void Logger::warn(const String& component, const String& message) {
    log(LogLevel::WARN, component, message);
}

void Logger::error(const String& component, const String& message) {
    log(LogLevel::ERROR, component, message);
}

void Logger::log(LogLevel level, const String& component, const String& message) {
    // Only log if the message level is at or above the current log level
    if (level < current_log_level_) {
        return;
    }

    String timestamp = formatTimestamp();
    String levelStr = levelToString(level);
    
    // Format: [TIMESTAMP] [LEVEL] [COMPONENT] MESSAGE
    String logMessage = "[" + timestamp + "] [" + levelStr + "]";
    
    if (!component.isEmpty()) {
        logMessage += " [" + component + "]";
    }
    
    logMessage += " " + message;
    
    Serial.println(logMessage);
}

String Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default:              return "UNKN ";
    }
}

String Logger::formatTimestamp() {
    // Simple timestamp using millis()
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    ms %= 1000;
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    char timestamp[16];
    snprintf(timestamp, sizeof(timestamp), "%02lu:%02lu:%02lu.%03lu", 
             hours, minutes, seconds, ms);
    
    return String(timestamp);
}

} // namespace Diagnostics