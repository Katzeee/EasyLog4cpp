#include <thread>   
#include "logger.h"
using namespace xac;


#define LILOG(logger_name, event_level) xac::LogEventWrap::SharedPtr( \
    new xac::LogEventWrap(xac::LogEvent::SharedPtr( \
    new xac::LogEvent(__FILE__, time(NULL), 0, __LINE__, \
    xac::GetThreadId(), xac::GetThreadName(), xac::GetFiberId(), \
    logger1, event_level))))->GetStringStream() 

int main() {
    LoggerManager::Instance();
    // auto logger = LoggerManager::GetInstance()->GetLogger("root");
    auto logger = std::make_shared<Logger>("test");
    //auto appender = std::make_shared<ConsoleLogAppender>("hi");
    auto appender = std::make_shared<FileLogAppender>("a.log", true);
    logger->AddAppender(appender);
    LoggerManager::GetInstance()->AddLogger(logger);
    auto t = std::thread([logger](){
        auto i = 1000;
        while(i--) {
            LDEBUG("test") << i;
        }
    });
    auto t1 = std::thread([logger](){
        auto i = 1000;
        while(i--) {
            LLOG("test", LogLevel::INFO) << i;
        }
    });
    auto t2 = std::thread([logger](){
        auto i = 1000;
        while(i--) {
            LLOG("test", LogLevel::WARNING) << i;
        }
    });
    t2.join();
    t.join();
    t1.join();
    LoggerManager::DestroyInstance();
    return 0;
}
