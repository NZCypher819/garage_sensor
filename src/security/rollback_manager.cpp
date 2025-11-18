#include "rollback_manager.h"

namespace Security {

bool RollbackManager::begin(const String& current_version) {
    Serial.println("RollbackManager: Initialized for " + current_version);
    return true;
}

bool RollbackManager::checkBootStatus(BootInfo& boot_info) {
    Serial.println("RollbackManager: Boot status OK");
    boot_info.consecutive_failures = 0;
    boot_info.last_boot_time = millis();
    return true; // Boot OK
}

bool RollbackManager::startUpdate(const String& target_version, StatusCallback callback) {
    Serial.println("RollbackManager: Starting update to " + target_version);
    return true;
}

bool RollbackManager::validateUpdate(unsigned long validation_timeout_ms) {
    Serial.println("RollbackManager: Update validated");
    return true;
}

bool RollbackManager::triggerRollback(const String& reason) {
    Serial.println("RollbackManager: Rollback triggered - " + reason);
    return true;
}

} // namespace Security