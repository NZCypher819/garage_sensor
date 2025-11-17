#include "version_manager.h"
#include "../diagnostics/logger.h"
#include <SPIFFS.h>
#include <time.h>

namespace Config {

// Build constants implementation
namespace BuildConstants {
    const char* VERSION = FIRMWARE_VERSION;
    const char* BUILD_DATE = BUILD_DATE;
    const char* BUILD_TIME = BUILD_TIME;
    const char* GIT_COMMIT = GIT_COMMIT;
    const char* GIT_BRANCH = GIT_BRANCH;
    const char* PLATFORM = "ESP32-S3";
    #ifdef DEBUG
    const bool DEBUG_BUILD = true;
    #else
    const bool DEBUG_BUILD = false;
    #endif
}

// Global instance
static VersionManager* g_version_manager = nullptr;

VersionManager& GlobalVersionManager() {
    if (!g_version_manager) {
        g_version_manager = new VersionManager();
    }
    return *g_version_manager;
}

VersionManager::VersionManager() 
    : version_file_path_("/version_history.json"),
      version_install_time_(millis()),
      first_boot_(true) {
}

VersionManager::~VersionManager() {
    saveVersionHistory();
}

bool VersionManager::begin() {
    if (!SPIFFS.begin()) {
        Logger::error("VersionManager: Failed to initialize SPIFFS");
        return false;
    }
    
    initializeBuildInfo();
    loadVersionHistory();
    
    // Check if this is a new version install
    if (version_history_.empty() || 
        version_history_.back().version != build_info_.version) {
        
        // Record new version installation
        recordVersionInstall(build_info_.version, "System boot");
        first_boot_ = true;
    } else {
        first_boot_ = false;
    }
    
    Logger::info("VersionManager initialized - Version: " + getCurrentVersion() + 
                ", First boot: " + (first_boot_ ? "yes" : "no"));
    
    return true;
}

void VersionManager::recordVersionInstall(const String& version, const String& reason) {
    VersionHistory entry;
    entry.version = version;
    entry.install_date = formatTimestamp(millis());
    entry.install_reason = reason;
    entry.successful = false; // Will be marked successful later
    entry.rollback_reason = "";
    
    addVersionEntry(entry);
    version_install_time_ = millis();
    
    Logger::info("Recorded version install: " + version + " (reason: " + reason + ")");
}

void VersionManager::markVersionSuccessful() {
    if (!version_history_.empty()) {
        version_history_.back().successful = true;
        saveVersionHistory();
        Logger::info("Marked version " + version_history_.back().version + " as successful");
    }
}

void VersionManager::recordVersionRollback(const String& from_version, 
                                          const String& to_version, 
                                          const String& reason) {
    // Mark previous version as unsuccessful
    if (!version_history_.empty()) {
        version_history_.back().successful = false;
        version_history_.back().rollback_reason = reason;
    }
    
    // Record rollback to previous version
    VersionHistory rollback_entry;
    rollback_entry.version = to_version;
    rollback_entry.install_date = formatTimestamp(millis());
    rollback_entry.install_reason = "Rollback from " + from_version;
    rollback_entry.successful = true; // Assume rollback is successful
    rollback_entry.rollback_reason = reason;
    
    addVersionEntry(rollback_entry);
    
    Logger::error("Recorded version rollback: " + from_version + " -> " + to_version + 
                 " (reason: " + reason + ")");
}

unsigned long VersionManager::getVersionUptime() const {
    return millis() - version_install_time_;
}

bool VersionManager::isFirstBoot() const {
    return first_boot_;
}

int VersionManager::getSuccessfulUpdates() const {
    int count = 0;
    for (const auto& entry : version_history_) {
        if (entry.successful && !entry.install_reason.startsWith("Rollback")) {
            count++;
        }
    }
    return count;
}

int VersionManager::getRollbackCount() const {
    int count = 0;
    for (const auto& entry : version_history_) {
        if (entry.install_reason.startsWith("Rollback")) {
            count++;
        }
    }
    return count;
}

String VersionManager::exportToJson(bool include_history) const {
    JsonDocument doc;
    
    // Current build info
    JsonObject build = doc["build"].to<JsonObject>();
    build["version"] = build_info_.version;
    build["build_date"] = build_info_.build_date;
    build["build_time"] = build_info_.build_time;
    build["git_commit"] = build_info_.git_commit;
    build["git_branch"] = build_info_.git_branch;
    build["compiler_version"] = build_info_.compiler_version;
    build["platform"] = build_info_.platform;
    build["debug_build"] = build_info_.debug_build;
    
    // Runtime info
    JsonObject runtime = doc["runtime"].to<JsonObject>();
    runtime["uptime_ms"] = millis();
    runtime["version_uptime_ms"] = getVersionUptime();
    runtime["first_boot"] = isFirstBoot();
    runtime["total_updates"] = getTotalUpdates();
    runtime["successful_updates"] = getSuccessfulUpdates();
    runtime["rollback_count"] = getRollbackCount();
    
    // Version history (if requested)
    if (include_history) {
        JsonArray history = doc["history"].to<JsonArray>();
        for (const auto& entry : version_history_) {
            JsonObject hist_entry = history.add<JsonObject>();
            hist_entry["version"] = entry.version;
            hist_entry["install_date"] = entry.install_date;
            hist_entry["install_reason"] = entry.install_reason;
            hist_entry["successful"] = entry.successful;
            if (!entry.rollback_reason.isEmpty()) {
                hist_entry["rollback_reason"] = entry.rollback_reason;
            }
        }
    }
    
    String json_output;
    serializeJson(doc, json_output);
    return json_output;
}

String VersionManager::generateStatusReport() const {
    String report = "=== Version Status Report ===\n";
    report += "Current Version: " + build_info_.version + "\n";
    report += "Build Date: " + build_info_.build_date + " " + build_info_.build_time + "\n";
    report += "Platform: " + build_info_.platform + "\n";
    report += "Git Commit: " + build_info_.git_commit + "\n";
    report += "Git Branch: " + build_info_.git_branch + "\n";
    report += "Debug Build: " + String(build_info_.debug_build ? "Yes" : "No") + "\n";
    report += "System Uptime: " + String(millis() / 1000) + " seconds\n";
    report += "Version Uptime: " + String(getVersionUptime() / 1000) + " seconds\n";
    report += "First Boot: " + String(isFirstBoot() ? "Yes" : "No") + "\n";
    report += "Total Updates: " + String(getTotalUpdates()) + "\n";
    report += "Successful Updates: " + String(getSuccessfulUpdates()) + "\n";
    report += "Rollback Count: " + String(getRollbackCount()) + "\n";
    
    if (!version_history_.empty()) {
        report += "\n=== Recent Version History ===\n";
        int count = 0;
        for (int i = version_history_.size() - 1; i >= 0 && count < 5; i--, count++) {
            const auto& entry = version_history_[i];
            report += entry.version + " - " + entry.install_date + 
                     " (" + entry.install_reason + ")";
            if (!entry.successful) {
                report += " [FAILED]";
                if (!entry.rollback_reason.isEmpty()) {
                    report += " - " + entry.rollback_reason;
                }
            }
            report += "\n";
        }
    }
    
    return report;
}

void VersionManager::initializeBuildInfo() {
    build_info_.version = BuildConstants::VERSION;
    build_info_.build_date = BuildConstants::BUILD_DATE;
    build_info_.build_time = BuildConstants::BUILD_TIME;
    build_info_.git_commit = BuildConstants::GIT_COMMIT;
    build_info_.git_branch = BuildConstants::GIT_BRANCH;
    build_info_.platform = BuildConstants::PLATFORM;
    build_info_.debug_build = BuildConstants::DEBUG_BUILD;
    
    // Get compiler version
    #ifdef __GNUC__
    build_info_.compiler_version = "GCC " + String(__GNUC__) + "." + 
                                  String(__GNUC_MINOR__) + "." + 
                                  String(__GNUC_PATCHLEVEL__);
    #else
    build_info_.compiler_version = "Unknown";
    #endif
    
    Logger::debug("Build info initialized: " + build_info_.version + 
                 " (" + build_info_.build_date + " " + build_info_.build_time + ")");
}

bool VersionManager::loadVersionHistory() {
    if (!SPIFFS.exists(version_file_path_)) {
        Logger::info("No version history file found, starting fresh");
        return true;
    }
    
    File version_file = SPIFFS.open(version_file_path_, "r");
    if (!version_file) {
        Logger::error("Failed to open version history file");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, version_file);
    version_file.close();
    
    if (error) {
        Logger::error("Failed to parse version history: " + String(error.c_str()));
        return false;
    }
    
    JsonArray history = doc["history"];
    version_history_.clear();
    
    for (JsonObject entry : history) {
        VersionHistory hist_entry;
        hist_entry.version = entry["version"].as<String>();
        hist_entry.install_date = entry["install_date"].as<String>();
        hist_entry.install_reason = entry["install_reason"].as<String>();
        hist_entry.successful = entry["successful"].as<bool>();
        hist_entry.rollback_reason = entry["rollback_reason"].as<String>();
        
        version_history_.push_back(hist_entry);
    }
    
    Logger::info("Loaded " + String(version_history_.size()) + " version history entries");
    return true;
}

bool VersionManager::saveVersionHistory() {
    JsonDocument doc;
    JsonArray history = doc["history"].to<JsonArray>();
    
    for (const auto& entry : version_history_) {
        JsonObject hist_entry = history.add<JsonObject>();
        hist_entry["version"] = entry.version;
        hist_entry["install_date"] = entry.install_date;
        hist_entry["install_reason"] = entry.install_reason;
        hist_entry["successful"] = entry.successful;
        if (!entry.rollback_reason.isEmpty()) {
            hist_entry["rollback_reason"] = entry.rollback_reason;
        }
    }
    
    doc["saved_at"] = formatTimestamp(millis());
    
    File version_file = SPIFFS.open(version_file_path_, "w");
    if (!version_file) {
        Logger::error("Failed to create version history file");
        return false;
    }
    
    if (serializeJson(doc, version_file) == 0) {
        version_file.close();
        Logger::error("Failed to write version history");
        return false;
    }
    
    version_file.close();
    Logger::debug("Version history saved (" + String(version_history_.size()) + " entries)");
    return true;
}

void VersionManager::addVersionEntry(const VersionHistory& entry) {
    version_history_.push_back(entry);
    
    // Limit history size to prevent excessive storage usage
    const size_t MAX_HISTORY_ENTRIES = 20;
    if (version_history_.size() > MAX_HISTORY_ENTRIES) {
        version_history_.erase(version_history_.begin());
    }
    
    saveVersionHistory();
}

String VersionManager::formatTimestamp(unsigned long timestamp) const {
    // Simple timestamp formatting (could be enhanced with real-time clock)
    unsigned long seconds = timestamp / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    return String(days) + "d " + String(hours % 24) + "h " + 
           String(minutes % 60) + "m " + String(seconds % 60) + "s";
}

} // namespace Config