#include "logger.h"
#include <iostream>

namespace xac {
auto LogLevel::ToLevel(const std::string &level_str) -> LogLevel::Level {
  auto level_str_upper = const_cast<std::string &>(level_str);
  std::transform(level_str_upper.begin(), level_str_upper.end(), level_str_upper.begin(), ::toupper);
#define XX(L) \
  if (#L == level_str_upper) return LogLevel::Level::L;
  XX(DEBUG)
  XX(INFO)
  XX(WARN)
  XX(ERROR)
  XX(FATAL)
#undef XX
  return LogLevel::Level::UNKNOWN;
}

constexpr auto LogLevel::ToString(LogLevel::Level level) -> const char * {
  switch (level) {
#define XX(L)              \
  case LogLevel::Level::L: \
    return #L;
    XX(DEBUG)
    XX(INFO)
    XX(WARN)
    XX(ERROR)
    XX(FATAL)
#undef XX
    default:
      return "UNKNOWN";
  }
}

Logger::Logger(const std::string &name) : name_(name) {}

Logger::~Logger() { log_appenders_.clear(); }

void Logger::AddAppender(const LogAppenderBase::SharedPtr &appender) {
  std::unique_lock guard(shared_mutex_);
  log_appenders_.push_back(appender);
}

// //TODO:thread safe get appender
// LogAppenderBase::SharedPtr Logger::GetAppender(const std::string&
// appender_name) {
//     auto guard = std::unique_lock(shared_mutex_);
//     auto appender = log_appenders_.find(appender_name);
//     if (appender == log_appenders_.end()) {
//         return nullptr;
//     } else {
//         return appender->second;
//     }
// }
//
// void Logger::DeleteAppender(const std::string& appender_name) {
//     auto guard = std::unique_lock(shared_mutex_);
//     log_appenders_.erase(appender_name);
// }

void Logger::Log(const LogEvent::SharedPtr &event) {
  auto event_level = event->GetLevel();
  auto guard = std::unique_lock(shared_mutex_);
  for (const auto &it : log_appenders_) {
    it->Log(event_level, event);
  }
}

void LogEvent::Format(const char *format, ...) {
  va_list valist;
  va_start(valist, format);
  char *buf = nullptr;
  int len = vasprintf(&buf, format, valist);
  if (len != -1) {
    content_ss_ << std::string(buf, len);
    free(buf);
  }
  va_end(valist);
}

LogEvent::LogEvent(const char *file_name, const uint64_t &time, const uint32_t &elapse, const uint32_t &line,
                   const uint32_t &thread_id, std::string thread_name, const uint32_t &fiber_id,
                   std::string logger_name, LogLevel::Level level)
    : file_name_(file_name),
      time_(time),
      elapse_(elapse),
      line_(line),
      thread_id_(thread_id),
      thread_name_(std::move(thread_name)),
      fiber_id_(fiber_id),
      logger_name_(std::move(logger_name)),
      level_(level) {}

LogEventWrap::~LogEventWrap() { LoggerManager::GetInstance()->GetLogger(event_->logger_name_)->Log(event_); }

const std::string Formatter::COMPLEXPATTERN = "[%p]%d{%Y-%m-%d %H:%M:%S}%T(tid)%t%T[%c]%T%f:%l%T%m%n";
// const std::string Formatter::COMPLEXPATTERN = "[%p]%d{%Y-%m-%d
// %H:%M:%S}%T(tid)%t%T(tname)%N%T(fid)%F%T[%c]%T%f:%l%T%m%n";
const std::string Formatter::SIMPLEPATTERN = "[%p]%T[%c]%T%f:%l%T%m%n";

Formatter::Formatter(std::string pattern) : pattern_(std::move(pattern)) { PatternParse(); }

void Formatter::Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) {
  for (const auto &it : format_items_) {
    it->Format(os, level, event);
  }
}

LogAppenderBase::LogAppenderBase() { formatter_ = std::make_shared<Formatter>(); }

FileLogAppender::~FileLogAppender() {
  if (is_async_ && async_log_writter_.joinable()) {
    async_log_writter_.detach();
  }
}

void LogAppenderBase::SetFormatter(const Formatter::SharedPtr &formatter) {
  auto guard = std::unique_lock(shared_mutex_);
  formatter_ = formatter;
}

auto LogAppenderBase::GetFormatter() -> Formatter::SharedPtr {
  auto guard = std::shared_lock(shared_mutex_);
  return formatter_;
}

void LogAppenderBase::SetLevel(LogLevel::Level level) {
  auto guard = std::unique_lock(shared_mutex_);
  level_ = level;
}

auto LogAppenderBase::GetLevel() -> LogLevel::Level {
  auto guard = std::shared_lock(shared_mutex_);
  return level_;
}

FileLogAppender::FileLogAppender(std::string file_name, const bool is_async)
    : is_async_(is_async), file_name_(std::move(file_name)) {
  if (file_stream_.is_open()) {
    file_stream_.close();
  }
  file_stream_.open(file_name_);
  if (is_async) {
    log_string_buf_ = std::make_unique<BlockDeque<std::string>>();
    async_log_writter_ = std::thread([&]() {
      std::string str;
      while (log_string_buf_->pop(str)) {
        file_stream_ << str;
      }
    });
  }
}

FileLogAppender::FileLogAppender(const std::string &file_name) : FileLogAppender(file_name, false) {}

auto FileLogAppender::ReopenFile() -> bool {
  if (file_stream_.is_open()) {
    file_stream_.close();
  }
  file_stream_.open(file_name_, std::ios_base::app);
  return file_stream_.is_open();
}

void FileLogAppender::Log(LogLevel::Level level, LogEvent::SharedPtr event) {
  if (is_async_) {
    std::stringstream temp_buf;
    if (level >= level_) {
      formatter_->Format(temp_buf, level, event);
    }
    log_string_buf_->push_back(temp_buf.str());
  } else {
    ReopenFile();
    if (level >= level_) {
      formatter_->Format(file_stream_, level, event);
    }
  }
}

void ConsoleLogAppender::Log(LogLevel::Level level, LogEvent::SharedPtr event) {
  if (level >= level_) {
    switch (level) {
      case LogLevel::Level::DEBUG:
        std::cout << "\033[1;32m";
        break;
      case LogLevel::Level::INFO:
        std::cout << "\033[1;36m";
        break;
      case LogLevel::Level::WARN:
        std::cout << "\033[1;33m";
        break;
      case LogLevel::Level::ERROR:
        std::cout << "\033[1;31m";
        break;
      case LogLevel::Level::FATAL:
        std::cout << "\033[1;41m";
        break;
      default:
        break;
    }
    formatter_->Format(std::cout, level, event);
    std::cout << "\033[0m";
  }
}

class ContentFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit ContentFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << event->GetContent();
  }  // namespace xac
};

class FileNameFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit FileNameFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << event->GetFileName();
  }
};

class TimeFormatItem : public Formatter::FormatItemBase {
 public:
  explicit TimeFormatItem(std::string time_format) : time_format_(std::move(time_format)) {
    if (time_format_.empty()) {
      time_format_ = "%Y-%m-%d %H:%M:%S";
    }
  }
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    struct tm tm_struct;
    auto time = static_cast<time_t>(event->GetTime());
    localtime_r(&time, &tm_struct);
    char buf[64];
    strftime(buf, sizeof(buf), time_format_.c_str(), &tm_struct);
    os << buf;
  }

 private:
  std::string time_format_;
};

class ElapseFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit ElapseFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << event->GetFileName();
  }
};

class LineFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit LineFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << event->GetLine();
  }
};

class ThreadIdFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit ThreadIdFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << event->GetThreadId();
  }
};

class ThreadNameFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit ThreadNameFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << event->GetThreadName();
  }
};

class FiberIdFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit FiberIdFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << event->GetFiberId();
  }
};

class StringFormatItem : public Formatter::FormatItemBase {
 public:
  explicit StringFormatItem(const std::string &string_content = "") : string_content_(string_content) {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << string_content_;
  }

 private:
  std::string string_content_;
};

class LevelFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit LevelFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << LogLevel::ToString(level);
  }
};

class LoggerNameFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit LoggerNameFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override {
    os << event->GetLoggerName();
  }
};

class NewLineFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit NewLineFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override { os << std::endl; }
};

class TabFormatItem : public Formatter::FormatItemBase {
 public:
  // In order to use map to init, `str` never used
  explicit TabFormatItem(const std::string &str = "") {}
  void Format(std::ostream &os, LogLevel::Level level, const LogEvent::SharedPtr &event) override { os << "  "; }
};

void Formatter::PatternParse() {
  // std::cout << "-----------start parse-----------" << std::endl;
  //  item: FormatItemBase, the character after %
  //  fmt: the parameter of FormatItemBase's Constructor, which is the character
  //  in {} type: 0 for StringItem, 1 for others
  std::vector<std::tuple<std::string /*item*/, std::string /*format*/, int /*type*/>> pattern_parsed;
  std::string content_str;  // not the formatted string, just content
  for (auto str_left = pattern_.begin(); str_left != pattern_.end();) {
    // char, have not met escape
    if (*str_left != '%') {
      content_str += *str_left;
      ++str_left;
      continue;
    }
    // met escape
    if ((++str_left != pattern_.end()) && *str_left == '%') {  // if %%
      content_str += *str_left;
      continue;
    }
    auto str_right = str_left;
    std::string item_str;         // the string after % before {
    std::string item_format_str;  // the string in {}
    int parse_state = 0;          // 0 means parsing item, 1 means parsing fmt
    while (str_left != pattern_.end()) {
      // parse item
      if (parse_state == 0) {
        if (!isalpha(*str_right) && *str_right != '{' &&
            *str_right != '}') {  // current item has been parsed, get to next
          item_str = std::string(str_left, str_right);
          str_left = str_right;
          break;
        }
        if (*str_right == '{') {
          item_str = std::string(str_left, str_right);
          str_left = ++str_right;
          parse_state = 1;  // start parsing fmt
        }
      }
      // parse fmt
      if (parse_state == 1 && *str_right == '}') {
        item_format_str = std::string(str_left, str_right);
        str_left = str_right + 1;
        parse_state = 0;  // end parsing fmt
        break;
      }
      ++str_right;
      // last item
      if (str_right == pattern_.end() && item_str.empty()) {
        item_str = std::string(str_left, str_right);
        str_left = str_right;
      }
    }
    if (parse_state == 1 || item_str.empty()) {  // error item
    }
    if (!content_str.empty()) {
      pattern_parsed.emplace_back(content_str, "", 0);
      content_str.clear();
    }
    pattern_parsed.emplace_back(item_str, item_format_str, 1);
    item_str.clear();
    item_format_str.clear();
  }
  if (!content_str.empty()) {
    pattern_parsed.emplace_back(content_str, "", 0);
  }
  // static map from char to item constructor
  static std::map<std::string, std::function<FormatItemBase::SharedPtr(const std::string &)>> char_to_item_map = {
#define XX(STR, CLASS)                                                                     \
  {                                                                                        \
#STR, [](const std::string &str) { return FormatItemBase::SharedPtr(new CLASS(str)); } \
  }

      XX(m, ContentFormatItem),    XX(p, LevelFormatItem),      XX(r, ElapseFormatItem),
      XX(c, LoggerNameFormatItem),  // logger name
      XX(t, ThreadIdFormatItem),   XX(n, NewLineFormatItem),    XX(d, TimeFormatItem),
      XX(f, FileNameFormatItem),   XX(l, LineFormatItem),       XX(T, TabFormatItem),
      XX(F, FiberIdFormatItem),    XX(N, ThreadNameFormatItem),
#undef XX
  };
  // add to format_items
  for (auto &i : pattern_parsed) {
    if (std::get<2>(i) == 0) {
      format_items_.push_back(FormatItemBase::SharedPtr(new StringFormatItem(std::get<0>(i))));
    } else {
      auto it = char_to_item_map.find(std::get<0>(i));
      if (it == char_to_item_map.end()) {
        format_items_.push_back(
            FormatItemBase::SharedPtr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
      } else {
        format_items_.push_back(it->second(std::get<1>(i)));
      }
    }
    // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") -
    // (" << std::get<2>(i) << ")" << std::endl;
  }
  // std::cout << "-----------end parse-----------" << std::endl;
}

auto LoggerManager::AddLogger(const std::shared_ptr<Logger> &logger) -> bool {
  auto logger_name = logger->GetName();
  if (loggers_.find(logger_name) == loggers_.end()) {
    loggers_.insert(std::pair(logger_name, logger));
    return true;
  }
  return false;
}

auto LoggerManager::DeleteLogger(const std::shared_ptr<Logger> &logger) -> bool {
  auto logger_name = logger->GetName();
  if (loggers_.find(logger_name) != loggers_.end()) {
    loggers_.erase(logger_name);
    return true;
  }
  return false;
}

auto LoggerManager::DeleteLogger(const std::string &logger_name) -> bool {
  if (loggers_.find(logger_name) != loggers_.end()) {
    loggers_.erase(logger_name);
    return true;
  }
  return false;
}
void LoggerManager::ClearLoggers() { loggers_.clear(); }
auto LoggerManager::GetLogger(const std::string &logger_name) -> const std::shared_ptr<Logger> & {
  auto it = loggers_.find(logger_name);
  if (it != loggers_.end()) {
    return it->second;
  }
  LRERROR << "No logger named " << logger_name;
  return root_logger_;
}

LoggerManager::LoggerManager() {
  root_logger_ = std::make_shared<Logger>("root");
  ConsoleLogAppender::SharedPtr stdout_log_appender(new ConsoleLogAppender());
  root_logger_->AddAppender(stdout_log_appender);
  AddLogger(root_logger_);
}

LoggerManager::~LoggerManager() { loggers_.clear(); }
};  // namespace xac
