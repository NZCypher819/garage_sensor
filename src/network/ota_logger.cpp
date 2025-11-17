#include "ota_logger.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <esp_system.h>

namespace Network {

// Global instance
static OTALogger* g_ota_logger = nullptr;

OTALogger& GlobalOTALogger() {
    if (!g_ota_logger) {
        g_ota_logger = new OTALogger();
    }
    return *g_ota_logger;
}

OTALogger::OTALogger() 
    : max_events_(100),
      min_log_level_(LogLevel::INFO),
      console_output_(true),
      persistent_logging_(true),
      update_in_progress_(false),
      stage_start_time_(0) {
    
    // Initialize current metrics
    memset(&current_metrics_, 0, sizeof(current_metrics_));
}

OTALogger::~OTALogger() {
    if (persistent_logging_) {
        saveLogs();
    }
}

bool OTALogger::begin(const String& log_file_path, size_t max_events) {
    log_file_path_ = log_file_path;
    max_events_ = max_events;
    
    if (!SPIFFS.begin()) {
        Logger::error("OTALogger: Failed to initialize SPIFFS");
        return false;
    }
    
    if (persistent_logging_) {
        loadLogs();
    }
    
    logStage("System", "OTA Logger initialized", LogLevel::INFO, 
             "Max events: " + String(max_events) + ", Persistent: " + 
             (persistent_logging_ ? "Yes" : "No"));
    
    return true;
}

void OTALogger::startUpdateLogging(const String& from_version, const String& to_version) {
    // Complete any previous update logging
    if (update_in_progress_) {
        completeUpdateLogging(false, "Previous update interrupted");
    }
    
    // Initialize new update metrics
    current_metrics_ = UpdateMetrics{};
    current_metrics_.version_from = from_version;
    current_metrics_.version_to = to_version;
    current_metrics_.start_time = millis();
    
    update_in_progress_ = true;
    stage_start_time_ = millis();
    
    logStage("Update", "Starting OTA update", LogLevel::INFO,
             "From: " + from_version + ", To: " + to_version);
    
    updateSystemMetrics();
}

void OTALogger::logStage(const String& stage, const String& message, 
                        LogLevel level, const String& details) {
    
    // Update timing for stage transitions
    if (current_stage_ != stage && !current_stage_.isEmpty()) {
        unsigned long stage_duration = millis() - stage_start_time_;
        
        // Record stage duration in metrics
        if (current_stage_ == "Downloading") {
            current_metrics_.download_duration = stage_duration;
        } else if (current_stage_ == "Validating") {
            current_metrics_.validation_duration = stage_duration;
        } else if (current_stage_ == "Installing") {
            current_metrics_.installation_duration = stage_duration;
        }
    }
    
    current_stage_ = stage;
    stage_start_time_ = millis();
    
    addEvent(stage, message, level, details);
}

void OTALogger::logDownloadProgress(size_t downloaded_bytes, size_t total_bytes, float speed_kbps) {
    current_metrics_.download_size = total_bytes;
    current_metrics_.download_speed_kbps = speed_kbps;
    
    String progress_msg = "Downloaded " + String(downloaded_bytes) + "/" + 
                         String(total_bytes) + " bytes (" + 
                         String((float)downloaded_bytes/total_bytes*100.0, 1) + "%)";
    
    String details = "Speed: " + String(speed_kbps, 1) + " KB/s";
    
    logStage("Downloading", progress_msg, LogLevel::DEBUG, details);
}

void OTALogger::logValidationProgress(size_t processed_bytes, size_t total_bytes) {
    String progress_msg = "Validated " + String(processed_bytes) + "/" + 
                         String(total_bytes) + " bytes (" + 
                         String((float)processed_bytes/total_bytes*100.0, 1) + "%)";
    
    logStage("Validating", progress_msg, LogLevel::DEBUG);
}

void OTALogger::logInstallationProgress(size_t written_bytes, size_t total_bytes) {
    String progress_msg = "Installed " + String(written_bytes) + "/" + 
                         String(total_bytes) + " bytes (" + 
                         String((float)written_bytes/total_bytes*100.0, 1) + "%)";
    
    logStage("Installing", progress_msg, LogLevel::DEBUG);
}

void OTALogger::completeUpdateLogging(bool success, const String& result_message) {
    if (!update_in_progress_) {
        return;
    }
    
    current_metrics_.end_time = millis();
    current_metrics_.success = success;
    current_metrics_.failure_reason = success ? "" : result_message;
    
    unsigned long total_duration = current_metrics_.end_time - current_metrics_.start_time;
    
    LogLevel level = success ? LogLevel::INFO : LogLevel::ERROR;
    String stage = success ? "Complete" : "Failed";
    
    String details = "Duration: " + String(total_duration) + "ms";
    if (current_metrics_.download_size > 0) {
        details += ", Size: " + String(current_metrics_.download_size) + " bytes";
    }
    if (current_metrics_.download_speed_kbps > 0) {
        details += ", Speed: " + String(current_metrics_.download_speed_kbps, 1) + " KB/s";
    }
    
    logStage(stage, result_message, level, details);
    
    update_in_progress_ = false;
    current_stage_ = "";
    
    if (persistent_logging_) {
        saveLogs();
    }
}

void OTALogger::logError(const String& error_message, int error_code, const String& context) {
    String details = "";
    if (error_code != 0) {
        details += "Code: " + String(error_code);
    }
    if (!context.isEmpty()) {
        if (!details.isEmpty()) details += ", ";
        details += "Context: " + context;
    }
    
    logStage("Error", error_message, LogLevel::ERROR, details);
    
    current_metrics_.retry_count++;
}

std::vector<OTALogger::UpdateEvent> OTALogger::getRecentEvents(size_t count) const {
    std::vector<UpdateEvent> recent;
    size_t start = (events_.size() > count) ? events_.size() - count : 0;
    
    for (size_t i = start; i < events_.size(); i++) {
        recent.push_back(events_[i]);
    }
    
    return recent;
}

OTALogger::DiagnosticData OTALogger::generateDiagnosticData() const {
    DiagnosticData data;
    data.system_uptime = millis();
    data.free_heap = esp_get_free_heap_size();
    
    if (WiFi.status() == WL_CONNECTED) {
        data.wifi_signal_strength = WiFi.RSSI();
        data.wifi_ssid = WiFi.SSID();
    } else {
        data.wifi_signal_strength = 0;
        data.wifi_ssid = "Not connected";
    }
    
    // Calculate update statistics
    data.total_update_attempts = 0;
    data.successful_updates = 0;
    data.failed_updates = 0;
    data.last_update_attempt = 0;
    
    for (const auto& event : events_) {
        if (event.stage == "Update" && event.message.startsWith("Starting")) {
            data.total_update_attempts++;
            if (data.last_update_attempt == 0) {
                data.last_update_attempt = event.timestamp;
            }
        } else if (event.stage == "Complete") {
            data.successful_updates++;
        } else if (event.stage == "Failed") {
            data.failed_updates++;
        }
        
        if (event.level == LogLevel::ERROR || event.level == LogLevel::CRITICAL) {
            data.recent_errors.push_back(event.message);
            if (data.recent_errors.size() > 10) {
                data.recent_errors.erase(data.recent_errors.begin());
            }
        }
    }
    
    return data;
}

String OTALogger::exportToJson(bool include_events, bool include_metrics, bool include_diagnostics) const {
    JsonDocument doc;
    
    // Basic info
    doc["generated_at"] = millis();
    doc["log_level"] = formatLogLevel(min_log_level_);
    doc["events_count"] = events_.size();
    
    // Current metrics
    if (include_metrics) {
        JsonObject metrics = doc["current_metrics"].to<JsonObject>();
        metrics["version_from"] = current_metrics_.version_from;
        metrics["version_to"] = current_metrics_.version_to;
        metrics["start_time"] = current_metrics_.start_time;
        metrics["end_time"] = current_metrics_.end_time;
        metrics["download_duration"] = current_metrics_.download_duration;
        metrics["validation_duration"] = current_metrics_.validation_duration;
        metrics["installation_duration"] = current_metrics_.installation_duration;
        metrics["download_size"] = current_metrics_.download_size;
        metrics["download_speed_kbps"] = current_metrics_.download_speed_kbps;
        metrics["success"] = current_metrics_.success;
        metrics["failure_reason"] = current_metrics_.failure_reason;
        metrics["retry_count"] = current_metrics_.retry_count;
    }
    
    // Recent events
    if (include_events) {
        JsonArray events = doc["events"].to<JsonArray>();
        for (const auto& event : events_) {
            JsonObject event_obj = events.add<JsonObject>();
            event_obj["timestamp"] = event.timestamp;
            event_obj["stage"] = event.stage;
            event_obj["message"] = event.message;
            event_obj["level"] = formatLogLevel(event.level);
            if (!event.details.isEmpty()) {
                event_obj["details"] = event.details;
            }
        }
    }
    
    // Diagnostic data
    if (include_diagnostics) {
        DiagnosticData diag = generateDiagnosticData();
        JsonObject diagnostics = doc["diagnostics"].to<JsonObject>();
        diagnostics["system_uptime"] = diag.system_uptime;
        diagnostics["free_heap"] = diag.free_heap;
        diagnostics["wifi_signal_strength"] = diag.wifi_signal_strength;
        diagnostics["wifi_ssid"] = diag.wifi_ssid;
        diagnostics["last_update_attempt"] = diag.last_update_attempt;
        diagnostics["total_update_attempts"] = diag.total_update_attempts;
        diagnostics["successful_updates"] = diag.successful_updates;
        diagnostics["failed_updates"] = diag.failed_updates;
        
        if (!diag.recent_errors.empty()) {
            JsonArray errors = diagnostics["recent_errors"].to<JsonArray>();
            for (const auto& error : diag.recent_errors) {
                errors.add(error);
            }
        }
    }
    
    String json_output;
    serializeJson(doc, json_output);
    return json_output;
}

String OTALogger::generateLogReport() const {
    String report = "=== OTA Update Log Report ===\n";
    report += "Generated: " + formatTimestamp(millis()) + "\n";
    report += "Total Events: " + String(events_.size()) + "\n";
    
    if (update_in_progress_) {
        report += "Status: Update in progress (" + current_stage_ + ")\n";
        report += "Current Update: " + current_metrics_.version_from + " -> " + current_metrics_.version_to + "\n";
    } else {
        report += "Status: No active update\n";
    }
    
    // System info
    DiagnosticData diag = generateDiagnosticData();
    report += "\n=== System Status ===\n";
    report += "Uptime: " + formatTimestamp(diag.system_uptime) + "\n";
    report += "Free Heap: " + String(diag.free_heap) + " bytes\n";
    report += "WiFi: " + diag.wifi_ssid + " (" + String(diag.wifi_signal_strength) + " dBm)\n";
    report += "Update Attempts: " + String(diag.total_update_attempts) + 
             " (Success: " + String(diag.successful_updates) + 
             ", Failed: " + String(diag.failed_updates) + ")\n";
    
    // Recent events
    report += "\n=== Recent Events ===\n";
    auto recent = getRecentEvents(10);
    for (const auto& event : recent) {
        report += "[" + formatTimestamp(event.timestamp) + "] " + 
                 formatLogLevel(event.level) + " " + event.stage + 
                 ": " + event.message;
        if (!event.details.isEmpty()) {
            report += " (" + event.details + ")";
        }
        report += "\n";
    }
    
    // Recent errors
    if (!diag.recent_errors.empty()) {
        report += "\n=== Recent Errors ===\n";
        for (const auto& error : diag.recent_errors) {
            report += "- " + error + "\n";
        }
    }
    
    return report;
}

void OTALogger::clearOldLogs(unsigned long older_than_ms) {
    unsigned long cutoff = millis() - older_than_ms;
    
    auto it = events_.begin();
    while (it != events_.end()) {
        if (it->timestamp < cutoff) {
            it = events_.erase(it);
        } else {
            ++it;
        }
    }
    
    logStage("Maintenance", "Cleared old log entries", LogLevel::INFO,
             "Entries remaining: " + String(events_.size()));
    
    if (persistent_logging_) {
        saveLogs();
    }
}

bool OTALogger::saveLogs() {
    if (log_file_path_.isEmpty()) {
        return false;
    }
    
    File log_file = SPIFFS.open(log_file_path_, "w");
    if (!log_file) {
        Logger::error("Failed to create OTA log file: " + log_file_path_);
        return false;
    }
    
    String json_data = exportToJson(true, true, false);
    if (log_file.print(json_data) != json_data.length()) {
        log_file.close();
        Logger::error("Failed to write OTA log data");
        return false;
    }
    
    log_file.close();
    Logger::debug("OTA logs saved (" + String(events_.size()) + " events, " + 
                 String(calculateLogSize()) + " bytes)");
    
    return true;
}

bool OTALogger::loadLogs() {
    if (!SPIFFS.exists(log_file_path_)) {
        Logger::info("No existing OTA log file found");
        return true;
    }
    
    File log_file = SPIFFS.open(log_file_path_, "r");
    if (!log_file) {
        Logger::error("Failed to open OTA log file: " + log_file_path_);
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, log_file);
    log_file.close();
    
    if (error) {
        Logger::error("Failed to parse OTA log file: " + String(error.c_str()));
        return false;
    }
    
    // Load events
    events_.clear();
    JsonArray events = doc["events"];
    for (JsonObject event_obj : events) {
        UpdateEvent event;
        event.timestamp = event_obj["timestamp"].as<unsigned long>();
        event.stage = event_obj["stage"].as<String>();
        event.message = event_obj["message"].as<String>();
        
        String level_str = event_obj["level"].as<String>();
        if (level_str == "DEBUG") event.level = LogLevel::DEBUG;
        else if (level_str == "INFO") event.level = LogLevel::INFO;
        else if (level_str == "WARNING") event.level = LogLevel::WARNING;
        else if (level_str == "ERROR") event.level = LogLevel::ERROR;
        else if (level_str == "CRITICAL") event.level = LogLevel::CRITICAL;
        else event.level = LogLevel::INFO;
        
        event.details = event_obj["details"].as<String>();
        
        events_.push_back(event);
    }
    
    Logger::info("Loaded " + String(events_.size()) + " OTA log events");
    return true;
}

void OTALogger::addEvent(const String& stage, const String& message, 
                        LogLevel level, const String& details) {
    
    if (level < min_log_level_) {
        return;
    }
    
    UpdateEvent event;
    event.timestamp = millis();
    event.stage = stage;
    event.message = message;
    event.level = level;
    event.details = details;
    
    events_.push_back(event);
    trimEventHistory();
    
    // Console output
    if (console_output_) {
        String log_msg = "[" + formatTimestamp(event.timestamp) + "] " + 
                        formatLogLevel(level) + " " + stage + ": " + message;
        if (!details.isEmpty()) {
            log_msg += " (" + details + ")";
        }
        
        if (level >= LogLevel::ERROR) {
            Logger::error(log_msg);
        } else if (level >= LogLevel::WARNING) {
            Logger::warning(log_msg);
        } else {
            Logger::info(log_msg);
        }
    }
}

String OTALogger::formatLogLevel(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

String OTALogger::formatTimestamp(unsigned long timestamp) const {
    unsigned long seconds = timestamp / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    return String(hours % 24) + ":" + 
           String(minutes % 60) + ":" + 
           String(seconds % 60);
}

void OTALogger::trimEventHistory() {
    if (events_.size() > max_events_) {
        size_t excess = events_.size() - max_events_;
        events_.erase(events_.begin(), events_.begin() + excess);
    }
}

void OTALogger::updateSystemMetrics() {
    // This could be called periodically to update system state
    // For now, just log current system status
    logStage("System", "System metrics updated", LogLevel::DEBUG,
             "Free heap: " + String(esp_get_free_heap_size()) + " bytes");
}

size_t OTALogger::calculateLogSize() const {
    // Rough estimate of log size in memory
    size_t size = sizeof(UpdateMetrics);
    
    for (const auto& event : events_) {
        size += sizeof(UpdateEvent);
        size += event.stage.length();
        size += event.message.length();
        size += event.details.length();
    }
    
    return size;
}

} // namespace Network