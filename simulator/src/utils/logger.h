#ifndef LOGGER_H
#define LOGGER_H

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <random>
#include <spdlog/fmt/bundled/printf.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace ratms {

// Log components for filtering and categorization
enum class LogComponent {
    Core,
    API,
    Simulation,
    Optimization,
    Database,
    SSE,
    General
};

// Convert component enum to string
inline std::string_view componentName(LogComponent comp) {
    switch (comp) {
    case LogComponent::Core:
        return "core";
    case LogComponent::API:
        return "api";
    case LogComponent::Simulation:
        return "simulation";
    case LogComponent::Optimization:
        return "optimization";
    case LogComponent::Database:
        return "database";
    case LogComponent::SSE:
        return "sse";
    case LogComponent::General:
    default:
        return "general";
    }
}

class Logger {
  public:
    // Initialize logging system with optional config
    static void init(const std::string &log_dir = "logs");

    // Runtime log level control
    static void setLevel(spdlog::level::level_enum level);
    static void setLevel(const std::string &level_str);
    static spdlog::level::level_enum getLevel();

    // Request ID management for tracing
    static void setRequestId(const std::string &request_id);
    static void clearRequestId();
    static const std::string &getRequestId();

    // Generate unique request ID
    static std::string generateRequestId();

    // Shutdown logging (flush and cleanup)
    static void shutdown();

    // Component-aware logging methods
    template <typename... Args>
    static void info(LogComponent comp, spdlog::format_string_t<Args...> fmt,
                     Args &&...args) {
        if (logger_) {
            std::string prefix = formatPrefix(comp);
            logger_->info("{}{}", prefix,
                          fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    static void error(LogComponent comp, spdlog::format_string_t<Args...> fmt,
                      Args &&...args) {
        if (logger_) {
            std::string prefix = formatPrefix(comp);
            logger_->error("{}{}", prefix,
                           fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    static void warn(LogComponent comp, spdlog::format_string_t<Args...> fmt,
                     Args &&...args) {
        if (logger_) {
            std::string prefix = formatPrefix(comp);
            logger_->warn("{}{}", prefix,
                          fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    static void debug(LogComponent comp, spdlog::format_string_t<Args...> fmt,
                      Args &&...args) {
        if (logger_) {
            std::string prefix = formatPrefix(comp);
            logger_->debug("{}{}", prefix,
                           fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    template <typename... Args>
    static void trace(LogComponent comp, spdlog::format_string_t<Args...> fmt,
                      Args &&...args) {
        if (logger_) {
            std::string prefix = formatPrefix(comp);
            logger_->trace("{}{}", prefix,
                           fmt::format(fmt, std::forward<Args>(args)...));
        }
    }

    // Performance logging
    static void logLatency(LogComponent comp, std::string_view operation,
                           double latency_ms);

  private:
    static std::string formatPrefix(LogComponent comp);

    static std::shared_ptr<spdlog::logger> logger_;
    static thread_local std::string current_request_id_;
};

// Scoped timer for automatic performance logging
class ScopedTimer {
  public:
    ScopedTimer(LogComponent comp, std::string_view operation)
        : comp_(comp), operation_(operation),
          start_(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto ms =
            std::chrono::duration<double, std::milli>(end - start_).count();
        Logger::logLatency(comp_, operation_, ms);
    }

    // Non-copyable
    ScopedTimer(const ScopedTimer &) = delete;
    ScopedTimer &operator=(const ScopedTimer &) = delete;

  private:
    LogComponent comp_;
    std::string_view operation_;
    std::chrono::steady_clock::time_point start_;
};

// Request context for automatic request ID management
class RequestContext {
  public:
    RequestContext() : request_id_(Logger::generateRequestId()) {
        Logger::setRequestId(request_id_);
        start_time_ = std::chrono::steady_clock::now();
    }

    explicit RequestContext(const std::string &request_id)
        : request_id_(request_id) {
        Logger::setRequestId(request_id_);
        start_time_ = std::chrono::steady_clock::now();
    }

    ~RequestContext() {
        auto duration = std::chrono::steady_clock::now() - start_time_;
        auto ms =
            std::chrono::duration<double, std::milli>(duration).count();
        Logger::logLatency(LogComponent::API, "request", ms);
        Logger::clearRequestId();
    }

    const std::string &requestId() const { return request_id_; }

    // Non-copyable
    RequestContext(const RequestContext &) = delete;
    RequestContext &operator=(const RequestContext &) = delete;

  private:
    std::string request_id_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace ratms

// Modern logging macros with component tagging
#define LOG_INFO(comp, ...) ratms::Logger::info(comp, __VA_ARGS__)
#define LOG_ERROR(comp, ...) ratms::Logger::error(comp, __VA_ARGS__)
#define LOG_WARN(comp, ...) ratms::Logger::warn(comp, __VA_ARGS__)
#define LOG_DEBUG(comp, ...) ratms::Logger::debug(comp, __VA_ARGS__)
#define LOG_TRACE(comp, ...) ratms::Logger::trace(comp, __VA_ARGS__)

// Performance timing macro
#define TIMED_SCOPE(comp, name)                                                \
    ratms::ScopedTimer _timed_scope_##__LINE__(comp, name)

// Request context macro
#define REQUEST_SCOPE() ratms::RequestContext _request_context_

// Backward compatibility macros (map to General component)
// These preserve the old printf-style interface using fmt::sprintf
#define __FILENAME__                                                           \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define log_info(fmt_str, ...)                                                 \
    spdlog::info(fmt::sprintf("%s:%d: " fmt_str, __FILENAME__, __LINE__,       \
                              ##__VA_ARGS__))

#define log_error(fmt_str, ...)                                                \
    spdlog::error(fmt::sprintf("%s:%d: " fmt_str, __FILENAME__, __LINE__,      \
                               ##__VA_ARGS__))

#define log_warning(fmt_str, ...)                                              \
    spdlog::warn(fmt::sprintf("%s:%d: " fmt_str, __FILENAME__, __LINE__,       \
                              ##__VA_ARGS__))

#define log_debug(fmt_str, ...)                                                \
    spdlog::debug(fmt::sprintf("%s:%d: " fmt_str, __FILENAME__, __LINE__,      \
                               ##__VA_ARGS__))

#endif // LOGGER_H