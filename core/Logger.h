#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <deque>
#include <mutex>

// Simple enum for log levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

struct LogEntry {
    LogLevel level;
    std::string message;
    float time; // Real time of the log
};

// Basic Logger class
class Logger {
public:
    static void log(LogLevel level, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        LogEntry entry = { level, message, 0.0f /* Time will be set by UISystem if needed or here */ };
        m_buffer.push_back(entry);
        
        if (m_buffer.size() > MAX_LOGS) {
            m_buffer.pop_front();
        }

        switch (level) {
            case LogLevel::Debug:
                std::cerr << "[DEBUG] " << message << std::endl;
                break;
            case LogLevel::Info:
                std::cout << "[INFO] " << message << std::endl;
                break;
            case LogLevel::Warning:
                std::cerr << "[WARNING] " << message << std::endl;
                break;
            case LogLevel::Error:
                std::cerr << "[ERROR] " << message << std::endl;
                break;
        }
    }

    static std::vector<LogEntry> getRecentLogs() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::vector<LogEntry>(m_buffer.begin(), m_buffer.end());
    }

private:
    static inline std::deque<LogEntry> m_buffer;
    static inline std::mutex m_mutex;
    static const size_t MAX_LOGS = 100;
};