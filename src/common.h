#pragma once
#include <string>
#include <optional>
#include <unistd.h>

namespace xac {

namespace utils {
    
} // end namespace utils


static std::string GetThreadName() { 
    return ""; 
}

static int GetThreadId() { 
    return gettid(); 
}

static int GetFiberId() { 
    return 1; 
}
    
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

} // end namespace xac


