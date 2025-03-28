#pragma once

#include <iostream>
#include <fstream>
#include <mutex>
#include <format>
#include <chrono>
#include <string_view>

class Logger {
public:
    Logger(const std::string& filename) : log_file(filename, std::ios::trunc) {
        if (!log_file) {
            throw std::runtime_error("Failed to open log file");
        }
    }

    template <typename... Args>
    void log(std::format_string<Args...> format_str, Args&&... args) {
        std::scoped_lock lock(mtx);
        std::string message = std::format(format_str, std::forward<Args>(args)...);
        //std::string timestamp = current_time();
        //std::string log_entry = std::format("[{}] {}", message);

        // Print to console
        std::cout << message << std::endl;
        
        // Write to file
        if (log_file) {
            log_file << message << '\n';
            log_file.flush();
        }
    }

private:
    std::ofstream log_file;
    std::mutex mtx;

    // static std::string current_time() {
    //     auto now = std::chrono::system_clock::now();
    //     auto time_t_now = std::chrono::system_clock::to_time_t(now);
    //     return std::format("{:%Y-%m-%d %H:%M:%S}", *std::localtime(&time_t_now));
    // }
};

inline Logger global_logger("log.txt");

#define LOG(fmt, ...) global_logger.log(fmt, __VA_ARGS__)