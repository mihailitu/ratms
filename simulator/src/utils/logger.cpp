#include "logger.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sstream>
#include <vector>

namespace ratms {

// Static member definitions
std::shared_ptr<spdlog::logger> Logger::logger_;
thread_local std::string Logger::current_request_id_;

void Logger::init(const std::string &log_dir) {
    // Create log directory if it doesn't exist
    std::filesystem::create_directories(log_dir);

    // Console sink - human readable with colors
    auto console_sink =
        std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    // Rotating file sink - detailed logs
    std::string log_file = log_dir + "/ratms.log";
    auto file_sink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file,
            10 * 1024 * 1024, // 10 MB max size
            5                  // Keep 5 rotated files
        );
    file_sink->set_level(spdlog::level::debug);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");

    // JSON file sink for structured logging (machine-parseable)
    std::string json_log_file = log_dir + "/ratms.json.log";
    auto json_sink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            json_log_file,
            10 * 1024 * 1024, // 10 MB max size
            5                  // Keep 5 rotated files
        );
    json_sink->set_level(spdlog::level::debug);
    // JSON pattern: timestamp, level, thread, message
    json_sink->set_pattern(
        R"({"ts":"%Y-%m-%dT%H:%M:%S.%e","level":"%l","thread":%t,"msg":"%v"})");

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink, json_sink};
    logger_ =
        std::make_shared<spdlog::logger>("ratms", sinks.begin(), sinks.end());

    // Set default level from environment or default to info
    const char *level_env = std::getenv("RATMS_LOG_LEVEL");
    if (level_env) {
        logger_->set_level(spdlog::level::from_str(level_env));
    } else {
        logger_->set_level(spdlog::level::info);
    }

    // Async flush every 3 seconds (better performance than per-message)
    spdlog::flush_every(std::chrono::seconds(3));

    // Also flush on info and above for reliability
    logger_->flush_on(spdlog::level::info);

    // Set as default logger
    spdlog::set_default_logger(logger_);

    logger_->info("[general] Logger initialized: console={}, file={}, json={}",
                  "stdout", log_file, json_log_file);
}

void Logger::setLevel(spdlog::level::level_enum level) {
    if (logger_) {
        logger_->set_level(level);
        logger_->info("[general] Log level changed to: {}",
                      spdlog::level::to_string_view(level));
    }
}

void Logger::setLevel(const std::string &level_str) {
    setLevel(spdlog::level::from_str(level_str));
}

spdlog::level::level_enum Logger::getLevel() {
    if (logger_) {
        return logger_->level();
    }
    return spdlog::level::info;
}

void Logger::setRequestId(const std::string &request_id) {
    current_request_id_ = request_id;
}

void Logger::clearRequestId() { current_request_id_.clear(); }

const std::string &Logger::getRequestId() { return current_request_id_; }

std::string Logger::generateRequestId() {
    static thread_local std::mt19937 gen(std::random_device{}());
    static thread_local std::uniform_int_distribution<> dis(0, 15);
    static const char *hex = "0123456789abcdef";

    std::string id = "req-";
    id.reserve(12); // "req-" + 8 hex chars
    for (int i = 0; i < 8; ++i) {
        id += hex[dis(gen)];
    }
    return id;
}

void Logger::shutdown() {
    if (logger_) {
        logger_->info("[general] Logger shutting down");
        logger_->flush();
    }
    spdlog::shutdown();
}

std::string Logger::formatPrefix(LogComponent comp) {
    std::string prefix = "[";
    prefix += componentName(comp);
    prefix += "]";

    if (!current_request_id_.empty()) {
        prefix += " [";
        prefix += current_request_id_;
        prefix += "]";
    }

    prefix += " ";
    return prefix;
}

void Logger::logLatency(LogComponent comp, std::string_view operation,
                        double latency_ms) {
    if (logger_) {
        std::string prefix = formatPrefix(comp);
        logger_->info("{}[perf] {} completed in {:.2f}ms", prefix, operation,
                      latency_ms);
    }
}

} // namespace ratms