#pragma once

#ifdef _DEBUG

#include <fstream>
#include <mutex>
#include <string>

enum class DebugLogLevel {
  Info,
  Warn,
  Error,
};

class DebugLogger {
public:
  static DebugLogger& Get();

  bool Initialize();
  void Shutdown();
  void Write(DebugLogLevel level, const char* category, const char* format, ...);

  const std::string& GetLogPath() const;

private:
  DebugLogger() = default;
  DebugLogger(const DebugLogger&) = delete;
  DebugLogger& operator=(const DebugLogger&) = delete;

  bool EnsureInitializedLocked();
  bool OpenLogFileLocked();
  std::string BuildLogPathLocked() const;
  static const char* GetLevelName(DebugLogLevel level);
  static std::string BuildTimestamp();
  void WriteLineLocked(const std::string& line);

  std::mutex m_mutex;
  std::ofstream m_stream;
  std::string m_logPath;
  bool m_initialized = false;
};

#define LOG_INFO(category, ...) DebugLogger::Get().Write(DebugLogLevel::Info, category, __VA_ARGS__)
#define LOG_WARN(category, ...) DebugLogger::Get().Write(DebugLogLevel::Warn, category, __VA_ARGS__)
#define LOG_ERROR(category, ...) DebugLogger::Get().Write(DebugLogLevel::Error, category, __VA_ARGS__)

#else

#define LOG_INFO(...) ((void)0)
#define LOG_WARN(...) ((void)0)
#define LOG_ERROR(...) ((void)0)

#endif