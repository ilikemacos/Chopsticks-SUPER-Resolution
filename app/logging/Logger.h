#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace ufx {

enum class LogLevel { Trace, Debug, Info, Warn, Error };

// Minimal thread-safe file logger. Writes to <logDir>/universalframefx.log and
// mirrors to the debugger/stderr. Never uploads anything.
class Logger {
public:
    static Logger& Instance();

    void Init(const std::filesystem::path& logDir);
    void Log(LogLevel level, const std::string& message);

    void Trace(const std::string& m) { Log(LogLevel::Trace, m); }
    void Debug(const std::string& m) { Log(LogLevel::Debug, m); }
    void Info(const std::string& m)  { Log(LogLevel::Info, m); }
    void Warn(const std::string& m)  { Log(LogLevel::Warn, m); }
    void Error(const std::string& m) { Log(LogLevel::Error, m); }

private:
    Logger() = default;
    std::mutex mutex_;
    std::ofstream file_;
    bool initialized_ = false;
};

}
