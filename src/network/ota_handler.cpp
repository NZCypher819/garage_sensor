#include "ota_handler.h"

namespace Network {

OTAHandler::OTAHandler() {
    Serial.println("OTAHandler: Created");
}

OTAHandler::~OTAHandler() {
    // Destructor
}

bool OTAHandler::begin(const UpdateConfiguration& config) {
    Serial.println("OTAHandler: Initialized");
    return true;
}

bool OTAHandler::checkForUpdates(GitHubClient::ReleaseInfo& release_info) {
    Serial.println("OTAHandler: Checking for updates");
    return false; // No updates available
}

OTAHandler::UpdateResult OTAHandler::performUpdate(const GitHubClient::ReleaseInfo& release_info) {
    Serial.println("OTAHandler: Would perform update");
    return OTAHandler::UpdateResult::SUCCESS;
}

} // namespace Network