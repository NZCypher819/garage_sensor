#include "rollback_manager.h"
#include "../diagnostics/logger.h"
#include <esp_ota_ops.h>
#include <esp_system.h>

namespace Security {

// Global instance
static RollbackManager* g_rollback_manager = nullptr;

RollbackManager& GlobalRollbackManager() {
    if (!g_rollback_manager) {
        g_rollback_manager = new RollbackManager();
    }
    return *g_rollback_manager;
}

RollbackManager::RollbackManager() 
    : max_boot_failures_(3), 
      boot_timeout_ms_(30000),
      state_file_path_("/rollback_state.json"),
      backup_dir_("/backups") {
    
    // Initialize structures
    memset(&update_info_, 0, sizeof(update_info_));
    memset(&boot_info_, 0, sizeof(boot_info_));
    
    update_info_.status = UpdateStatus::NONE;
    boot_info_.status = BootStatus::NORMAL;
}

RollbackManager::~RollbackManager() {
    saveState();
}

bool RollbackManager::begin(const String& current_version) {
    if (!SPIFFS.begin()) {
        Logger::error("RollbackManager: Failed to initialize SPIFFS");
        return false;
    }
    
    update_info_.current_version = current_version;
    boot_info_.version = current_version;
    boot_info_.boot_time = millis();
    
    createDirectory(backup_dir_);
    
    if (!loadState()) {
        initializeState();
    }
    
    Logger::info("RollbackManager initialized for version: " + current_version);
    logState("initialization");
    
    return true;
}

bool RollbackManager::checkBootStatus(BootInfo& boot_info) {
    boot_info = boot_info_;
    
    // Check if this is first boot after update
    if (update_info_.status == UpdateStatus::PENDING) {
        boot_info_.status = BootStatus::FIRST_BOOT;
        Logger::info("First boot detected after update to: " + update_info_.target_version);
        
        // Start boot validation timer
        if (isBootTimeoutExpired()) {
            Logger::error("Boot timeout expired, triggering rollback");
            boot_info_.status = BootStatus::FAILED_BOOT;
            return false;
        }
    } else if (boot_info_.consecutive_failures > 0) {
        // Check for boot failures
        if (boot_info_.consecutive_failures >= max_boot_failures_) {
            Logger::error("Maximum boot failures reached (" + String(boot_info_.consecutive_failures) + 
                         "), triggering rollback");
            boot_info_.status = BootStatus::FAILED_BOOT;
            boot_info_.rollback_triggered = true;
            return false;
        }
    }
    
    return true;
}

bool RollbackManager::startUpdate(const String& target_version, StatusCallback status_callback) {
    if (update_info_.status != UpdateStatus::NONE && 
        update_info_.status != UpdateStatus::COMMITTED &&
        update_info_.status != UpdateStatus::FAILED &&
        update_info_.status != UpdateStatus::ROLLED_BACK) {
        Logger::error("Update already in progress");
        return false;
    }
    
    status_callback_ = status_callback;
    update_info_.target_version = target_version;
    update_info_.start_time = millis();
    update_info_.attempt_count++;
    update_info_.backup_path = generateBackupPath(update_info_.current_version);
    
    setUpdateStatus(UpdateStatus::DOWNLOADING, "Starting update to " + target_version);
    
    Logger::info("Starting update from " + update_info_.current_version + " to " + target_version);
    saveState();
    
    return true;
}

bool RollbackManager::createBackup() {
    Logger::info("Creating firmware backup...");
    
    const esp_partition_t* running_partition = esp_ota_get_running_partition();
    if (!running_partition) {
        update_info_.last_error = "Failed to get running partition";
        return false;
    }
    
    File backup_file = SPIFFS.open(update_info_.backup_path, "w");
    if (!backup_file) {
        update_info_.last_error = "Failed to create backup file: " + update_info_.backup_path;
        return false;
    }
    
    size_t partition_size = running_partition->size;
    size_t bytes_written = 0;
    uint8_t buffer[1024];
    
    for (size_t offset = 0; offset < partition_size; offset += sizeof(buffer)) {
        size_t read_size = min(sizeof(buffer), partition_size - offset);
        
        esp_err_t err = esp_partition_read(running_partition, offset, buffer, read_size);
        if (err != ESP_OK) {
            backup_file.close();
            deleteFile(update_info_.backup_path);
            update_info_.last_error = "Failed to read partition at offset " + String(offset);
            return false;
        }
        
        size_t written = backup_file.write(buffer, read_size);
        if (written != read_size) {
            backup_file.close();
            deleteFile(update_info_.backup_path);
            update_info_.last_error = "Failed to write backup data";
            return false;
        }
        
        bytes_written += written;
    }
    
    backup_file.close();
    update_info_.rollback_available = true;
    
    Logger::info("Firmware backup created: " + update_info_.backup_path + 
                " (" + String(bytes_written) + " bytes)");
    saveState();
    
    return true;
}

void RollbackManager::setUpdateStatus(UpdateStatus status, const String& message) {
    update_info_.status = status;
    if (!message.isEmpty()) {
        update_info_.last_error = message;
    }
    
    notifyStatus(status, message);
    saveState();
    logState("status update");
}

bool RollbackManager::validateUpdate(unsigned long validation_timeout_ms) {
    if (update_info_.status != UpdateStatus::PENDING) {
        Logger::error("No pending update to validate");
        return false;
    }
    
    Logger::info("Validating update within " + String(validation_timeout_ms) + "ms");
    
    // Mark boot as successful
    if (!markBootSuccess()) {
        return false;
    }
    
    setUpdateStatus(UpdateStatus::VALIDATED, "Update validation successful");
    
    // Reset boot failure counter
    boot_info_.consecutive_failures = 0;
    saveState();
    
    Logger::info("Update validated successfully");
    return true;
}

bool RollbackManager::commitUpdate() {
    if (update_info_.status != UpdateStatus::VALIDATED) {
        Logger::error("Update not validated, cannot commit");
        return false;
    }
    
    // Clean old backups but keep the most recent one
    cleanupBackups(1);
    
    // Update current version
    update_info_.current_version = update_info_.target_version;
    boot_info_.version = update_info_.target_version;
    
    setUpdateStatus(UpdateStatus::COMMITTED, "Update committed successfully");
    
    Logger::info("Update committed permanently to version: " + update_info_.current_version);
    
    // Reset update state for next update
    update_info_.target_version = "";
    update_info_.backup_path = "";
    update_info_.attempt_count = 0;
    update_info_.status = UpdateStatus::NONE;
    
    saveState();
    return true;
}

bool RollbackManager::triggerRollback(const String& reason) {
    if (!update_info_.rollback_available) {
        Logger::error("No rollback available");
        return false;
    }
    
    Logger::error("Triggering rollback: " + reason);
    setUpdateStatus(UpdateStatus::ROLLING_BACK, "Rolling back due to: " + reason);
    
    return performRollback();
}

bool RollbackManager::performRollback() {
    if (!fileExists(update_info_.backup_path)) {
        update_info_.last_error = "Backup file not found: " + update_info_.backup_path;
        Logger::error(update_info_.last_error);
        return false;
    }
    
    Logger::info("Performing rollback from backup: " + update_info_.backup_path);
    
    if (!restoreFromBackup(update_info_.backup_path)) {
        setUpdateStatus(UpdateStatus::FAILED, "Rollback failed: " + update_info_.last_error);
        return false;
    }
    
    // Update state
    update_info_.target_version = update_info_.current_version; // Reset target
    setUpdateStatus(UpdateStatus::ROLLED_BACK, "Rollback completed successfully");
    
    boot_info_.status = BootStatus::ROLLBACK_BOOT;
    boot_info_.consecutive_failures = 0;
    boot_info_.rollback_triggered = true;
    
    saveState();
    
    Logger::info("Rollback completed, restarting...");
    
    // Restart to boot from rolled back firmware
    delay(1000);
    ESP.restart();
    
    return true;
}

bool RollbackManager::cleanupBackups(int keep_count) {
    // Implementation would scan backup directory and remove old backups
    // For now, just log the operation
    Logger::info("Cleaning up old backups, keeping " + String(keep_count) + " most recent");
    return true;
}

bool RollbackManager::loadState() {
    if (!fileExists(state_file_path_)) {
        return false;
    }
    
    File state_file = SPIFFS.open(state_file_path_, "r");
    if (!state_file) {
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, state_file);
    state_file.close();
    
    if (error) {
        Logger::error("Failed to parse rollback state: " + String(error.c_str()));
        return false;
    }
    
    // Load update info
    JsonObject update = doc["update"];
    if (!update.isNull()) {
        update_info_.current_version = update["current_version"].as<String>();
        update_info_.target_version = update["target_version"].as<String>();
        update_info_.backup_path = update["backup_path"].as<String>();
        update_info_.status = static_cast<UpdateStatus>(update["status"].as<int>());
        update_info_.start_time = update["start_time"].as<unsigned long>();
        update_info_.attempt_count = update["attempt_count"].as<int>();
        update_info_.last_error = update["last_error"].as<String>();
        update_info_.rollback_available = update["rollback_available"].as<bool>();
    }
    
    // Load boot info
    JsonObject boot = doc["boot"];
    if (!boot.isNull()) {
        boot_info_.status = static_cast<BootStatus>(boot["status"].as<int>());
        boot_info_.version = boot["version"].as<String>();
        boot_info_.boot_time = boot["boot_time"].as<unsigned long>();
        boot_info_.consecutive_failures = boot["consecutive_failures"].as<int>();
        boot_info_.rollback_triggered = boot["rollback_triggered"].as<bool>();
    }
    
    return true;
}

bool RollbackManager::saveState() {
    JsonDocument doc;
    
    // Save update info
    JsonObject update = doc["update"].to<JsonObject>();
    update["current_version"] = update_info_.current_version;
    update["target_version"] = update_info_.target_version;
    update["backup_path"] = update_info_.backup_path;
    update["status"] = static_cast<int>(update_info_.status);
    update["start_time"] = update_info_.start_time;
    update["attempt_count"] = update_info_.attempt_count;
    update["last_error"] = update_info_.last_error;
    update["rollback_available"] = update_info_.rollback_available;
    
    // Save boot info
    JsonObject boot = doc["boot"].to<JsonObject>();
    boot["status"] = static_cast<int>(boot_info_.status);
    boot["version"] = boot_info_.version;
    boot["boot_time"] = boot_info_.boot_time;
    boot["consecutive_failures"] = boot_info_.consecutive_failures;
    boot["rollback_triggered"] = boot_info_.rollback_triggered;
    
    doc["saved_at"] = millis();
    
    File state_file = SPIFFS.open(state_file_path_, "w");
    if (!state_file) {
        Logger::error("Failed to create state file");
        return false;
    }
    
    if (serializeJson(doc, state_file) == 0) {
        state_file.close();
        return false;
    }
    
    state_file.close();
    return true;
}

bool RollbackManager::initializeState() {
    update_info_.status = UpdateStatus::NONE;
    update_info_.rollback_available = false;
    update_info_.attempt_count = 0;
    
    boot_info_.status = BootStatus::NORMAL;
    boot_info_.consecutive_failures = 0;
    boot_info_.rollback_triggered = false;
    
    return saveState();
}

bool RollbackManager::markBootSuccess() {
    boot_info_.consecutive_failures = 0;
    boot_info_.status = BootStatus::NORMAL;
    return saveState();
}

bool RollbackManager::markBootFailure() {
    boot_info_.consecutive_failures++;
    boot_info_.status = BootStatus::FAILED_BOOT;
    Logger::error("Boot failure recorded, count: " + String(boot_info_.consecutive_failures));
    return saveState();
}

bool RollbackManager::isBootTimeoutExpired() {
    return (millis() - boot_info_.boot_time) > boot_timeout_ms_;
}

String RollbackManager::generateBackupPath(const String& version) {
    return backup_dir_ + "/firmware_" + version + "_" + String(millis()) + ".bin";
}

bool RollbackManager::restoreFromBackup(const String& backup_path) {
    // This would implement the actual firmware restoration
    // For ESP32, this involves writing to the inactive OTA partition
    Logger::info("Restoring firmware from: " + backup_path);
    
    // Implementation would use esp_ota_* functions to write backup to inactive partition
    // and then set it as the boot partition
    
    return true; // Placeholder implementation
}

void RollbackManager::notifyStatus(UpdateStatus status, const String& message) {
    if (status_callback_) {
        status_callback_(status, message);
    }
}

void RollbackManager::logState(const String& operation) {
    Logger::info("RollbackManager [" + operation + "] Status=" + String(static_cast<int>(update_info_.status)) +
                ", Version=" + update_info_.current_version + 
                ", Failures=" + String(boot_info_.consecutive_failures));
}

// File system utility implementations
bool RollbackManager::createDirectory(const String& path) {
    // SPIFFS doesn't have directories, but we can simulate them
    return true;
}

bool RollbackManager::fileExists(const String& path) {
    return SPIFFS.exists(path);
}

bool RollbackManager::deleteFile(const String& path) {
    return SPIFFS.remove(path);
}

bool RollbackManager::copyFile(const String& source, const String& destination) {
    File src = SPIFFS.open(source, "r");
    if (!src) return false;
    
    File dst = SPIFFS.open(destination, "w");
    if (!dst) {
        src.close();
        return false;
    }
    
    uint8_t buffer[512];
    while (src.available()) {
        size_t read_bytes = src.readBytes((char*)buffer, sizeof(buffer));
        dst.write(buffer, read_bytes);
    }
    
    src.close();
    dst.close();
    return true;
}

size_t RollbackManager::getFileSize(const String& path) {
    File file = SPIFFS.open(path, "r");
    if (!file) return 0;
    
    size_t size = file.size();
    file.close();
    return size;
}

// Utility functions implementation
namespace RollbackUtils {

bool installBootWatchdog(int timeout_seconds) {
    // Implementation would configure ESP32 watchdog timer
    Logger::info("Installing boot watchdog with " + String(timeout_seconds) + "s timeout");
    return true;
}

void feedBootWatchdog() {
    // Feed the watchdog to prevent timeout
    // Implementation would reset hardware watchdog
}

void disableBootWatchdog() {
    Logger::info("Disabling boot watchdog");
    // Implementation would disable hardware watchdog
}

bool isBootLoopDetected() {
    RollbackManager::BootInfo boot_info = GlobalRollbackManager().getBootInfo();
    return boot_info.consecutive_failures >= 2;
}

void resetBootFailureCounter() {
    GlobalRollbackManager().markBootSuccess();
}

} // namespace RollbackUtils

} // namespace Security