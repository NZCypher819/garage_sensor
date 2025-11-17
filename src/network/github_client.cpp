#include "github_client.h"
#include <SPIFFS.h>
#include "../diagnostics/logger.h"

namespace Network {

// GitHub's root certificate (DigiCert Global Root CA)
const char* GitHubClient::github_root_cert_ = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)EOF";

GitHubClient::GitHubClient() 
    : timeout_ms_(30000), max_download_size_(2 * 1024 * 1024), last_http_code_(0) {
}

GitHubClient::~GitHubClient() {
    http_client_.end();
}

bool GitHubClient::begin(const String& owner, const String& repo) {
    owner_ = owner;
    repo_ = repo;
    base_url_ = "https://api.github.com/repos/" + owner + "/" + repo;
    
    // Configure WiFi client with certificate
    wifi_client_.setCACert(github_root_cert_);
    wifi_client_.setTimeout(timeout_ms_ / 1000);
    
    // Configure HTTP client
    http_client_.setTimeout(timeout_ms_);
    
    Logger::info("GitHubClient initialized for " + owner + "/" + repo);
    return true;
}

bool GitHubClient::getLatestRelease(ReleaseInfo& release_info, bool include_prereleases) {
    String endpoint = include_prereleases ? "/releases" : "/releases/latest";
    String url = buildApiUrl(endpoint);
    
    String response;
    if (!makeHttpRequest(url, response)) {
        return false;
    }
    
    return parseReleaseJson(response, release_info, include_prereleases);
}

bool GitHubClient::downloadFirmware(const String& download_url, 
                                   const String& destination_path,
                                   ProgressCallback progress_callback,
                                   ErrorCallback error_callback) {
    
    Logger::info("Starting firmware download from: " + download_url);
    
    wifi_client_.setCACert(github_root_cert_);
    http_client_.begin(wifi_client_, download_url);
    http_client_.setTimeout(timeout_ms_);
    
    int http_code = http_client_.GET();
    if (http_code != HTTP_CODE_OK) {
        last_http_code_ = http_code;
        last_error_ = "Download failed with HTTP code: " + String(http_code);
        if (error_callback) {
            error_callback(last_error_);
        }
        http_client_.end();
        return false;
    }
    
    size_t content_length = http_client_.getSize();
    if (content_length > max_download_size_) {
        last_error_ = "File too large: " + String(content_length) + " bytes (max: " + String(max_download_size_) + ")";
        if (error_callback) {
            error_callback(last_error_);
        }
        http_client_.end();
        return false;
    }
    
    // Open file for writing
    if (!SPIFFS.begin()) {
        last_error_ = "Failed to initialize SPIFFS";
        if (error_callback) {
            error_callback(last_error_);
        }
        http_client_.end();
        return false;
    }
    
    File file = SPIFFS.open(destination_path, "w");
    if (!file) {
        last_error_ = "Failed to open file for writing: " + destination_path;
        if (error_callback) {
            error_callback(last_error_);
        }
        http_client_.end();
        return false;
    }
    
    // Download with progress reporting
    WiFiClient* stream = http_client_.getStreamPtr();
    size_t downloaded = 0;
    unsigned long start_time = millis();
    uint8_t buffer[1024];
    
    while (http_client_.connected() && (content_length > 0 || content_length == -1)) {
        size_t available = stream->available();
        if (available) {
            size_t read_bytes = stream->readBytes(buffer, min(available, sizeof(buffer)));
            file.write(buffer, read_bytes);
            downloaded += read_bytes;
            
            if (progress_callback && content_length > 0) {
                DownloadProgress progress;
                progress.downloaded_bytes = downloaded;
                progress.total_bytes = content_length;
                progress.percentage = (float)downloaded / content_length * 100.0f;
                progress.elapsed_ms = millis() - start_time;
                progress_callback(progress);
            }
            
            if (content_length > 0 && downloaded >= content_length) {
                break;
            }
        }
        delay(1);
    }
    
    file.close();
    http_client_.end();
    
    if (content_length > 0 && downloaded != content_length) {
        last_error_ = "Download incomplete: " + String(downloaded) + "/" + String(content_length) + " bytes";
        if (error_callback) {
            error_callback(last_error_);
        }
        return false;
    }
    
    Logger::info("Firmware download completed: " + String(downloaded) + " bytes in " + String(millis() - start_time) + "ms");
    return true;
}

bool GitHubClient::validateCertificate() {
    wifi_client_.setCACert(github_root_cert_);
    
    if (!wifi_client_.connect("api.github.com", 443)) {
        last_error_ = "Failed to connect to GitHub for certificate validation";
        return false;
    }
    
    // Connection successful means certificate was validated
    wifi_client_.stop();
    Logger::info("GitHub certificate validation successful");
    return true;
}

bool GitHubClient::makeHttpRequest(const String& url, String& response) {
    wifi_client_.setCACert(github_root_cert_);
    http_client_.begin(wifi_client_, url);
    http_client_.setTimeout(timeout_ms_);
    http_client_.addHeader("User-Agent", "GarageSensor/1.0");
    http_client_.addHeader("Accept", "application/vnd.github.v3+json");
    
    int http_code = http_client_.GET();
    last_http_code_ = http_code;
    
    if (http_code == HTTP_CODE_OK) {
        response = http_client_.getString();
        http_client_.end();
        return true;
    } else {
        last_error_ = "HTTP request failed with code: " + String(http_code);
        logHttpError("API request", http_code);
        http_client_.end();
        return false;
    }
}

bool GitHubClient::parseReleaseJson(const String& json_response, ReleaseInfo& release_info, bool include_prereleases) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json_response);
    
    if (error) {
        last_error_ = "JSON parsing failed: " + String(error.c_str());
        return false;
    }
    
    JsonObject release;
    if (include_prereleases) {
        // Find first non-prerelease or first release
        JsonArray releases = doc.as<JsonArray>();
        for (JsonObject r : releases) {
            if (!include_prereleases && r["prerelease"].as<bool>()) {
                continue;
            }
            release = r;
            break;
        }
        if (release.isNull()) {
            last_error_ = "No suitable release found";
            return false;
        }
    } else {
        release = doc.as<JsonObject>();
    }
    
    // Extract release information
    release_info.tag_name = release["tag_name"].as<String>();
    release_info.prerelease = release["prerelease"].as<bool>();
    release_info.published_at = release["published_at"].as<String>();
    
    // Find firmware binary asset
    JsonArray assets = release["assets"];
    for (JsonObject asset : assets) {
        String name = asset["name"].as<String>();
        if (name.endsWith(".bin") || name.endsWith(".firmware")) {
            release_info.download_url = asset["browser_download_url"].as<String>();
            release_info.file_size = asset["size"].as<size_t>();
            
            // Look for SHA256 in release body or asset name
            String body = release["body"].as<String>();
            int sha_start = body.indexOf("sha256:");
            if (sha_start == -1) {
                sha_start = body.indexOf("SHA256:");
            }
            if (sha_start != -1) {
                sha_start += 7; // Skip "sha256:"
                release_info.sha256_hash = body.substring(sha_start, sha_start + 64);
                release_info.sha256_hash.trim();
            }
            break;
        }
    }
    
    if (release_info.download_url.isEmpty()) {
        last_error_ = "No firmware binary found in release assets";
        return false;
    }
    
    Logger::info("Parsed release: " + release_info.tag_name + " (" + String(release_info.file_size) + " bytes)");
    return true;
}

String GitHubClient::buildApiUrl(const String& endpoint) {
    return base_url_ + endpoint;
}

void GitHubClient::logHttpError(const String& operation, int http_code) {
    String error_msg = "HTTP " + operation + " failed with code " + String(http_code);
    switch (http_code) {
        case 404:
            error_msg += " (Not Found)";
            break;
        case 403:
            error_msg += " (Forbidden - rate limited?)";
            break;
        case 401:
            error_msg += " (Unauthorized)";
            break;
        default:
            break;
    }
    Logger::error(error_msg);
}

} // namespace Network