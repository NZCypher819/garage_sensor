#pragma once

#include <Arduino.h>
#include <String.h>

namespace Diagnostics {

/**
 * Simple logging utility for debugging and diagnostics
 * Provides different log levels and structured output
 */
class Logger {
public:
    enum class LogLevel {
        DEBUG = 0,
        INFO = 1, 
        WARN = 2,
        ERROR = 3
    };

    /**
     * Set the global log level
     * Only messages at or above this level will be printed
     */
    static void setLogLevel(LogLevel level);

    /**
     * Logging functions for different levels
     */
    static void debug(const String& message);
    static void info(const String& message);
    static void warn(const String& message);
    static void error(const String& message);

    /**
     * Logging with component context
     */
    static void debug(const String& component, const String& message);
    static void info(const String& component, const String& message);
    static void warn(const String& component, const String& message);
    static void error(const String& component, const String& message);

private:
    static LogLevel current_log_level_;
    static void log(LogLevel level, const String& component, const String& message);
    static String levelToString(LogLevel level);
    static String formatTimestamp();
};

// Convenience macros for component-based logging
#define LOG_DEBUG(component, message) Diagnostics::Logger::debug(component, message)
#define LOG_INFO(component, message) Diagnostics::Logger::info(component, message)
#define LOG_WARN(component, message) Diagnostics::Logger::warn(component, message)
#define LOG_ERROR(component, message) Diagnostics::Logger::error(component, message)

} // namespace Diagnostics