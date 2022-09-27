#include <thread>   
#include "logger.h"
using namespace xac;




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
