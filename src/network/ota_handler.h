#pragma once

#include <Arduino.h>
#include <functional>
#include "github_client.h"
#include "../security/integrity_check.h"
#include "../security/rollback_manager.h"

namespace Network {

/**
 * @brief Over-The-Air (OTA) update handler with GitHub integration
 * 
 * Orchestrates secure firmware updates from GitHub releases:
 * - Automatic update checking and scheduling
 * - Secure download with integrity validation
 * - Safe installation with rollback protection
 * - Progress reporting and error handling
 * - Constitutional compliance with reliability-first principle
 */
class OTAHandler {
public:
    enum class UpdateResult {
        SUCCESS,            // Update completed successfully
        NO_UPDATE,          // No update available
        DOWNLOAD_FAILED,    // Download failed
        VALIDATION_FAILED,  // Integrity check failed
        INSTALLATION_FAILED,// Installation failed
        ROLLBACK_TRIGGERED, // Update rolled back
        NETWORK_ERROR,      // Network connectivity issue
        STORAGE_ERROR,      // Storage/filesystem error
        CONFIGURATION_ERROR // Configuration issue
    };

    struct UpdateProgress {
        float percentage;
        String current_stage;
        size_t bytes_downloaded;
        size_t total_bytes;
        unsigned long elapsed_ms;
        String message;
    };

    struct UpdateConfiguration {
        String repository_owner;
        String repository_name;
        String current_version;
        bool auto_check_enabled;
        unsigned long check_interval_ms;
        bool include_prereleases;
        bool require_manual_approval;
        String download_path;
        unsigned long download_timeout_ms;
        unsigned long validation_timeout_ms;
    };

    using ProgressCallback = std::function<void(const UpdateProgress&)>;
    using ResultCallback = std::function<void(UpdateResult, const String&)>;
    using ApprovalCallback = std::function<bool(const GitHubClient::ReleaseInfo&)>;

    OTAHandler();
    ~OTAHandler();

    /**
     * @brief Initialize OTA handler with configuration
     * @param config Update configuration
     * @return true if initialization successful
     */
    bool begin(const UpdateConfiguration& config);

    /**
     * @brief Check for available updates
     * @param release_info Output release information if update available
     * @return true if update is available
     */
    bool checkForUpdates(GitHubClient::ReleaseInfo& release_info);

    /**
     * @brief Perform firmware update 
     * @param release_info Release information for update
     * @return Update result status
     */
    UpdateResult performUpdate(const GitHubClient::ReleaseInfo& release_info);

    /**
     * @brief Start automatic update process
     * @param progress_callback Optional progress reporting
     * @param result_callback Optional result notification
     * @return true if update process started successfully
     */
    bool startUpdate(ProgressCallback progress_callback = nullptr, 
                    ResultCallback result_callback = nullptr);

    /**
     * @brief Start update with specific release
     * @param release_info Release to update to
     * @param progress_callback Optional progress reporting
     * @param result_callback Optional result notification
     * @return true if update process started successfully
     */
    bool startUpdate(const GitHubClient::ReleaseInfo& release_info,
                    ProgressCallback progress_callback = nullptr,
                    ResultCallback result_callback = nullptr);

    /**
     * @brief Enable automatic update checking
     * @param enabled Enable/disable automatic checking
     */
    void setAutoCheckEnabled(bool enabled) { config_.auto_check_enabled = enabled; }

    /**
     * @brief Set update approval callback for manual approval mode
     * @param callback Approval callback function
     */
    void setApprovalCallback(ApprovalCallback callback) { approval_callback_ = callback; }

    /**
     * @brief Process periodic update checks (call from main loop)
     */
    void processPeriodicCheck();

    /**
     * @brief Cancel ongoing update
     * @return true if cancellation successful
     */
    bool cancelUpdate();

    /**
     * @brief Get current update configuration
     */
    UpdateConfiguration getConfiguration() const { return config_; }

    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void updateConfiguration(const UpdateConfiguration& config);

    /**
     * @brief Check if update is in progress
     */
    bool isUpdateInProgress() const;

    /**
     * @brief Get last update result
     */
    UpdateResult getLastResult() const { return last_result_; }

    /**
     * @brief Get last error message
     */
    String getLastError() const { return last_error_; }

    /**
     * @brief Get GitHub client instance
     */
    GitHubClient& getGitHubClient() { return github_client_; }

    /**
     * @brief Get integrity checker instance
     */
    Security::IntegrityCheck& getIntegrityChecker() { return integrity_checker_; }

    /**
     * @brief Get rollback manager instance
     */
    Security::RollbackManager& getRollbackManager() { return rollback_manager_; }

private:
    UpdateConfiguration config_;
    GitHubClient github_client_;
    Security::IntegrityCheck integrity_checker_;
    Security::RollbackManager rollback_manager_;
    
    ProgressCallback progress_callback_;
    ResultCallback result_callback_;
    ApprovalCallback approval_callback_;
    
    UpdateResult last_result_;
    String last_error_;
    unsigned long last_check_time_;
    bool update_in_progress_;
    
    // Update process stages
    bool downloadFirmware(const GitHubClient::ReleaseInfo& release_info);
    bool validateFirmware(const String& file_path, const String& expected_hash);
    bool installFirmware(const String& file_path);
    bool verifyInstallation();
    
    // Progress and error reporting
    void reportProgress(float percentage, const String& stage, const String& message = "");
    void reportResult(UpdateResult result, const String& message);
    void setError(const String& error_message);
    
    // Configuration validation
    bool validateConfiguration(const UpdateConfiguration& config);
    
    // Version comparison
    bool isNewerVersion(const String& available_version, const String& current_version);
    
    // Storage management
    bool ensureStorageSpace(size_t required_bytes);
    void cleanupTempFiles();
    
    // Network status
    bool isNetworkAvailable();
    
    // State management
    void saveState();
    void loadState();
};

/**
 * @brief Global OTA handler instance
 */
extern OTAHandler& GlobalOTAHandler();

/**
 * @brief Utility functions for OTA operations
 */
namespace OTAUtils {
    /**
     * @brief Parse semantic version string
     * @param version_string Version in format "v1.2.3" or "1.2.3"
     * @param major Output major version
     * @param minor Output minor version  
     * @param patch Output patch version
     * @return true if parsing successful
     */
    bool parseVersion(const String& version_string, int& major, int& minor, int& patch);
    
    /**
     * @brief Compare two semantic versions
     * @param version1 First version
     * @param version2 Second version
     * @return -1 if version1 < version2, 0 if equal, 1 if version1 > version2
     */
    int compareVersions(const String& version1, const String& version2);
    
    /**
     * @brief Check if version string format is valid
     * @param version_string Version to validate
     * @return true if format is valid
     */
    bool isValidVersionFormat(const String& version_string);
    
    /**
     * @brief Get available storage space
     * @return Available bytes in SPIFFS
     */
    size_t getAvailableStorage();
    
    /**
     * @brief Get current firmware build information
     * @return Build info string (version, date, commit)
     */
    String getBuildInfo();
}

} // namespace Network