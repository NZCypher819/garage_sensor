#pragma once

#include <Arduino.h>
#include <SPIFFS.h>
#include <functional>

namespace Security {

/**
 * @brief File integrity validation using SHA256 hashing
 * 
 * Provides cryptographic integrity verification for firmware updates:
 * - SHA256 hash calculation for files
 * - Hash verification against known values
 * - Progress reporting for large file operations
 * - Memory-efficient streaming hash calculation
 */
class IntegrityCheck {
public:
    struct HashProgress {
        size_t processed_bytes;
        size_t total_bytes;
        float percentage;
        unsigned long elapsed_ms;
    };

    using ProgressCallback = std::function<void(const HashProgress&)>;

    IntegrityCheck();
    ~IntegrityCheck();

    /**
     * @brief Calculate SHA256 hash of a file
     * @param file_path Path to file in SPIFFS
     * @param hash_output 64-character hex string output (must be pre-allocated)
     * @param progress_callback Optional progress reporting callback
     * @return true if hash calculation successful
     */
    bool calculateFileHash(const String& file_path, 
                          String& hash_output,
                          ProgressCallback progress_callback = nullptr);

    /**
     * @brief Verify file integrity against known hash
     * @param file_path Path to file in SPIFFS
     * @param expected_hash 64-character hex SHA256 hash
     * @param progress_callback Optional progress reporting callback
     * @return true if file integrity verified
     */
    bool verifyFileIntegrity(const String& file_path,
                            const String& expected_hash,
                            ProgressCallback progress_callback = nullptr);

    /**
     * @brief Calculate SHA256 hash of memory buffer
     * @param data Buffer to hash
     * @param length Size of buffer
     * @param hash_output 64-character hex string output
     * @return true if hash calculation successful
     */
    bool calculateBufferHash(const uint8_t* data, size_t length, String& hash_output);

    /**
     * @brief Verify hash string format (64 hex characters)
     * @param hash_string Hash to validate
     * @return true if format is valid
     */
    bool isValidHashFormat(const String& hash_string);

    /**
     * @brief Get last error message
     */
    String getLastError() const { return last_error_; }

    /**
     * @brief Set buffer size for streaming operations (default: 1024 bytes)
     */
    void setBufferSize(size_t buffer_size) { buffer_size_ = buffer_size; }

private:
    String last_error_;
    size_t buffer_size_;

    void initSHA256();
    void updateSHA256(const uint8_t* data, size_t length);
    void finalizeSHA256(uint8_t* hash);
    void hashToHexString(const uint8_t* hash, String& hex_string);
    bool hexStringToBytes(const String& hex_string, uint8_t* bytes, size_t max_bytes);

    // SHA256 context - platform specific implementation
    void* sha256_context_;
};

/**
 * @brief Utility functions for integrity checking
 */
namespace IntegrityUtils {
    /**
     * @brief Compare two hash strings (constant time)
     * @param hash1 First hash
     * @param hash2 Second hash  
     * @return true if hashes match
     */
    bool compareHashes(const String& hash1, const String& hash2);

    /**
     * @brief Generate integrity manifest for multiple files
     * @param file_paths List of file paths
     * @param manifest_path Output manifest file path
     * @return true if manifest generation successful
     */
    bool generateManifest(const std::vector<String>& file_paths, const String& manifest_path);

    /**
     * @brief Verify integrity manifest
     * @param manifest_path Path to manifest file
     * @return true if all files in manifest verify correctly
     */
    bool verifyManifest(const String& manifest_path);
}

} // namespace Security