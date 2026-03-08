#include "Logger.h"

#ifdef _DEBUG

#include <Windows.h>

#include <array>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <sstream>

namespace {
constexpr const char* kLogFilePrefix = "BF4Purgev3";
}

DebugLogger& DebugLogger::Get() {
  static DebugLogger instance;
  return instance;
}

bool DebugLogger::Initialize() {
  std::lock_guard<std::mutex> guard(m_mutex);
  return EnsureInitializedLocked();
}

void DebugLogger::Shutdown() {
  std::lock_guard<std::mutex> guard(m_mutex);
  if (!m_initialized) return;

  WriteLineLocked(BuildTimestamp() + " [INFO] [logger] shutdown");
  m_stream.close();
  m_initialized = false;
}

void DebugLogger::Write(DebugLogLevel level, const char* category, const char* format, ...) {
  std::lock_guard<std::mutex> guard(m_mutex);
  if (!EnsureInitializedLocked()) return;

  std::array<char, 1024> message = {};

  va_list args;
  va_start(args, format);
  vsnprintf_s(message.data(), message.size(), _TRUNCATE, format, args);
  va_end(args);

  std::ostringstream stream;
  stream << BuildTimestamp() << " [" << GetLevelName(level) << "] ";
  if (category != nullptr && category[0] != '\0') {
    stream << '[' << category << "] ";
  }
  stream << message.data();

  WriteLineLocked(stream.str());
}

const std::string& DebugLogger::GetLogPath() const {
  return m_logPath;
}

bool DebugLogger::EnsureInitializedLocked() {
  if (m_initialized && m_stream.is_open()) return true;
  return OpenLogFileLocked();
}

bool DebugLogger::OpenLogFileLocked() {
  if (m_stream.is_open()) return true;

  m_logPath = BuildLogPathLocked();
  m_stream.open(m_logPath, std::ios::out | std::ios::app);
  if (!m_stream.is_open()) return false;

  m_initialized = true;
  WriteLineLocked(BuildTimestamp() + " [INFO] [logger] initialized");
  return true;
}

std::string DebugLogger::BuildLogPathLocked() const {
  char tempPath[MAX_PATH] = {};
  DWORD tempPathLength = GetTempPathA(MAX_PATH, tempPath);

  std::ostringstream stream;
  if (tempPathLength == 0 || tempPathLength > MAX_PATH) {
    stream << ".\\";
  } else {
    stream << tempPath;
  }

  stream << kLogFilePrefix << "_debug_" << GetCurrentProcessId() << ".log";
  return stream.str();
}

const char* DebugLogger::GetLevelName(DebugLogLevel level) {
  switch (level) {
  case DebugLogLevel::Info:
    return "INFO";
  case DebugLogLevel::Warn:
    return "WARN";
  case DebugLogLevel::Error:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

std::string DebugLogger::BuildTimestamp() {
  SYSTEMTIME localTime = {};
  GetLocalTime(&localTime);

  std::array<char, 64> buffer = {};
  sprintf_s(
    buffer.data(),
    buffer.size(),
    "%04u-%02u-%02u %02u:%02u:%02u.%03u",
    static_cast<unsigned>(localTime.wYear),
    static_cast<unsigned>(localTime.wMonth),
    static_cast<unsigned>(localTime.wDay),
    static_cast<unsigned>(localTime.wHour),
    static_cast<unsigned>(localTime.wMinute),
    static_cast<unsigned>(localTime.wSecond),
    static_cast<unsigned>(localTime.wMilliseconds));
  return buffer.data();
}

void DebugLogger::WriteLineLocked(const std::string& line) {
  m_stream << line << std::endl;
  m_stream.flush();
}

#endif