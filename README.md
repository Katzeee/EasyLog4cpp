# EasyLog4cpp
Simple Logger for cpp.

Inspired by [sylar](https://github.com/sylar-yin/sylar), [log4cpp](https://log4cpp.sourceforge.net). 

The whole structure is basically based on `sylar`, adding some new features.

- Use `shared_mutex` to instead `pthread_rwlock`.

- Add different colors for different levels of log in console.

- Async write to file.