#pragma once

#include <functional>
#include <tuple>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <list>
#include <ctime>
#include <cstring>
#include <cstdarg>
#include <algorithm>
#include <thread>
#include "bq.h"



#define LLOG(logger_name, event_level) xac::LogEventWrap::SharedPtr( \
    new xac::LogEventWrap(xac::LogEvent::SharedPtr( \
    new xac::LogEvent(__FILE__, time(NULL), 0, __LINE__, \
    xac::GetThreadId(), xac::GetThreadName(), xac::GetFiberId(), \
    logger_name, event_level))))->GetStringStream() 

#define LDEBUG(logger_name) LLOG(logger_name, xac::LogLevel::Level::DEBUG)
#define LINFO(logger_name) LLOG(logger_name, xac::LogLevel::Level::INFO)
#define LWARNING(logger_name) LLOG(logger_name, xac::LogLevel::Level::WARNING)
#define LERROR(logger_name) LLOG(logger_name, xac::LogLevel::Level::ERROR)
#define LFATAL(logger_name) LLOG(logger_name, xac::LogLevel::Level::FATAL)

#define LRDEBUG LDEBUG("root")
#define LRINFO LINFO("root")
#define LRWARNING LWARNING("root")
#define LRERROR LERROR("root")
#define LRFATAL LFATAL("root")


#define FLLOG(logger_name, event_level, format, ...) xac::LogEventWrap::SharedPtr( \
    new xac::LogEventWrap(xac::LogEvent::SharedPtr( \
    new xac::LogEvent(__FILE__, time(NULL), 0, __LINE__, \
    xac::GetThreadId(), xac::GetThreadName(), xac::GetFiberId(), \
    xac::LoggerManager::GetInstance().GetLogger(logger_name), \
    event_level))))->GetEvent()->Format(format, __VA_ARGS__) 
#define FLDEBUG(logger_name, format, ...) FLLOG(logger_name, \
    xac::LogLevel::Level::DEBUG, format, __VA_ARGS__)
#define FLINFO(logger_name, format, ...) FLLOG(logger_name, \
    xac::LogLevel::Level::INFO, format, __VA_ARGS__)
#define FLWARNING(logger_name, format, ...) FLLOG(logger_name, \
    xac::LogLevel::Level::WARNING, format, __VA_ARGS__)
#define FLERROR(logger_name, format, ...) FLLOG(logger_name, \
    xac::LogLevel::Level::ERROR, format, __VA_ARGS__)
#define FLFATAL(logger_name, format, ...) FLLOG(logger_name, \
    xac::LogLevel::Level::FATAL, format, __VA_ARGS__)

#define FLRDEBUG(format, ...) FLDEBUG("root", format, __VA_ARGS__)
#define FLRINFO(format, ...) FLINFO("root", format, __VA_ARGS__)
#define FLRWARNING(format, ...) FLWARNING("root", fromat, __VA_ARGS__)
#define FLRERROR(format, ...) FLERROR("root", format, _VA__ARGS__)
#define FLRFATAL(format, ...) FLFATAL("root", format, _VA_ARGS__)


namespace xac {

template<typename T>
class Singleton {
public:
    template<typename... Args>
    static T* Instance(Args&&... args) {
        if (instance_ == nullptr) {
            instance_ = new T(std::forward(args)...);
        }
        return instance_;
    }
    static T* GetInstance() {
        if (instance_ == nullptr) {
            throw std::logic_error("No instance!");
        }
        return instance_;
    }
    static void DestroyInstance() {
        delete instance_;
        instance_ = nullptr;
    }
private:
    static T* instance_;

};

template<typename T>
T* Singleton<T>::instance_ = nullptr;



std::string GetThreadName();
int GetThreadId();
int GetFiberId();


class Logger;
class LogAppenderBase;
class LoggerManager;

class LogLevel {
public:
    enum Level {
        UNKNOWN = 0,
        DEBUG = 1,
        INFO = 2,
        WARNING = 3,
        ERROR = 4,
        FATAL = 5,
  };
  static Level ToLevel(const std::string& level_str); 
  static const std::string ToString(Level level); 
};

class LogEvent {
friend class LogEventWrap;
public:
    typedef std::shared_ptr<LogEvent> SharedPtr;
    LogEvent() = delete;
    LogEvent(const char* file_name, const uint64_t& time, 
             const uint32_t& elapse, const uint32_t& line, 
             const uint32_t& thread_id, const std::string& thread_name,
             const uint32_t& fiber_id, const std::string logger_name,
             LogLevel::Level level);
    const char* GetFileName() const { return file_name_; }
    const uint64_t& GetTime() const { return time_; }
    const uint32_t& GetElapse() const { return elapse_; }
    const uint32_t& GetLine() const { return line_; }
    const uint32_t& GetThreadId() const { return thread_id_; }
    const std::string& GetThreadName() const { return thread_name_; }
    const uint32_t& GetFiberId() const { return fiber_id_; }
    const std::string GetContent() const { return content_ss_.str(); }
    const LogLevel::Level GetLevel() const { return level_; }
    const std::string GetLoggerName() { return logger_name_; }
    std::stringstream& GetStringStream() { return content_ss_; }
    void Format(const char* format, ...);
private:
    const char* file_name_; // file name
    uint64_t time_; // timestamp
    uint32_t elapse_; // elapsed time from program run
    uint32_t line_; // line number
    uint32_t thread_id_; // thread id
    std::string thread_name_; // thread name
    uint32_t fiber_id_; // fiber id
    std::stringstream content_ss_; // content
    std::string logger_name_;
    LogLevel::Level level_;

};

class LogEventWrap {
public:
    typedef std::shared_ptr<LogEventWrap> SharedPtr;
    LogEventWrap(LogEvent::SharedPtr event) : event_(event) {}
    ~LogEventWrap(); 
    LogEvent::SharedPtr GetEvent() { return event_; }
    std::stringstream& GetStringStream() { return event_->GetStringStream(); }
private:
    LogEvent::SharedPtr event_;
};



class Formatter {
public:
    typedef std::shared_ptr<Formatter> SharedPtr;
    Formatter() { PatternParse(); }
    Formatter(const std::string& pattern);
    // output the event as formatted
    void Format(
        std::ostream& os, LogLevel::Level level, LogEvent::SharedPtr event);
    class FormatItemBase { // every format item has `Format` method to output their content
    public:
        typedef std::shared_ptr<FormatItemBase> SharedPtr;
        //FormatItemBase(const std::string& str = "") {}
        virtual ~FormatItemBase() {}
        virtual void Format(
            std::ostream& os, LogLevel::Level level, LogEvent::SharedPtr event) = 0;
    };
private:
    std::string pattern_ = std::string("[%p]%d{%Y-%m-%d %H:%M:%S}%T(tid)%t%T(tname)%N%T(fid)%F%T[%c]%T%f:%l%T%m%n"); // the pattern of formatter
    std::vector<FormatItemBase::SharedPtr> format_items_; // formatted items
    void PatternParse(); // parse the pattern to format items

};



class LogAppenderBase {
friend class Logger;
public:
    using SharedPtr = std::shared_ptr<LogAppenderBase>;
    LogAppenderBase() : LogAppenderBase(false) {}
    LogAppenderBase(const bool is_async);
    virtual ~LogAppenderBase();
    // set the formatter of the appender
    void SetFormatter(Formatter::SharedPtr formatter);
    // get the formatter of the appender
    Formatter::SharedPtr GetFormatter();
    // set the level of the appender
    void SetLevel(LogLevel::Level level);
    // get the level of the appender
    const LogLevel::Level GetLevel();
protected:
    bool is_async_ = false;
    std::thread async_log_writter_;
    std::shared_mutex shared_mutex_;
    LogLevel::Level level_ = LogLevel::Level::DEBUG;
    Formatter::SharedPtr formatter_;
    // set the event level and log it 
    void Log(LogLevel::Level level, LogEvent::SharedPtr event);
    virtual void DoLog(LogLevel::Level level, LogEvent::SharedPtr event) = 0;
    virtual void DoLogAsync(LogLevel::Level level, LogEvent::SharedPtr event) = 0;
};



class Logger {
public:
    using SharedPtr = std::shared_ptr<Logger>;
    Logger() = delete;
    Logger(const std::string name);
    Logger(const Logger& logger) = delete;
    ~Logger();
    // log the event
    void Log(LogEvent::SharedPtr event);
    void AddAppender(LogAppenderBase::SharedPtr appender);
    // get log appender by name
    LogAppenderBase::SharedPtr GetAppender(const std::string& appender_name);
    // delete a log appender to the logger
    void DeleteAppender(const std::string& appender_name);
    // clear all log appenders
    void ClearAppenders() { log_appenders_.clear(); }
    const std::string& GetName() { return name_; }
private:
    std::string name_;
    std::shared_mutex shared_mutex_;
    // the list of logappenders
    std::list<LogAppenderBase::SharedPtr> log_appenders_;

};

class ConsoleLogAppender : public LogAppenderBase {
public:
    ConsoleLogAppender() : LogAppenderBase(false) {}
private:
    void DoLog(LogLevel::Level level, LogEvent::SharedPtr event) override;
    void DoLogAsync(LogLevel::Level level, LogEvent::SharedPtr event) override {}
};

class FileLogAppender : public LogAppenderBase {
public:
    FileLogAppender(const std::string& file_name);
    FileLogAppender(const std::string& file_name, const bool is_async);
private:
    std::unique_ptr<BlockDeque<std::string>> log_string_buf_;
    std::stringstream temp_buf_; // use for async write
    const std::string file_name_;
    bool ReopenFile();
    std::ofstream file_stream_;
    void DoLog(LogLevel::Level level, LogEvent::SharedPtr event) override;
    void DoLogAsync(LogLevel::Level level, LogEvent::SharedPtr event) override;
};

class LoggerManager : public Singleton<LoggerManager> {
friend class Singleton;
public:
    ~LoggerManager();
    bool AddLogger(std::shared_ptr<Logger> logger);
    bool DeleteLogger(std::shared_ptr<Logger> logger);
    bool DeleteLogger(const std::string& logger_name);
    void ClearLoggers();
    std::shared_ptr<Logger> GetLogger(const std::string& logger_name);
private:
    std::shared_ptr<Logger> root_logger_;
    std::map<std::string, std::shared_ptr<Logger> > loggers_;
    LoggerManager();

};


} // end namespace xac
