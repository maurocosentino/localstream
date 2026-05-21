#pragma once

#include <chrono>
#include <ctime>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace localstream {

enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

class Logger {
 public:
  // Singleton — acceso global a la única instancia
  static Logger& instance() {
    static Logger logger;
    return logger;
  }

  void setLevel(LogLevel level) { min_level_ = level; }

  void log(LogLevel level, const std::string& module,
           const std::string& message) {
    if (level < min_level_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    std::cout << "[" << timestamp() << "] " << "[" << levelToString(level)
              << "] " << "[" << padModule(module) << "] " << message << "\n";
  }

  // Elimina copy y move — un Singleton no se copia
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
  Logger(Logger&&) = delete;
  Logger& operator=(Logger&&) = delete;

 private:
  Logger() : min_level_(LogLevel::INFO) {}

  LogLevel min_level_;
  std::mutex mutex_;

  std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
  }

  std::string levelToString(LogLevel level) {
    switch (level) {
      case LogLevel::DEBUG:
        return "DEBUG";
      case LogLevel::INFO:
        return "INFO ";
      case LogLevel::WARN:
        return "WARN ";
      case LogLevel::ERROR:
        return "ERROR";
    }
    return "?????";
  }

  std::string padModule(const std::string& module) {
    // Padding fijo de 8 caracteres para alinear columnas
    if (module.size() >= 8) return module.substr(0, 8);
    return module + std::string(8 - module.size(), ' ');
  }
};

// Macros de conveniencia — evitan escribir Logger::instance().log(...) cada vez
#define LOG_DEBUG(module, msg) \
  localstream::Logger::instance().log(localstream::LogLevel::DEBUG, module, msg)

#define LOG_INFO(module, msg) \
  localstream::Logger::instance().log(localstream::LogLevel::INFO, module, msg)

#define LOG_WARN(module, msg) \
  localstream::Logger::instance().log(localstream::LogLevel::WARN, module, msg)

#define LOG_ERROR(module, msg) \
  localstream::Logger::instance().log(localstream::LogLevel::ERROR, module, msg)

}  // namespace localstream