#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <functional>

namespace Security {

/**
 * @brief Firmware rollback management for OTA update safety
 * 
 * Provides automatic rollback capabilities to ensure system reliability:
 * - Backup current firmware before updates
 * - Boot failure detection and automatic rollback
 * - Update attempt tracking and failure limits
 * - Safe update validation and commit process
 */
class RollbackManager {
public:
    enum class UpdateStatus {
        NONE,           // No update in progress
        DOWNLOADING,    // Downloading new firmware
        VALIDATING,     // Validating downloaded firmware
        INSTALLING,     // Installing new firmware
        PENDING,        // Update installed, pending validation
        VALIDATED,      // Update validated successfully
        COMMITTED,      // Update permanently committed
        FAILED,         // Update failed
        ROLLED_BACK     // Rolled back to previous version
    };

    enum class BootStatus {
        NORMAL,         // Normal boot
        FIRST_BOOT,     // First boot after update
        FAILED_BOOT,    // Boot failure detected
        ROLLBACK_BOOT   // Boot after rollback
    };

    struct UpdateInfo {
        String current_version;
        String target_version;
        String backup_path;
        UpdateStatus status;
        unsigned long start_time;
        int attempt_count;
        String last_error;
        bool rollback_available;
    };

    struct BootInfo {
        BootStatus status;
        String version;
        unsigned long boot_time;
        int consecutive_failures;
        bool rollback_triggered;
    };

    using StatusCallback = std::function<void(UpdateStatus, const String&)>;

    RollbackManager();
    ~RollbackManager();

    /**
     * @brief Initialize rollback manager
     * @param current_version Current firmware version
     * @return true if initialization successful
     */
    bool begin(const String& current_version);

    /**
     * @brief Check boot status and handle rollback if needed
     * @param boot_info Output boot information
     * @return true if boot is valid, false if rollback needed
     */
    bool checkBootStatus(BootInfo& boot_info);

    /**
     * @brief Start new update process
     * @param target_version Version being updated to
     * @param status_callback Optional status update callback
     * @return true if update process started successfully
     */
    bool startUpdate(const String& target_version, StatusCallback status_callback = nullptr);

    /**
     * @brief Create backup of current firmware
     * @return true if backup created successfully
     */
    bool createBackup();

    /**
     * @brief Set update status
     * @param status New status
     * @param message Optional status message
     */
    void setUpdateStatus(UpdateStatus status, const String& message = "");

    /**
     * @brief Validate update after installation
     * @param validation_timeout_ms Time to wait for validation (default: 60000ms)
     * @return true if update validated successfully
     */
    bool validateUpdate(unsigned long validation_timeout_ms = 60000);

    /**
     * @brief Commit update permanently (disable rollback)
     * @return true if update committed successfully
     */
    bool commitUpdate();

    /**
     * @brief Trigger rollback to previous version
     * @param reason Reason for rollback
     * @return true if rollback initiated successfully
     */
    bool triggerRollback(const String& reason);

    /**
     * @brief Perform actual rollback operation
     * @return true if rollback successful
     */
    bool performRollback();

    /**
     * @brief Clean up old backup files
     * @param keep_count Number of backups to keep (default: 2)
     * @return true if cleanup successful
     */
    bool cleanupBackups(int keep_count = 2);

    /**
     * @brief Get current update information
     */
    UpdateInfo getUpdateInfo() const { return update_info_; }

    /**
     * @brief Get current boot information
     */
    BootInfo getBootInfo() const { return boot_info_; }

    /**
     * @brief Check if rollback is available
     */
    bool isRollbackAvailable() const { return update_info_.rollback_available; }

    /**
     * @brief Get maximum consecutive boot failures before rollback (default: 3)
     */
    int getMaxBootFailures() const { return max_boot_failures_; }

    /**
     * @brief Set maximum consecutive boot failures before rollback
     */
    void setMaxBootFailures(int max_failures) { max_boot_failures_ = max_failures; }

    /**
     * @brief Get boot validation timeout (default: 30000ms)
     */
    unsigned long getBootTimeout() const { return boot_timeout_ms_; }

    /**
     * @brief Set boot validation timeout
     */
    void setBootTimeout(unsigned long timeout_ms) { boot_timeout_ms_ = timeout_ms; }

private:
    UpdateInfo update_info_;
    BootInfo boot_info_;
    StatusCallback status_callback_;
    int max_boot_failures_;
    unsigned long boot_timeout_ms_;
    String state_file_path_;
    String backup_dir_;

    bool loadState();
    bool saveState();
    bool initializeState();
    bool checkFirstBoot();
    bool markBootSuccess();
    bool markBootFailure();
    bool isBootTimeoutExpired();
    String generateBackupPath(const String& version);
    bool restoreFromBackup(const String& backup_path);
    void notifyStatus(UpdateStatus status, const String& message);
    void logState(const String& operation);

    // File system operations
    bool createDirectory(const String& path);
    bool fileExists(const String& path);
    bool deleteFile(const String& path);
    bool copyFile(const String& source, const String& destination);
    size_t getFileSize(const String& path);
};

/**
 * @brief Global rollback manager instance
 */
extern RollbackManager& GlobalRollbackManager();

/**
 * @brief Utility functions for rollback management
 */
namespace RollbackUtils {
    /**
     * @brief Install watchdog timer for boot failure detection
     * @param timeout_seconds Watchdog timeout in seconds
     * @return true if watchdog installed successfully
     */
    bool installBootWatchdog(int timeout_seconds = 30);

    /**
     * @brief Feed watchdog to prevent timeout
     */
    void feedBootWatchdog();

    /**
     * @brief Disable watchdog after successful boot validation
     */
    void disableBootWatchdog();

    /**
     * @brief Check if system is in a boot loop
     * @return true if boot loop detected
     */
    bool isBootLoopDetected();

    /**
     * @brief Reset boot failure counter
     */
    void resetBootFailureCounter();
}

} // namespace Security