#pragma once

#include <string>
#include <iostream>

// Simple enum for log levels
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

// Basic Logger class
class Logger {
public:
    static void log(LogLevel level, const std::string& message) {
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
};