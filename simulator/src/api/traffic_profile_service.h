#ifndef TRAFFIC_PROFILE_SERVICE_H
#define TRAFFIC_PROFILE_SERVICE_H

#include "../data/storage/database_manager.h"
#include "../core/simulator.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace ratms {
namespace api {

/**
 * @brief Service for managing traffic profiles
 *
 * Handles loading, saving, applying, and exporting traffic profiles
 * which contain spawn rates and traffic light timings.
 */
class TrafficProfileService {
public:
    // Complete profile with all data
    struct TrafficProfile {
        int id;
        std::string name;
        std::string description;
        bool isActive;
        int64_t createdAt;
        std::vector<data::DatabaseManager::ProfileSpawnRateRecord> spawnRates;
        std::vector<data::DatabaseManager::ProfileTrafficLightRecord> trafficLights;
    };

    TrafficProfileService(
        std::shared_ptr<data::DatabaseManager> database,
        std::shared_ptr<simulator::Simulator> simulator,
        std::mutex& simMutex);

    // Profile CRUD operations
    int createProfile(const std::string& name, const std::string& description);
    TrafficProfile getProfile(int profileId);
    TrafficProfile getProfileByName(const std::string& name);
    std::vector<TrafficProfile> getAllProfiles();
    bool updateProfile(int profileId, const std::string& name, const std::string& description);
    bool deleteProfile(int profileId);

    // Profile activation
    bool setActiveProfile(int profileId);
    TrafficProfile getActiveProfile();

    // Save spawn rates and traffic lights to a profile
    bool saveProfileData(int profileId,
                        const std::vector<data::DatabaseManager::ProfileSpawnRateRecord>& spawnRates,
                        const std::vector<data::DatabaseManager::ProfileTrafficLightRecord>& trafficLights);

    // Apply profile to simulation (sets spawn rates and traffic lights)
    bool applyProfile(int profileId);
    bool applyProfileByName(const std::string& name);

    // Capture current simulation state as a new profile
    int captureCurrentState(const std::string& name, const std::string& description);

    // JSON import/export
    std::string exportProfileToJson(int profileId);
    std::string exportProfileToJson(const std::string& name);
    int importProfileFromJson(const std::string& jsonStr);

    // Load default profiles from JSON files (called at startup)
    void loadDefaultProfiles(const std::string& profilesDir);

private:
    std::shared_ptr<data::DatabaseManager> database_;
    std::shared_ptr<simulator::Simulator> simulator_;
    std::mutex& sim_mutex_;

    // Helper to convert DB records to TrafficProfile
    TrafficProfile recordToProfile(const data::DatabaseManager::ProfileRecord& record);

    // Helper to apply spawn rates to simulation
    void applySpawnRates(const std::vector<data::DatabaseManager::ProfileSpawnRateRecord>& rates);

    // Helper to apply traffic light timings to simulation
    void applyTrafficLights(const std::vector<data::DatabaseManager::ProfileTrafficLightRecord>& lights);
};

} // namespace api
} // namespace ratms

#endif // TRAFFIC_PROFILE_SERVICE_H
