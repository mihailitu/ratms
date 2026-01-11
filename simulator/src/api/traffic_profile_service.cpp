#include "traffic_profile_service.h"
#include "../utils/logger.h"
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

namespace ratms {
namespace api {

TrafficProfileService::TrafficProfileService(
    std::shared_ptr<data::DatabaseManager> database,
    std::shared_ptr<simulator::Simulator> simulator,
    std::mutex& simMutex)
    : database_(database), simulator_(simulator), sim_mutex_(simMutex) {
    LOG_INFO(LogComponent::API, "TrafficProfileService initialized");
}

int TrafficProfileService::createProfile(const std::string& name, const std::string& description) {
    return database_->createProfile(name, description);
}

TrafficProfileService::TrafficProfile TrafficProfileService::getProfile(int profileId) {
    auto record = database_->getProfile(profileId);
    return recordToProfile(record);
}

TrafficProfileService::TrafficProfile TrafficProfileService::getProfileByName(const std::string& name) {
    auto record = database_->getProfileByName(name);
    return recordToProfile(record);
}

std::vector<TrafficProfileService::TrafficProfile> TrafficProfileService::getAllProfiles() {
    std::vector<TrafficProfile> profiles;
    auto records = database_->getAllProfiles();

    for (const auto& record : records) {
        profiles.push_back(recordToProfile(record));
    }

    return profiles;
}

bool TrafficProfileService::updateProfile(int profileId, const std::string& name, const std::string& description) {
    return database_->updateProfile(profileId, name, description);
}

bool TrafficProfileService::deleteProfile(int profileId) {
    return database_->deleteProfile(profileId);
}

bool TrafficProfileService::setActiveProfile(int profileId) {
    return database_->setActiveProfile(profileId);
}

TrafficProfileService::TrafficProfile TrafficProfileService::getActiveProfile() {
    auto record = database_->getActiveProfile();
    return recordToProfile(record);
}

bool TrafficProfileService::saveProfileData(
    int profileId,
    const std::vector<data::DatabaseManager::ProfileSpawnRateRecord>& spawnRates,
    const std::vector<data::DatabaseManager::ProfileTrafficLightRecord>& trafficLights) {

    bool success = true;
    success = success && database_->saveProfileSpawnRates(profileId, spawnRates);
    success = success && database_->saveProfileTrafficLights(profileId, trafficLights);

    if (success) {
        LOG_INFO(LogComponent::API, "Saved profile {} with {} spawn rates and {} traffic lights",
                profileId, spawnRates.size(), trafficLights.size());
    }

    return success;
}

bool TrafficProfileService::applyProfile(int profileId) {
    auto profile = getProfile(profileId);

    if (profile.id <= 0) {
        LOG_WARN(LogComponent::API, "Cannot apply profile: profile {} not found", profileId);
        return false;
    }

    LOG_INFO(LogComponent::API, "Applying profile '{}' ({} spawn rates, {} traffic lights)",
            profile.name, profile.spawnRates.size(), profile.trafficLights.size());

    // Apply spawn rates
    applySpawnRates(profile.spawnRates);

    // Apply traffic lights
    applyTrafficLights(profile.trafficLights);

    // Set as active profile
    database_->setActiveProfile(profileId);

    LOG_INFO(LogComponent::API, "Profile '{}' applied successfully", profile.name);
    return true;
}

bool TrafficProfileService::applyProfileByName(const std::string& name) {
    auto profile = getProfileByName(name);

    if (profile.id <= 0) {
        LOG_WARN(LogComponent::API, "Cannot apply profile: '{}' not found", name);
        return false;
    }

    return applyProfile(profile.id);
}

int TrafficProfileService::captureCurrentState(const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (!simulator_) {
        LOG_ERROR(LogComponent::API, "Cannot capture state: simulator not initialized");
        return -1;
    }

    // Create the profile
    int profileId = database_->createProfile(name, description);
    if (profileId <= 0) {
        return -1;
    }

    // Capture spawn rates (from roads with spawn capability)
    std::vector<data::DatabaseManager::ProfileSpawnRateRecord> spawnRates;
    // Note: Current spawn rates are stored in server, not in simulator
    // This would need integration with the server's spawn_rates_ map

    // Capture traffic light timings
    std::vector<data::DatabaseManager::ProfileTrafficLightRecord> trafficLights;

    for (const auto& [roadId, road] : simulator_->cityMap) {
        const auto& roadTrafficLights = road.getTrafficLights();
        for (unsigned lane = 0; lane < road.getLanesNo(); ++lane) {
            if (lane < roadTrafficLights.size()) {
                const auto& tl = roadTrafficLights[lane];
                data::DatabaseManager::ProfileTrafficLightRecord record;
                record.profile_id = profileId;
                record.road_id = roadId;
                record.lane = static_cast<int>(lane);
                record.green_time = tl.getGreenTime();
                record.yellow_time = tl.getYellowTime();
                record.red_time = tl.getRedTime();
                trafficLights.push_back(record);
            }
        }
    }

    // Save the captured data
    database_->saveProfileSpawnRates(profileId, spawnRates);
    database_->saveProfileTrafficLights(profileId, trafficLights);

    LOG_INFO(LogComponent::API, "Captured current state as profile '{}' (ID: {}, {} traffic lights)",
            name, profileId, trafficLights.size());

    return profileId;
}

std::string TrafficProfileService::exportProfileToJson(int profileId) {
    auto profile = getProfile(profileId);

    if (profile.id <= 0) {
        return "{}";
    }

    json j;
    j["name"] = profile.name;
    j["description"] = profile.description;

    // Export spawn rates
    j["spawnRates"] = json::array();
    for (const auto& rate : profile.spawnRates) {
        j["spawnRates"].push_back({
            {"roadId", rate.road_id},
            {"lane", rate.lane},
            {"vehiclesPerMinute", rate.vehicles_per_minute}
        });
    }

    // Export traffic lights
    j["trafficLights"] = json::array();
    for (const auto& light : profile.trafficLights) {
        j["trafficLights"].push_back({
            {"roadId", light.road_id},
            {"lane", light.lane},
            {"greenTime", light.green_time},
            {"yellowTime", light.yellow_time},
            {"redTime", light.red_time}
        });
    }

    return j.dump(2);
}

std::string TrafficProfileService::exportProfileToJson(const std::string& name) {
    auto profile = getProfileByName(name);
    if (profile.id <= 0) {
        return "{}";
    }
    return exportProfileToJson(profile.id);
}

int TrafficProfileService::importProfileFromJson(const std::string& jsonStr) {
    try {
        json j = json::parse(jsonStr);

        std::string name = j.value("name", "Imported Profile");
        std::string description = j.value("description", "");

        // Check if profile with this name already exists
        auto existing = database_->getProfileByName(name);
        if (existing.id > 0) {
            LOG_WARN(LogComponent::API, "Profile '{}' already exists, updating...", name);
            // Update existing profile
            database_->updateProfile(existing.id, name, description);

            // Clear and re-import data
            std::vector<data::DatabaseManager::ProfileSpawnRateRecord> spawnRates;
            if (j.contains("spawnRates") && j["spawnRates"].is_array()) {
                for (const auto& rate : j["spawnRates"]) {
                    data::DatabaseManager::ProfileSpawnRateRecord record;
                    record.profile_id = existing.id;
                    record.road_id = rate.value("roadId", 0);
                    record.lane = rate.value("lane", 0);
                    record.vehicles_per_minute = rate.value("vehiclesPerMinute", 10.0);
                    spawnRates.push_back(record);
                }
            }

            std::vector<data::DatabaseManager::ProfileTrafficLightRecord> trafficLights;
            if (j.contains("trafficLights") && j["trafficLights"].is_array()) {
                for (const auto& light : j["trafficLights"]) {
                    data::DatabaseManager::ProfileTrafficLightRecord record;
                    record.profile_id = existing.id;
                    record.road_id = light.value("roadId", 0);
                    record.lane = light.value("lane", 0);
                    record.green_time = light.value("greenTime", 30.0);
                    record.yellow_time = light.value("yellowTime", 3.0);
                    record.red_time = light.value("redTime", 30.0);
                    trafficLights.push_back(record);
                }
            }

            saveProfileData(existing.id, spawnRates, trafficLights);
            return existing.id;
        }

        // Create new profile
        int profileId = database_->createProfile(name, description);
        if (profileId <= 0) {
            LOG_ERROR(LogComponent::API, "Failed to create profile from JSON");
            return -1;
        }

        // Import spawn rates
        std::vector<data::DatabaseManager::ProfileSpawnRateRecord> spawnRates;
        if (j.contains("spawnRates") && j["spawnRates"].is_array()) {
            for (const auto& rate : j["spawnRates"]) {
                data::DatabaseManager::ProfileSpawnRateRecord record;
                record.profile_id = profileId;
                record.road_id = rate.value("roadId", 0);
                record.lane = rate.value("lane", 0);
                record.vehicles_per_minute = rate.value("vehiclesPerMinute", 10.0);
                spawnRates.push_back(record);
            }
        }

        // Import traffic lights
        std::vector<data::DatabaseManager::ProfileTrafficLightRecord> trafficLights;
        if (j.contains("trafficLights") && j["trafficLights"].is_array()) {
            for (const auto& light : j["trafficLights"]) {
                data::DatabaseManager::ProfileTrafficLightRecord record;
                record.profile_id = profileId;
                record.road_id = light.value("roadId", 0);
                record.lane = light.value("lane", 0);
                record.green_time = light.value("greenTime", 30.0);
                record.yellow_time = light.value("yellowTime", 3.0);
                record.red_time = light.value("redTime", 30.0);
                trafficLights.push_back(record);
            }
        }

        saveProfileData(profileId, spawnRates, trafficLights);

        LOG_INFO(LogComponent::API, "Imported profile '{}' (ID: {}) with {} spawn rates, {} traffic lights",
                name, profileId, spawnRates.size(), trafficLights.size());

        return profileId;
    } catch (const json::exception& e) {
        LOG_ERROR(LogComponent::API, "Failed to parse profile JSON: {}", e.what());
        return -1;
    }
}

void TrafficProfileService::loadDefaultProfiles(const std::string& profilesDir) {
    namespace fs = std::filesystem;

    if (!fs::exists(profilesDir)) {
        LOG_DEBUG(LogComponent::API, "Profiles directory does not exist: {}", profilesDir);
        return;
    }

    int loaded = 0;
    for (const auto& entry : fs::directory_iterator(profilesDir)) {
        if (entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                int id = importProfileFromJson(buffer.str());
                if (id > 0) {
                    loaded++;
                    LOG_INFO(LogComponent::API, "Loaded default profile: {}", entry.path().filename().string());
                }
            }
        }
    }

    LOG_INFO(LogComponent::API, "Loaded {} default profiles from {}", loaded, profilesDir);
}

TrafficProfileService::TrafficProfile TrafficProfileService::recordToProfile(
    const data::DatabaseManager::ProfileRecord& record) {

    TrafficProfile profile;
    profile.id = record.id;
    profile.name = record.name;
    profile.description = record.description;
    profile.isActive = record.is_active;
    profile.createdAt = record.created_at;

    if (record.id > 0) {
        profile.spawnRates = database_->getProfileSpawnRates(record.id);
        profile.trafficLights = database_->getProfileTrafficLights(record.id);
    }

    return profile;
}

void TrafficProfileService::applySpawnRates(
    const std::vector<data::DatabaseManager::ProfileSpawnRateRecord>& rates) {

    // Note: Spawn rates are managed by the server, not the simulator directly
    // This would need integration with server's spawn_rates_ map
    // For now, we just log what would be applied
    LOG_DEBUG(LogComponent::API, "Would apply {} spawn rates (requires server integration)", rates.size());
}

void TrafficProfileService::applyTrafficLights(
    const std::vector<data::DatabaseManager::ProfileTrafficLightRecord>& lights) {

    std::lock_guard<std::mutex> lock(sim_mutex_);

    if (!simulator_) {
        LOG_ERROR(LogComponent::API, "Cannot apply traffic lights: simulator not initialized");
        return;
    }

    int applied = 0;
    for (const auto& light : lights) {
        auto roadIt = simulator_->cityMap.find(light.road_id);
        if (roadIt != simulator_->cityMap.end()) {
            auto& road = roadIt->second;
            auto& roadTrafficLights = road.getTrafficLightsMutable();
            if (static_cast<unsigned>(light.lane) < roadTrafficLights.size()) {
                roadTrafficLights[light.lane].setTimings(
                    light.green_time, light.yellow_time, light.red_time);
                applied++;
            }
        }
    }

    LOG_INFO(LogComponent::API, "Applied {} traffic light timings", applied);
}

} // namespace api
} // namespace ratms
