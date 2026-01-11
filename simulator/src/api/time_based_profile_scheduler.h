#ifndef TIME_BASED_PROFILE_SCHEDULER_H
#define TIME_BASED_PROFILE_SCHEDULER_H

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>

namespace ratms {
namespace api {

/**
 * @brief Profile definition loaded from JSON
 */
struct TimeProfile {
    std::string name;
    std::string description;
    std::vector<int> hours;           // Hours when this profile is active (0-23)
    double rateMultiplier;            // Multiplier for base spawn rate
    bool isWeekendProfile = false;    // If true, only applies on weekends
};

/**
 * @brief TimeBasedProfileScheduler - Applies traffic profiles based on system time
 *
 * Reads profile definitions from JSON file and periodically checks system time
 * to apply the appropriate profile. Uses rate multipliers to adjust spawn rates.
 */
class TimeBasedProfileScheduler {
public:
    using SpawnRateCallback = std::function<void(double rateMultiplier, const std::string& profileName)>;

    TimeBasedProfileScheduler();
    ~TimeBasedProfileScheduler();

    /**
     * @brief Load profiles from JSON file
     * @param filePath Path to profiles JSON file
     * @return true if loaded successfully
     */
    bool loadProfiles(const std::string& filePath);

    /**
     * @brief Set callback for when spawn rates should be updated
     * @param callback Function to call with rate multiplier and profile name
     */
    void setSpawnRateCallback(SpawnRateCallback callback);

    /**
     * @brief Set base spawn rate (vehicles per minute)
     * @param rate Base rate before multiplier is applied
     */
    void setBaseSpawnRate(double rate);

    /**
     * @brief Get base spawn rate
     */
    double getBaseSpawnRate() const { return baseSpawnRate_; }

    /**
     * @brief Start the scheduler thread
     * @param checkIntervalSeconds How often to check for profile changes (default: 60)
     */
    void start(int checkIntervalSeconds = 60);

    /**
     * @brief Stop the scheduler thread
     */
    void stop();

    /**
     * @brief Check if scheduler is running
     */
    bool isRunning() const { return running_; }

    /**
     * @brief Get the currently active profile name
     */
    std::string getCurrentProfileName() const;

    /**
     * @brief Get the current rate multiplier
     */
    double getCurrentRateMultiplier() const;

    /**
     * @brief Force immediate profile check and apply
     */
    void applyCurrentTimeProfile();

private:
    /**
     * @brief Find the appropriate profile for given hour and day
     * @param hour Hour of day (0-23)
     * @param isWeekend True if Saturday or Sunday
     * @return Pointer to matching profile, or nullptr if none found
     */
    const TimeProfile* findProfileForTime(int hour, bool isWeekend) const;

    /**
     * @brief Scheduler thread main loop
     */
    void schedulerLoop();

    std::vector<TimeProfile> profiles_;
    std::string defaultProfileName_;
    double baseSpawnRate_ = 10.0;

    SpawnRateCallback spawnRateCallback_;

    std::atomic<bool> running_{false};
    std::atomic<bool> shouldStop_{false};
    std::unique_ptr<std::thread> schedulerThread_;
    int checkIntervalSeconds_ = 60;

    mutable std::mutex mutex_;
    std::string currentProfileName_;
    double currentRateMultiplier_ = 1.0;
};

} // namespace api
} // namespace ratms

#endif // TIME_BASED_PROFILE_SCHEDULER_H
