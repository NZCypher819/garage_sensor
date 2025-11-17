#pragma once

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <functional>

namespace Network {

/**
 * @brief GitHub API client with HTTPS certificate validation
 * 
 * Provides secure communication with GitHub API for:
 * - Fetching latest release information
 * - Downloading firmware binaries
 * - Validating SSL certificates
 * - Progress reporting during downloads
 */
class GitHubClient {
public:
    struct ReleaseInfo {
        String tag_name;
        String download_url;
        String sha256_hash;
        size_t file_size;
        bool prerelease;
        String published_at;
    };

    struct DownloadProgress {
        size_t downloaded_bytes;
        size_t total_bytes;
        float percentage;
        unsigned long elapsed_ms;
    };

    using ProgressCallback = std::function<void(const DownloadProgress&)>;
    using ErrorCallback = std::function<void(const String&)>;

    GitHubClient();
    ~GitHubClient();

    /**
     * @brief Initialize GitHub client with repository information
     * @param owner Repository owner (e.g., "myuser")
     * @param repo Repository name (e.g., "garage-sensor")
     * @return true if initialization successful
     */
    bool begin(const String& owner, const String& repo);

    /**
     * @brief Fetch latest release information from GitHub
     * @param release_info Output structure for release details
     * @param include_prereleases Include pre-release versions
     * @return true if fetch successful
     */
    bool getLatestRelease(ReleaseInfo& release_info, bool include_prereleases = false);

    /**
     * @brief Download firmware file with progress reporting
     * @param download_url URL to download from
     * @param destination_path Local file path to save to
     * @param progress_callback Optional progress reporting callback
     * @param error_callback Optional error reporting callback
     * @return true if download successful
     */
    bool downloadFirmware(const String& download_url, 
                         const String& destination_path,
                         ProgressCallback progress_callback = nullptr,
                         ErrorCallback error_callback = nullptr);

    /**
     * @brief Validate SSL certificate for GitHub
     * @return true if certificate validation successful
     */
    bool validateCertificate();

    /**
     * @brief Get last HTTP response code
     */
    int getLastHttpCode() const { return last_http_code_; }

    /**
     * @brief Get last error message
     */
    String getLastError() const { return last_error_; }

    /**
     * @brief Set connection timeout (default: 30000ms)
     */
    void setTimeout(unsigned long timeout_ms) { timeout_ms_ = timeout_ms; }

    /**
     * @brief Set maximum download size (default: 2MB)
     */
    void setMaxDownloadSize(size_t max_size) { max_download_size_ = max_size; }

private:
    WiFiClientSecure wifi_client_;
    HTTPClient http_client_;
    String owner_;
    String repo_;
    String base_url_;
    unsigned long timeout_ms_;
    size_t max_download_size_;
    int last_http_code_;
    String last_error_;

    // GitHub API root certificate (GitHub's CA)
    static const char* github_root_cert_;

    bool makeHttpRequest(const String& url, String& response);
    bool parseReleaseJson(const String& json_response, ReleaseInfo& release_info, bool include_prereleases);
    String buildApiUrl(const String& endpoint);
    void logHttpError(const String& operation, int http_code);
};

} // namespace Network