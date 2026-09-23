#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ufx {

namespace fs = std::filesystem;

Logger& Logger::Instance() {
    static Logger inst;
    return inst;
}

void Logger::Init(const fs::path& logDir) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::error_code ec;
    fs::create_directories(logDir, ec);
    file_.open(logDir / "universalframefx.log", std::ios::app);
    initialized_ = true;
}

namespace {
const char* LevelStr(LogLevel l) {
    switch (l) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

std::string Timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string line = "[" + Timestamp() + "] [" + LevelStr(level) + "] " + message + "\n";
    if (file_.is_open()) { file_ << line; file_.flush(); }
    std::cerr << line;
#ifdef _WIN32
    OutputDebugStringA(line.c_str());
#endif
}

}
