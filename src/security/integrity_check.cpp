#include "integrity_check.h"
#include "../diagnostics/logger.h"
#include <mbedtls/sha256.h>
#include <ArduinoJson.h>

namespace Security {

IntegrityCheck::IntegrityCheck() : buffer_size_(1024) {
    sha256_context_ = malloc(sizeof(mbedtls_sha256_context));
    if (!sha256_context_) {
        last_error_ = "Failed to allocate SHA256 context";
    }
}

IntegrityCheck::~IntegrityCheck() {
    if (sha256_context_) {
        free(sha256_context_);
    }
}

bool IntegrityCheck::calculateFileHash(const String& file_path, 
                                      String& hash_output,
                                      ProgressCallback progress_callback) {
    if (!SPIFFS.begin()) {
        last_error_ = "Failed to initialize SPIFFS";
        return false;
    }
    
    File file = SPIFFS.open(file_path, "r");
    if (!file) {
        last_error_ = "Failed to open file: " + file_path;
        return false;
    }
    
    size_t file_size = file.size();
    size_t processed = 0;
    unsigned long start_time = millis();
    
    initSHA256();
    
    uint8_t* buffer = (uint8_t*)malloc(buffer_size_);
    if (!buffer) {
        last_error_ = "Failed to allocate buffer";
        file.close();
        return false;
    }
    
    Logger::info("Calculating SHA256 hash for: " + file_path + " (" + String(file_size) + " bytes)");
    
    // Process file in chunks
    while (file.available()) {
        size_t read_bytes = file.readBytes((char*)buffer, buffer_size_);
        updateSHA256(buffer, read_bytes);
        processed += read_bytes;
        
        if (progress_callback) {
            HashProgress progress;
            progress.processed_bytes = processed;
            progress.total_bytes = file_size;
            progress.percentage = (float)processed / file_size * 100.0f;
            progress.elapsed_ms = millis() - start_time;
            progress_callback(progress);
        }
    }
    
    file.close();
    free(buffer);
    
    // Finalize hash
    uint8_t hash[32];
    finalizeSHA256(hash);
    hashToHexString(hash, hash_output);
    
    Logger::info("SHA256 hash calculated: " + hash_output);
    return true;
}

bool IntegrityCheck::verifyFileIntegrity(const String& file_path,
                                        const String& expected_hash,
                                        ProgressCallback progress_callback) {
    if (!isValidHashFormat(expected_hash)) {
        last_error_ = "Invalid hash format";
        return false;
    }
    
    String calculated_hash;
    if (!calculateFileHash(file_path, calculated_hash, progress_callback)) {
        return false;
    }
    
    bool verified = IntegrityUtils::compareHashes(calculated_hash, expected_hash);
    if (verified) {
        Logger::info("File integrity verified: " + file_path);
    } else {
        last_error_ = "Hash mismatch - expected: " + expected_hash + ", got: " + calculated_hash;
        Logger::error("File integrity check FAILED: " + file_path);
        Logger::error("Expected: " + expected_hash);
        Logger::error("Actual:   " + calculated_hash);
    }
    
    return verified;
}

bool IntegrityCheck::calculateBufferHash(const uint8_t* data, size_t length, String& hash_output) {
    if (!data || length == 0) {
        last_error_ = "Invalid buffer or length";
        return false;
    }
    
    initSHA256();
    updateSHA256(data, length);
    
    uint8_t hash[32];
    finalizeSHA256(hash);
    hashToHexString(hash, hash_output);
    
    return true;
}

bool IntegrityCheck::isValidHashFormat(const String& hash_string) {
    if (hash_string.length() != 64) {
        return false;
    }
    
    for (int i = 0; i < hash_string.length(); i++) {
        char c = hash_string.charAt(i);
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    
    return true;
}

void IntegrityCheck::initSHA256() {
    mbedtls_sha256_context* ctx = (mbedtls_sha256_context*)sha256_context_;
    mbedtls_sha256_init(ctx);
    mbedtls_sha256_starts(ctx, 0); // 0 for SHA256 (not SHA224)
}

void IntegrityCheck::updateSHA256(const uint8_t* data, size_t length) {
    mbedtls_sha256_context* ctx = (mbedtls_sha256_context*)sha256_context_;
    mbedtls_sha256_update(ctx, data, length);
}

void IntegrityCheck::finalizeSHA256(uint8_t* hash) {
    mbedtls_sha256_context* ctx = (mbedtls_sha256_context*)sha256_context_;
    mbedtls_sha256_finish(ctx, hash);
    mbedtls_sha256_free(ctx);
}

void IntegrityCheck::hashToHexString(const uint8_t* hash, String& hex_string) {
    hex_string = "";
    for (int i = 0; i < 32; i++) {
        if (hash[i] < 16) {
            hex_string += "0";
        }
        hex_string += String(hash[i], HEX);
    }
    hex_string.toLowerCase();
}

bool IntegrityCheck::hexStringToBytes(const String& hex_string, uint8_t* bytes, size_t max_bytes) {
    if (hex_string.length() % 2 != 0 || hex_string.length() / 2 > max_bytes) {
        return false;
    }
    
    for (size_t i = 0; i < hex_string.length(); i += 2) {
        String byte_string = hex_string.substring(i, i + 2);
        bytes[i / 2] = (uint8_t)strtol(byte_string.c_str(), NULL, 16);
    }
    
    return true;
}

namespace IntegrityUtils {

bool compareHashes(const String& hash1, const String& hash2) {
    if (hash1.length() != hash2.length() || hash1.length() != 64) {
        return false;
    }
    
    // Constant-time comparison to prevent timing attacks
    uint8_t result = 0;
    for (int i = 0; i < hash1.length(); i++) {
        result |= (hash1.charAt(i) ^ hash2.charAt(i));
    }
    
    return result == 0;
}

bool generateManifest(const std::vector<String>& file_paths, const String& manifest_path) {
    if (!SPIFFS.begin()) {
        return false;
    }
    
    JsonDocument manifest;
    JsonArray files = manifest["files"].to<JsonArray>();
    
    IntegrityCheck checker;
    for (const String& file_path : file_paths) {
        String hash;
        if (!checker.calculateFileHash(file_path, hash)) {
            Logger::error("Failed to calculate hash for: " + file_path);
            return false;
        }
        
        JsonObject file_entry = files.add<JsonObject>();
        file_entry["path"] = file_path;
        file_entry["sha256"] = hash;
        
        File file = SPIFFS.open(file_path, "r");
        if (file) {
            file_entry["size"] = file.size();
            file.close();
        }
    }
    
    manifest["generated_at"] = millis();
    manifest["version"] = "1.0";
    
    File manifest_file = SPIFFS.open(manifest_path, "w");
    if (!manifest_file) {
        Logger::error("Failed to create manifest file: " + manifest_path);
        return false;
    }
    
    if (serializeJson(manifest, manifest_file) == 0) {
        manifest_file.close();
        return false;
    }
    
    manifest_file.close();
    Logger::info("Generated integrity manifest: " + manifest_path);
    return true;
}

bool verifyManifest(const String& manifest_path) {
    if (!SPIFFS.begin()) {
        return false;
    }
    
    File manifest_file = SPIFFS.open(manifest_path, "r");
    if (!manifest_file) {
        Logger::error("Failed to open manifest file: " + manifest_path);
        return false;
    }
    
    JsonDocument manifest;
    DeserializationError error = deserializeJson(manifest, manifest_file);
    manifest_file.close();
    
    if (error) {
        Logger::error("Failed to parse manifest JSON: " + String(error.c_str()));
        return false;
    }
    
    JsonArray files = manifest["files"];
    IntegrityCheck checker;
    
    for (JsonObject file_entry : files) {
        String file_path = file_entry["path"];
        String expected_hash = file_entry["sha256"];
        
        if (!checker.verifyFileIntegrity(file_path, expected_hash)) {
            Logger::error("Manifest verification FAILED for: " + file_path);
            return false;
        }
    }
    
    Logger::info("Manifest verification PASSED: " + String(files.size()) + " files");
    return true;
}

} // namespace IntegrityUtils

} // namespace Security