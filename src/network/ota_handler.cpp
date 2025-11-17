#include "ota_handler.h"
#include "../diagnostics/logger.h"
#include <WiFi.h>
#include <Update.h>
#include <esp_ota_ops.h>

namespace Network {

// Global instance
static OTAHandler* g_ota_handler = nullptr;

OTAHandler& GlobalOTAHandler() {
    if (!g_ota_handler) {
        g_ota_handler = new OTAHandler();
    }
    return *g_ota_handler;
}

OTAHandler::OTAHandler() 
    : last_result_(UpdateResult::SUCCESS),
      last_check_time_(0),
      update_in_progress_(false) {
}

OTAHandler::~OTAHandler() {
    if (update_in_progress_) {
        cancelUpdate();
    }
}

bool OTAHandler::begin(const UpdateConfiguration& config) {
    if (!validateConfiguration(config)) {
        return false;
    }
    
    config_ = config;
    
    // Initialize GitHub client
    if (!github_client_.begin(config.repository_owner, config.repository_name)) {
        setError("Failed to initialize GitHub client");
        return false;
    }
    
    // Initialize rollback manager
    if (!rollback_manager_.begin(config.current_version)) {
        setError("Failed to initialize rollback manager");
        return false;
    }
    
    // Check boot status for rollback detection
    Security::RollbackManager::BootInfo boot_info;
    if (!rollback_manager_.checkBootStatus(boot_info)) {
        Logger::error("Boot validation failed, triggering rollback");
        rollback_manager_.performRollback();
        return false;
    }
    
    // Validate successful boot if pending update
    if (boot_info.status == Security::RollbackManager::BootStatus::FIRST_BOOT) {
        Logger::info("Validating first boot after update");
        if (rollback_manager_.validateUpdate(config.validation_timeout_ms)) {
            rollback_manager_.commitUpdate();
            Logger::info("Update validation successful, committed permanently");
        } else {
            Logger::error("Update validation failed");
            rollback_manager_.triggerRollback("Boot validation timeout");
        }
    }
    
    loadState();
    
    Logger::info("OTAHandler initialized for " + config.repository_owner + "/" + 
                config.repository_name + " (current: " + config.current_version + ")");
    
    return true;
}

bool OTAHandler::checkForUpdates(GitHubClient::ReleaseInfo& release_info) {
    if (!isNetworkAvailable()) {
        setError("Network not available");
        return false;
    }
    
    Logger::info("Checking for updates...");
    
    if (!github_client_.getLatestRelease(release_info, config_.include_prereleases)) {
        setError("Failed to fetch release information: " + github_client_.getLastError());
        return false;
    }
    
    if (!isNewerVersion(release_info.tag_name, config_.current_version)) {
        Logger::info("No newer version available (latest: " + release_info.tag_name + 
                    ", current: " + config_.current_version + ")");
        return false;
    }
    
    Logger::info("New version available: " + release_info.tag_name + 
                " (current: " + config_.current_version + ")");
    
    last_check_time_ = millis();
    return true;
}

bool OTAHandler::startUpdate(ProgressCallback progress_callback, ResultCallback result_callback) {
    GitHubClient::ReleaseInfo release_info;
    if (!checkForUpdates(release_info)) {
        reportResult(UpdateResult::NO_UPDATE, "No updates available");
        return false;
    }
    
    return startUpdate(release_info, progress_callback, result_callback);
}

bool OTAHandler::startUpdate(const GitHubClient::ReleaseInfo& release_info,
                            ProgressCallback progress_callback,
                            ResultCallback result_callback) {
    if (update_in_progress_) {
        setError("Update already in progress");
        return false;
    }
    
    // Check for manual approval if required
    if (config_.require_manual_approval && approval_callback_) {
        if (!approval_callback_(release_info)) {
            reportResult(UpdateResult::NO_UPDATE, "Update not approved by user");
            return false;
        }
    }
    
    progress_callback_ = progress_callback;
    result_callback_ = result_callback;
    update_in_progress_ = true;
    
    Logger::info("Starting OTA update to version: " + release_info.tag_name);
    
    // Start update process with rollback manager
    if (!rollback_manager_.startUpdate(release_info.tag_name)) {
        setError("Failed to start rollback manager");
        reportResult(UpdateResult::ROLLBACK_TRIGGERED, last_error_);
        return false;
    }
    
    reportProgress(5.0f, "Preparing", "Creating firmware backup");
    
    // Create backup of current firmware
    if (!rollback_manager_.createBackup()) {
        setError("Failed to create firmware backup");
        reportResult(UpdateResult::INSTALLATION_FAILED, last_error_);
        update_in_progress_ = false;
        return false;
    }
    
    reportProgress(15.0f, "Downloading", "Starting firmware download");
    
    // Download new firmware
    if (!downloadFirmware(release_info)) {
        reportResult(UpdateResult::DOWNLOAD_FAILED, last_error_);
        update_in_progress_ = false;
        return false;
    }
    
    reportProgress(60.0f, "Validating", "Checking firmware integrity");
    
    // Validate firmware integrity
    if (!validateFirmware(config_.download_path, release_info.sha256_hash)) {
        reportResult(UpdateResult::VALIDATION_FAILED, last_error_);
        update_in_progress_ = false;
        return false;
    }
    
    reportProgress(75.0f, "Installing", "Installing new firmware");
    
    // Install firmware
    if (!installFirmware(config_.download_path)) {
        reportResult(UpdateResult::INSTALLATION_FAILED, last_error_);
        update_in_progress_ = false;
        return false;
    }
    
    reportProgress(95.0f, "Finalizing", "Preparing for reboot");
    
    // Mark update as pending validation
    rollback_manager_.setUpdateStatus(Security::RollbackManager::UpdateStatus::PENDING, 
                                     "Update installed, pending validation");
    
    reportProgress(100.0f, "Complete", "Update installed successfully");
    reportResult(UpdateResult::SUCCESS, "Update completed, reboot required for activation");
    
    update_in_progress_ = false;
    
    // Schedule reboot
    Logger::info("Update installation completed, rebooting in 5 seconds...");
    delay(5000);
    ESP.restart();
    
    return true;
}

void OTAHandler::processPeriodicCheck() {
    if (!config_.auto_check_enabled || update_in_progress_) {
        return;
    }
    
    unsigned long now = millis();
    if (now - last_check_time_ < config_.check_interval_ms) {
        return;
    }
    
    GitHubClient::ReleaseInfo release_info;
    if (checkForUpdates(release_info)) {
        if (config_.require_manual_approval) {
            Logger::info("Update available but manual approval required: " + release_info.tag_name);
        } else {
            Logger::info("Starting automatic update to: " + release_info.tag_name);
            startUpdate(release_info);
        }
    }
}

bool OTAHandler::cancelUpdate() {
    if (!update_in_progress_) {
        return false;
    }
    
    Logger::info("Cancelling OTA update");
    update_in_progress_ = false;
    
    cleanupTempFiles();
    
    rollback_manager_.setUpdateStatus(Security::RollbackManager::UpdateStatus::FAILED, 
                                     "Update cancelled by user");
    
    reportResult(UpdateResult::NO_UPDATE, "Update cancelled");
    return true;
}

void OTAHandler::updateConfiguration(const UpdateConfiguration& config) {
    if (validateConfiguration(config)) {
        config_ = config;
        saveState();
    }
}

bool OTAHandler::isUpdateInProgress() const {
    return update_in_progress_ || 
           rollback_manager_.getUpdateInfo().status == Security::RollbackManager::UpdateStatus::DOWNLOADING ||
           rollback_manager_.getUpdateInfo().status == Security::RollbackManager::UpdateStatus::INSTALLING;
}

bool OTAHandler::downloadFirmware(const GitHubClient::ReleaseInfo& release_info) {
    Logger::info("Downloading firmware: " + release_info.download_url + 
                " (" + String(release_info.file_size) + " bytes)");
    
    // Check available storage
    if (!ensureStorageSpace(release_info.file_size)) {
        setError("Insufficient storage space");
        return false;
    }
    
    // Progress callback for download
    auto download_progress = [this](const GitHubClient::DownloadProgress& progress) {
        float percentage = 15.0f + (progress.percentage * 0.45f); // Map to 15-60% range
        String message = String(progress.downloaded_bytes) + "/" + String(progress.total_bytes) + " bytes";
        reportProgress(percentage, "Downloading", message);
    };
    
    // Error callback for download
    auto download_error = [this](const String& error) {
        setError("Download failed: " + error);
    };
    
    rollback_manager_.setUpdateStatus(Security::RollbackManager::UpdateStatus::DOWNLOADING, 
                                     "Downloading firmware");
    
    bool success = github_client_.downloadFirmware(release_info.download_url, 
                                                  config_.download_path,
                                                  download_progress,
                                                  download_error);
    
    if (!success) {
        setError("Firmware download failed: " + github_client_.getLastError());
        return false;
    }
    
    Logger::info("Firmware download completed");
    return true;
}

bool OTAHandler::validateFirmware(const String& file_path, const String& expected_hash) {
    if (expected_hash.isEmpty()) {
        Logger::warning("No hash provided for firmware validation, skipping integrity check");
        return true;
    }
    
    Logger::info("Validating firmware integrity...");
    
    rollback_manager_.setUpdateStatus(Security::RollbackManager::UpdateStatus::VALIDATING, 
                                     "Validating firmware integrity");
    
    // Progress callback for validation
    auto validation_progress = [this](const Security::IntegrityCheck::HashProgress& progress) {
        float percentage = 60.0f + (progress.percentage * 0.15f); // Map to 60-75% range
        String message = "Processed " + String(progress.processed_bytes) + " bytes";
        reportProgress(percentage, "Validating", message);
    };
    
    bool valid = integrity_checker_.verifyFileIntegrity(file_path, expected_hash, validation_progress);
    
    if (!valid) {
        setError("Firmware integrity validation failed: " + integrity_checker_.getLastError());
        return false;
    }
    
    Logger::info("Firmware integrity validation passed");
    return true;
}

bool OTAHandler::installFirmware(const String& file_path) {
    Logger::info("Installing firmware: " + file_path);
    
    if (!SPIFFS.begin()) {
        setError("Failed to initialize SPIFFS for installation");
        return false;
    }
    
    File firmware_file = SPIFFS.open(file_path, "r");
    if (!firmware_file) {
        setError("Failed to open firmware file for installation");
        return false;
    }
    
    size_t firmware_size = firmware_file.size();
    
    // Begin OTA update
    if (!Update.begin(firmware_size)) {
        setError("Failed to begin OTA update: " + String(Update.errorString()));
        firmware_file.close();
        return false;
    }
    
    rollback_manager_.setUpdateStatus(Security::RollbackManager::UpdateStatus::INSTALLING, 
                                     "Installing firmware");
    
    // Write firmware data
    size_t written = 0;
    uint8_t buffer[1024];
    
    while (firmware_file.available()) {
        size_t read_bytes = firmware_file.readBytes((char*)buffer, sizeof(buffer));
        size_t bytes_written = Update.write(buffer, read_bytes);
        
        if (bytes_written != read_bytes) {
            setError("Failed to write firmware data: " + String(Update.errorString()));
            firmware_file.close();
            Update.abort();
            return false;
        }
        
        written += bytes_written;
        
        // Report progress
        float percentage = 75.0f + ((float)written / firmware_size * 20.0f); // Map to 75-95% range
        String message = "Written " + String(written) + "/" + String(firmware_size) + " bytes";
        reportProgress(percentage, "Installing", message);
    }
    
    firmware_file.close();
    
    // Finalize update
    if (!Update.end(true)) {
        setError("Failed to finalize OTA update: " + String(Update.errorString()));
        return false;
    }
    
    Logger::info("Firmware installation completed successfully");
    return true;
}

void OTAHandler::reportProgress(float percentage, const String& stage, const String& message) {
    if (progress_callback_) {
        UpdateProgress progress;
        progress.percentage = percentage;
        progress.current_stage = stage;
        progress.message = message;
        progress.elapsed_ms = millis(); // Could track actual start time
        progress_callback_(progress);
    }
    
    Logger::info("OTA Progress: " + String(percentage, 1) + "% - " + stage + 
                (message.isEmpty() ? "" : " (" + message + ")"));
}

void OTAHandler::reportResult(UpdateResult result, const String& message) {
    last_result_ = result;
    
    if (result_callback_) {
        result_callback_(result, message);
    }
    
    String result_str;
    switch (result) {
        case UpdateResult::SUCCESS: result_str = "SUCCESS"; break;
        case UpdateResult::NO_UPDATE: result_str = "NO_UPDATE"; break;
        case UpdateResult::DOWNLOAD_FAILED: result_str = "DOWNLOAD_FAILED"; break;
        case UpdateResult::VALIDATION_FAILED: result_str = "VALIDATION_FAILED"; break;
        case UpdateResult::INSTALLATION_FAILED: result_str = "INSTALLATION_FAILED"; break;
        case UpdateResult::ROLLBACK_TRIGGERED: result_str = "ROLLBACK_TRIGGERED"; break;
        case UpdateResult::NETWORK_ERROR: result_str = "NETWORK_ERROR"; break;
        case UpdateResult::STORAGE_ERROR: result_str = "STORAGE_ERROR"; break;
        case UpdateResult::CONFIGURATION_ERROR: result_str = "CONFIGURATION_ERROR"; break;
        default: result_str = "UNKNOWN"; break;
    }
    
    Logger::info("OTA Result: " + result_str + " - " + message);
    saveState();
}

void OTAHandler::setError(const String& error_message) {
    last_error_ = error_message;
    Logger::error("OTA Error: " + error_message);
}

bool OTAHandler::validateConfiguration(const UpdateConfiguration& config) {
    if (config.repository_owner.isEmpty() || config.repository_name.isEmpty()) {
        setError("Repository owner and name must be specified");
        return false;
    }
    
    if (config.current_version.isEmpty()) {
        setError("Current version must be specified");
        return false;
    }
    
    if (!OTAUtils::isValidVersionFormat(config.current_version)) {
        setError("Invalid version format: " + config.current_version);
        return false;
    }
    
    if (config.download_path.isEmpty()) {
        setError("Download path must be specified");
        return false;
    }
    
    return true;
}

bool OTAHandler::isNewerVersion(const String& available_version, const String& current_version) {
    return OTAUtils::compareVersions(available_version, current_version) > 0;
}

bool OTAHandler::ensureStorageSpace(size_t required_bytes) {
    size_t available = OTAUtils::getAvailableStorage();
    if (available < required_bytes) {
        Logger::error("Insufficient storage: need " + String(required_bytes) + 
                     " bytes, available " + String(available) + " bytes");
        return false;
    }
    return true;
}

void OTAHandler::cleanupTempFiles() {
    if (SPIFFS.exists(config_.download_path)) {
        SPIFFS.remove(config_.download_path);
        Logger::info("Cleaned up temporary firmware file");
    }
}

bool OTAHandler::isNetworkAvailable() {
    return WiFi.status() == WL_CONNECTED;
}

void OTAHandler::saveState() {
    // Implementation would save current state to persistent storage
    Logger::debug("OTA state saved");
}

void OTAHandler::loadState() {
    // Implementation would load state from persistent storage
    Logger::debug("OTA state loaded");
}

// Utility functions implementation
namespace OTAUtils {

bool parseVersion(const String& version_string, int& major, int& minor, int& patch) {
    String version = version_string;
    
    // Remove 'v' prefix if present
    if (version.startsWith("v")) {
        version = version.substring(1);
    }
    
    int first_dot = version.indexOf('.');
    int second_dot = version.indexOf('.', first_dot + 1);
    
    if (first_dot == -1 || second_dot == -1) {
        return false;
    }
    
    major = version.substring(0, first_dot).toInt();
    minor = version.substring(first_dot + 1, second_dot).toInt();
    patch = version.substring(second_dot + 1).toInt();
    
    return true;
}

int compareVersions(const String& version1, const String& version2) {
    int major1, minor1, patch1;
    int major2, minor2, patch2;
    
    if (!parseVersion(version1, major1, minor1, patch1) || 
        !parseVersion(version2, major2, minor2, patch2)) {
        return 0; // Equal if parsing fails
    }
    
    if (major1 != major2) return (major1 > major2) ? 1 : -1;
    if (minor1 != minor2) return (minor1 > minor2) ? 1 : -1;
    if (patch1 != patch2) return (patch1 > patch2) ? 1 : -1;
    
    return 0; // Equal
}

bool isValidVersionFormat(const String& version_string) {
    int major, minor, patch;
    return parseVersion(version_string, major, minor, patch);
}

size_t getAvailableStorage() {
    if (!SPIFFS.begin()) {
        return 0;
    }
    
    return SPIFFS.totalBytes() - SPIFFS.usedBytes();
}

String getBuildInfo() {
    // This would be populated by build system with version, date, commit hash
    return "GarageSensor v1.0.0 (Built: " + String(__DATE__) + " " + String(__TIME__) + ")";
}

} // namespace OTAUtils

} // namespace Network