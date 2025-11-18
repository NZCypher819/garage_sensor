#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

namespace Config {

/**
 * @brief Firmware version management and tracking
 * 
 * Provides comprehensive version tracking for the garage sensor:
 * - Build information management  
 * - Version history tracking
 * - Update status monitoring
 * - Constitutional compliance tracking
 */
class VersionManager {
public:
    struct BuildInfo {
        String version;
        String build_date;
        String build_time;
        String git_commit;
        String git_branch;
        String compiler_version;
        String platform;
        bool debug_build;
    };

    struct VersionHistory {
        String version;
        String install_date;
        String install_reason;
        bool successful;
        String rollback_reason;
    };

    VersionManager();
    ~VersionManager();

    /**
     * @brief Initialize version manager
     * @return true if initialization successful
     */
    bool begin();

    /**
     * @brief Get current build information
     */
    BuildInfo getBuildInfo() const { return build_info_; }

    /**
     * @brief Get current version string
     */
    String getCurrentVersion() const { return build_info_.version; }

    /**
     * @brief Get version history
     */
    std::vector<VersionHistory> getVersionHistory() const { return version_history_; }

    /**
     * @brief Record new version installation
     * @param version Version being installed
     * @param reason Reason for installation (e.g., "OTA update", "factory install")
     */
    void recordVersionInstall(const String& version, const String& reason);

    /**
     * @brief Mark current version as successful
     */
    void markVersionSuccessful();

    /**
     * @brief Record version rollback
     * @param from_version Version rolled back from
     * @param to_version Version rolled back to  
     * @param reason Reason for rollback
     */
    void recordVersionRollback(const String& from_version, const String& to_version, const String& reason);

    /**
     * @brief Get firmware uptime since current version install
     * @return Uptime in milliseconds
     */
    unsigned long getVersionUptime() const;

    /**
     * @brief Check if this is first boot of current version
     */
    bool isFirstBoot() const;

    /**
     * @brief Get total number of version updates
     */
    int getTotalUpdates() const { return version_history_.size(); }

    /**
     * @brief Get number of successful updates
     */
    int getSuccessfulUpdates() const;

    /**
     * @brief Get number of rollbacks
     */
    int getRollbackCount() const;

    /**
     * @brief Export version information to JSON
     * @param include_history Include full version history
     */
    String exportToJson(bool include_history = false) const;

    /**
     * @brief Generate status report for diagnostics
     */
    String generateStatusReport() const;

private:
    BuildInfo build_info_;
    std::vector<VersionHistory> version_history_;
    String version_file_path_;
    unsigned long version_install_time_;
    bool first_boot_;

    void initializeBuildInfo();
    bool loadVersionHistory();
    bool saveVersionHistory();
    void addVersionEntry(const VersionHistory& entry);
    String formatTimestamp(unsigned long timestamp) const;
};

/**
 * @brief Global version manager instance
 */
extern VersionManager& GlobalVersionManager();

/**
 * @brief Compile-time build information (populated by build system)
 */
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "1.0.0"
#endif

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "unknown"
#endif

#ifndef GIT_BRANCH_NAME
#define GIT_BRANCH_NAME "unknown"
#endif

} // namespace Config