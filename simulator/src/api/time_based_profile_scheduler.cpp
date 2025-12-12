#include "time_based_profile_scheduler.h"
#include "../utils/logger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <ctime>

using json = nlohmann::json;

namespace ratms {
namespace api {

TimeBasedProfileScheduler::TimeBasedProfileScheduler() = default;

TimeBasedProfileScheduler::~TimeBasedProfileScheduler() {
    stop();
}

bool TimeBasedProfileScheduler::loadProfiles(const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR(LogComponent::API, "Failed to open profiles file: {}", filePath);
            return false;
        }

        json j;
        file >> j;

        // Read base rate
        if (j.contains("baseRateVehiclesPerMinute")) {
            baseSpawnRate_ = j["baseRateVehiclesPerMinute"].get<double>();
        }

        // Read default profile
        if (j.contains("defaultProfile")) {
            defaultProfileName_ = j["defaultProfile"].get<std::string>();
        }

        // Read profiles
        profiles_.clear();
        if (j.contains("profiles") && j["profiles"].is_array()) {
            for (const auto& p : j["profiles"]) {
                TimeProfile profile;
                profile.name = p.value("name", "");
                profile.description = p.value("description", "");
                profile.rateMultiplier = p.value("rateMultiplier", 1.0);

                if (p.contains("hours") && p["hours"].is_array()) {
                    for (const auto& h : p["hours"]) {
                        profile.hours.push_back(h.get<int>());
                    }
                }

                // Check if this is a weekend-specific profile
                if (profile.name.find("weekend") != std::string::npos) {
                    profile.isWeekendProfile = true;
                }

                profiles_.push_back(profile);
                LOG_DEBUG(LogComponent::API, "Loaded profile: {} (hours: {}, multiplier: {:.2f})",
                         profile.name, profile.hours.size(), profile.rateMultiplier);
            }
        }

        LOG_INFO(LogComponent::API, "Loaded {} traffic profiles from {}", profiles_.size(), filePath);
        return !profiles_.empty();

    } catch (const std::exception& e) {
        LOG_ERROR(LogComponent::API, "Failed to parse profiles file: {}", e.what());
        return false;
    }
}

void TimeBasedProfileScheduler::setSpawnRateCallback(SpawnRateCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    spawnRateCallback_ = std::move(callback);
}

void TimeBasedProfileScheduler::setBaseSpawnRate(double rate) {
    baseSpawnRate_ = rate;
}

void TimeBasedProfileScheduler::start(int checkIntervalSeconds) {
    if (running_) {
        LOG_WARN(LogComponent::API, "Profile scheduler already running");
        return;
    }

    checkIntervalSeconds_ = checkIntervalSeconds;
    shouldStop_ = false;
    running_ = true;

    // Apply profile immediately on start
    applyCurrentTimeProfile();

    // Start background thread
    schedulerThread_ = std::make_unique<std::thread>(&TimeBasedProfileScheduler::schedulerLoop, this);

    LOG_INFO(LogComponent::API, "Time-based profile scheduler started (check interval: {}s)", checkIntervalSeconds);
}

void TimeBasedProfileScheduler::stop() {
    if (!running_) return;

    shouldStop_ = true;

    if (schedulerThread_ && schedulerThread_->joinable()) {
        schedulerThread_->join();
    }

    running_ = false;
    LOG_INFO(LogComponent::API, "Time-based profile scheduler stopped");
}

std::string TimeBasedProfileScheduler::getCurrentProfileName() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentProfileName_;
}

double TimeBasedProfileScheduler::getCurrentRateMultiplier() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentRateMultiplier_;
}

const TimeProfile* TimeBasedProfileScheduler::findProfileForTime(int hour, bool isWeekend) const {
    // First, try to find a weekend-specific profile if it's a weekend
    if (isWeekend) {
        for (const auto& profile : profiles_) {
            if (profile.isWeekendProfile) {
                for (int h : profile.hours) {
                    if (h == hour) {
                        return &profile;
                    }
                }
            }
        }
    }

    // Find matching non-weekend profile
    for (const auto& profile : profiles_) {
        if (profile.isWeekendProfile) continue;  // Skip weekend profiles for weekdays

        for (int h : profile.hours) {
            if (h == hour) {
                return &profile;
            }
        }
    }

    // Fall back to default profile
    for (const auto& profile : profiles_) {
        if (profile.name == defaultProfileName_) {
            return &profile;
        }
    }

    return nullptr;
}

void TimeBasedProfileScheduler::applyCurrentTimeProfile() {
    // Get current system time
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&now_time);

    int hour = local_tm->tm_hour;
    int dayOfWeek = local_tm->tm_wday;  // 0 = Sunday, 6 = Saturday
    bool isWeekend = (dayOfWeek == 0 || dayOfWeek == 6);

    const TimeProfile* profile = findProfileForTime(hour, isWeekend);

    if (profile) {
        std::string newProfileName;
        double newMultiplier;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            newProfileName = profile->name;
            newMultiplier = profile->rateMultiplier;

            // Check if profile changed
            if (currentProfileName_ == newProfileName) {
                return;  // No change needed
            }

            currentProfileName_ = newProfileName;
            currentRateMultiplier_ = newMultiplier;
        }

        LOG_INFO(LogComponent::API, "Applying traffic profile: {} (hour: {}, multiplier: {:.2f}, weekend: {})",
                 newProfileName, hour, newMultiplier, isWeekend);

        // Call callback to update spawn rates
        SpawnRateCallback callback;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback = spawnRateCallback_;
        }

        if (callback) {
            callback(newMultiplier, newProfileName);
        }
    } else {
        LOG_WARN(LogComponent::API, "No matching profile found for hour {} (weekend: {})", hour, isWeekend);
    }
}

void TimeBasedProfileScheduler::schedulerLoop() {
    while (!shouldStop_) {
        // Sleep for the check interval (in small increments for responsiveness)
        for (int i = 0; i < checkIntervalSeconds_ && !shouldStop_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (shouldStop_) break;

        applyCurrentTimeProfile();
    }
}

} // namespace api
} // namespace ratms
