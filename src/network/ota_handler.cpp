#include "ota_handler.h"

namespace Network {

OTAHandler::OTAHandler() {
    Serial.println("OTAHandler: Created");
}

bool OTAHandler::begin(const UpdateConfiguration& config) {
    Serial.println("OTAHandler: Initialized");
    return true;
}

bool OTAHandler::checkForUpdates(ReleaseInfo& release_info) {
    Serial.println("OTAHandler: Checking for updates");
    return false; // No updates available
}

UpdateResult OTAHandler::performUpdate(const ReleaseInfo& release_info) {
    Serial.println("OTAHandler: Would perform update");
    return UpdateResult::SUCCESS;
}

} // namespace Network