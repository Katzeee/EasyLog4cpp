#include <thread>   
#include "logger.h"
using namespace xac;

void filelog_test() {

    auto logger = std::make_shared<Logger>("test");

    auto appender = std::make_shared<FileLogAppender>("test.log", true);

    logger->AddAppender(appender);

    LoggerManager::GetInstance()->AddLogger(logger);

    auto t = std::thread([](){
        auto i = 10000;
        while(i--) {
            LDEBUG("test") << i;
        }
    });
    auto t1 = std::thread([](){
        auto i = 10000;
        while(i--) {
            LLOG("test", LogLevel::INFO) << i;
        }
    });
    auto t2 = std::thread([](){
        auto i = 10000;
        while(i--) {
            LLOG("test", LogLevel::WARN) << i;
        }
    });
    t2.join();
    t1.join();
    t.join();
}


void rootlogger_test() {
    LRDEBUG << "debug";
    LRINFO << "info";
    LRWARN << "warn";
    LRERROR << "error";
    LRFATAL << "FATAL";
}

void formatlog_test() {
    FLDEBUG("root", "%d + %d = %d", 1, 1, 2);
}

void formatter_test() {
    auto appender = std::make_shared<ConsoleLogAppender>();
    auto formatter = std::make_shared<Formatter>(Formatter::COMPLEXPATTERN);
    appender->SetFormatter(formatter);
    LoggerManager::GetInstance()->GetLogger("root")->AddAppender(appender);

    LRDEBUG << "test new format";


}

void createlogger_test() {
    // create a logger named new
    auto logger = std::make_shared<Logger>("new");

    // create an appender
    auto appender = std::make_shared<ConsoleLogAppender>();

    // add the appender to logger
    logger->AddAppender(appender);

    //add logger to logger manager
    LoggerManager::GetInstance()->AddLogger(logger);

    LDEBUG("new") << "this is from new logger";
}


int main() {
    LoggerManager::Instance();


    rootlogger_test();

    formatlog_test();

    createlogger_test();

    formatter_test();

    filelog_test();

    LoggerManager::DestroyInstance();
    return 0;
}
