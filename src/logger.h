#pragma once

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>
#include "bq.h"
#include "common.h"

#define LLOG(logger_name, event_level)                                                                    \
  xac::LogEventWrap::SharedPtr(                                                                           \
      new xac::LogEventWrap(xac::LogEvent::SharedPtr(new xac::LogEvent(                                   \
          __FILE__, time(NULL), 0, __LINE__, xac::GetThreadId(), xac::GetThreadName(), xac::GetFiberId(), \
          LoggerManager::GetInstance()->GetLogger(logger_name)->GetName(), event_level))))                \
      ->GetStringStream()

#define LDEBUG(logger_name) LLOG(logger_name, xac::LogLevel::Level::DEBUG)
#define LINFO(logger_name) LLOG(logger_name, xac::LogLevel::Level::INFO)
#define LWARN(logger_name) LLOG(logger_name, xac::LogLevel::Level::WARN)
#define LERROR(logger_name) LLOG(logger_name, xac::LogLevel::Level::ERROR)
#define LFATAL(logger_name) LLOG(logger_name, xac::LogLevel::Level::FATAL)

#define LRDEBUG LDEBUG("root")
#define LRINFO LINFO("root")
#define LRWARN LWARN("root")
#define LRERROR LERROR("root")
#define LRFATAL LFATAL("root")

#define FLLOG(logger_name, event_level, format, ...)                                                      \
  xac::LogEventWrap::SharedPtr(                                                                           \
      new xac::LogEventWrap(xac::LogEvent::SharedPtr(new xac::LogEvent(                                   \
          __FILE__, time(NULL), 0, __LINE__, xac::GetThreadId(), xac::GetThreadName(), xac::GetFiberId(), \
          xac::LoggerManager::GetInstance()->GetLogger(logger_name)->GetName(), event_level))))           \
      ->GetEvent()                                                                                        \
      ->Format(format, __VA_ARGS__)
#define FLDEBUG(logger_name, format, ...) FLLOG(logger_name, xac::LogLevel::Level::DEBUG, format, __VA_ARGS__)
#define FLINFO(logger_name, format, ...) FLLOG(logger_name, xac::LogLevel::Level::INFO, format, __VA_ARGS__)
#define FLWARN(logger_name, format, ...) FLLOG(logger_name, xac::LogLevel::Level::WARN, format, __VA_ARGS__)
#define FLERROR(logger_name, format, ...) FLLOG(logger_name, xac::LogLevel::Level::ERROR, format, __VA_ARGS__)
#define FLFATAL(logger_name, format, ...) FLLOG(logger_name, xac::LogLevel::Level::FATAL, format, __VA_ARGS__)

#define FLRDEBUG(format, ...) FLDEBUG("root", format, __VA_ARGS__)
#define FLRINFO(format, ...) FLINFO("root", format, __VA_ARGS__)
#define FLRWARN(format, ...) FLWARNING("root", fromat, __VA_ARGS__)
#define FLRERROR(format, ...) FLERROR("root", format, _VA__ARGS__)
#define FLRFATAL(format, ...) FLFATAL("root", format, _VA_ARGS__)

namespace xac {

class Logger;
class LogAppenderBase;
class LoggerManager;

class LogLevel {
 public:
  enum Level {
    UNKNOWN = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5,
  };
  static Level ToLevel(const std::string &level_str);
  static constexpr const char *ToString(Level level);
};

class LogEvent {
  friend class LogEventWrap;

 public:
  using SharedPtr = std::shared_ptr<LogEvent>;
  LogEvent() = delete;
  LogEvent(const char *file_name, const uint64_t &time, const uint32_t &elapse, const uint32_t &line,
           const uint32_t &thread_id, std::string thread_name, const uint32_t &fiber_id, std::string logger_name,
           LogLevel::Level level);
  const char *GetFileName() const { return file_name_; }
  const uint64_t &GetTime() const { return time_; }
  const uint32_t &GetElapse() const { return elapse_; }
  const uint32_t &GetLine() const { return line_; }
  const uint32_t &GetThreadId() const { return thread_id_; }
  const std::string &GetThreadName() const { return thread_name_; }
  const uint32_t &GetFiberId() const { return fiber_id_; }
  const std::string GetContent() const { return content_ss_.str(); }
  LogLevel::Level GetLevel() const { return level_; }
  const std::string GetLoggerName() { return logger_name_; }
  std::stringstream &GetStringStream() { return content_ss_; }
  void Format(const char *format, ...);

 private:
  const char *file_name_;         // file name
  uint64_t time_;                 // timestamp
  uint32_t elapse_;               // elapsed time from program run
  uint32_t line_;                 // line number
  uint32_t thread_id_;            // thread id
  std::string thread_name_;       // thread name
  uint32_t fiber_id_;             // fiber id
  std::stringstream content_ss_;  // content
  std::string logger_name_;
  LogLevel::Level level_;
};

class LogEventWrap {
 public:
  typedef std::shared_ptr<LogEventWrap> SharedPtr;
  LogEventWrap(LogEvent::SharedPtr event) : event_(event) {}
  ~LogEventWrap();
  LogEvent::SharedPtr GetEvent() { return event_; }
  std::stringstream &GetStringStream() { return event_->GetStringStream(); }

 private:
  LogEvent::SharedPtr event_;
};

class Formatter {
 public:
  using SharedPtr = std::shared_ptr<Formatter>;
  Formatter() { PatternParse(); }
  explicit Formatter(std::string pattern);
  // output the event as formatted
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event);
  class FormatItemBase {  // every format item has `Format` method to output
                          // their content
   public:
    using SharedPtr = std::shared_ptr<FormatItemBase>;
    // FormatItemBase(const std::string& str = "") {}
    virtual ~FormatItemBase() = default;
    virtual void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) = 0;
  };
  static const std::string COMPLEXPATTERN;
  static const std::string SIMPLEPATTERN;

 private:
  std::string pattern_ = std::string(SIMPLEPATTERN);     // the pattern of formatter
  std::vector<FormatItemBase::SharedPtr> format_items_;  // formatted items
  void PatternParse();                                   // parse the pattern to format items
};

class LogAppenderBase {
  friend class Logger;

 public:
  using SharedPtr = std::shared_ptr<LogAppenderBase>;
  virtual ~LogAppenderBase() = default;
  // set the formatter of the appender
  void SetFormatter(const Formatter::SharedPtr &formatter);
  // get the formatter of the appender
  Formatter::SharedPtr GetFormatter();
  // set the level of the appender
  void SetLevel(LogLevel::Level level);
  // get the level of the appender
  LogLevel::Level GetLevel();

 protected:
  LogAppenderBase();
  std::shared_mutex shared_mutex_;
  LogLevel::Level level_ = LogLevel::Level::DEBUG;
  Formatter::SharedPtr formatter_;
  // set the event level and log it
  virtual void Log(LogLevel::Level level, LogEvent::SharedPtr event) = 0;
};

class Logger {
 public:
  using SharedPtr = std::shared_ptr<Logger>;
  Logger() = delete;
  Logger(const std::string &name);
  Logger(const Logger &logger) = delete;
  ~Logger();
  // log the event
  void Log(const LogEvent::SharedPtr &event);
  void AddAppender(const LogAppenderBase::SharedPtr &appender);
  // get log appender by name
  LogAppenderBase::SharedPtr GetAppender(const std::string &appender_name);
  // delete a log appender to the logger
  void DeleteAppender(const std::string &appender_name);
  // clear all log appenders
  void ClearAppenders() { log_appenders_.clear(); }
  const std::string &GetName() { return name_; }

 private:
  std::string name_;
  std::shared_mutex shared_mutex_;
  // the list of logappenders
  std::list<LogAppenderBase::SharedPtr> log_appenders_;
};

class ConsoleLogAppender : public LogAppenderBase {
 public:
  ConsoleLogAppender() = default;

 private:
  void Log(LogLevel::Level level, LogEvent::SharedPtr event) override;
};

class FileLogAppender : public LogAppenderBase {
 public:
  FileLogAppender(const std::string &file_name);
  FileLogAppender(std::string file_name, const bool is_async);
  ~FileLogAppender();

 private:
  bool is_async_ = false;
  std::thread async_log_writter_;
  std::unique_ptr<BlockDeque<std::string>> log_string_buf_;
  const std::string file_name_;
  bool ReopenFile();
  std::ofstream file_stream_;
  void Log(LogLevel::Level level, LogEvent::SharedPtr event) override;
};

class LoggerManager : public Singleton<LoggerManager> {
  friend class Singleton;

 public:
  ~LoggerManager();
  bool AddLogger(const std::shared_ptr<Logger> &logger);
  bool DeleteLogger(const std::shared_ptr<Logger> &logger);
  bool DeleteLogger(const std::string &logger_name);
  void ClearLoggers();
  const std::shared_ptr<Logger> &GetLogger(const std::string &logger_name);

 private:
  std::shared_ptr<Logger> root_logger_;
  std::map<std::string, std::shared_ptr<Logger>> loggers_;
  LoggerManager();
};

}  // end namespace xac
