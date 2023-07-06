#ifndef LOGLOG_H
#define LOGLOG_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/timeb.h>
#include <sys/syscall.h>

#include <mutex>
#include <string>

#define ERROR_LEVEL 0
#define WARN_LEVEL  1
#define INFO_LEVEL  2
#define DEBUG_LEVEL 3
#define LOG_LEVEL   4

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define __THREADID__ (int)syscall(SYS_gettid)

class LogTitle
{
public:
    static const char* get()
    {
        return title.data();
    }

    static void set(const char* t)
    {
        title = t;
    }

private:
    inline static std::string title;
};

class LogLevel
{
public:
    static int get()
    {
        return level;
    }

    static int set(int l)
    {
        level = l;
    }

private:
    inline static int level=LOG_LEVEL;
};

class PrintLock
{
public:
    static std::mutex& get()
    {
        static std::mutex lock;
        return lock;
    }
};

class ThreadColor
{
public:
    enum Color {Green,Yellow,Pink,Teal,BoldOrange,BoldGreen,BoldYellow,BoldBlue,BoldPink,BoldTeal};

    static ThreadColor& getInstance()
    {
        static thread_local ThreadColor t;

        return t;
    }

    void set()
    {
        switch(my_color)
        {
        case Green:     fprintf(stderr, "\033[0;32m"); break;
        case Yellow:    fprintf(stderr, "\033[0;33m"); break;
        case Pink:      fprintf(stderr, "\033[0;35m"); break;
        case Teal:      fprintf(stderr, "\033[0;36m"); break;
        case BoldOrange:fprintf(stderr, "\033[31;1m"); break;
        case BoldGreen: fprintf(stderr, "\033[32;1m"); break;
        case BoldYellow:fprintf(stderr, "\033[33;1m"); break;
        case BoldBlue:  fprintf(stderr, "\033[34;1m"); break;
        case BoldPink:  fprintf(stderr, "\033[35;1m"); break;
        case BoldTeal:  fprintf(stderr, "\033[36;1m"); break;
        default:        fprintf(stderr, "\033[0;37"); break;
        }
    }

    void reset()
    {
        fprintf(stderr, "\033[0m");
    }

private:
    int my_color;

    ThreadColor()
    {
        static int last_color = Green;

        static std::mutex lock;
        std::lock_guard<std::mutex> l(lock);

        last_color = ++last_color>BoldTeal? Green:last_color;

        my_color = last_color;
    }
};

class CallLevelKeeper
{
public:
    CallLevelKeeper()
    {
        if (LogLevel::get() >= INFO_LEVEL)
        {
            (*getCallLevel())++;
        }
    }
    ~CallLevelKeeper()
    {
        if (LogLevel::get() >= INFO_LEVEL)
        {
            std::lock_guard<std::mutex> l(PrintLock::get());

            struct tm *__now;
            struct timeb __tb;
            ftime(&__tb);
            __now = localtime(&__tb.time);
            fprintf(stderr, "%02d:%02d:%02d:%03d [%s][CALL]: ",
                    __now->tm_hour, __now->tm_min, __now->tm_sec, __tb.millitm, LogTitle::get());

            ThreadColor::getInstance().set();

            fprintf(stderr, "%d:%s", __THREADID__, std::string((*getCallLevel())*2, ' ').c_str());
            fprintf(stderr, "<<%s()\n", mFuncName.c_str());
            fflush(stderr);

            ThreadColor::getInstance().reset();

            (*getCallLevel())--;
        }
    }

    void setFuncName(const std::string& funcName)
    {
        if (LogLevel::get() >= INFO_LEVEL)
        {
            mFuncName = funcName;
        }
    }

    static uint* getCallLevel()
    {
        static thread_local uint callLevel = 0;

        return &callLevel;
    }

private:
    std::string mFuncName;
};

#define CALL_START(...)                                                            \
    CallLevelKeeper callLevelKeeper;                                               \
    do{                                                                            \
        if (LogLevel::get() >= INFO_LEVEL)                                         \
        {                                                                          \
            struct tm *__now;                                                      \
            struct timeb __tb;                                                     \
            ftime(&__tb);                                                          \
            __now = localtime(&__tb.time);                                         \
            {                                                                      \
            std::lock_guard<std::mutex> l(PrintLock::get());                       \
            fprintf(stderr, "%02d:%02d:%02d:%03d [%s][CALL]: ",                    \
                    __now->tm_hour, __now->tm_min, __now->tm_sec, __tb.millitm,LogTitle::get());\
            ThreadColor::getInstance().set();                                      \
            std::string func_name = __PRETTY_FUNCTION__;                           \
            func_name = func_name.substr(0,func_name.find("("));                   \
            func_name = func_name.substr(func_name.find_last_of(' ')+1);           \
            callLevelKeeper.setFuncName(func_name);                                \
            fprintf(stderr, "%d:%s", __THREADID__,                                 \
                    std::string((*CallLevelKeeper::getCallLevel())*2, ' ').c_str());\
            fprintf(stderr, ">>%s(", func_name.c_str());                           \
            fprintf(stderr, ##__VA_ARGS__);                                        \
            fprintf(stderr, ") ----%s:%d\n", __FILENAME__, __LINE__);              \
            fflush(stderr);                                                        \
            ThreadColor::getInstance().reset();                                    \
            }                                                                      \
        }                                                                          \
    }while(0)

#define A_PRINT(type,...)                                                    \
{                                                                            \
    struct tm *__now;                                                        \
    struct timeb __tb;                                                       \
    ftime(&__tb);                                                            \
    __now = localtime(&__tb.time);                                           \
                                                                             \
    fprintf(stderr, "%02d:%02d:%02d:%03d [%s][%s]: ",                        \
            __now->tm_hour, __now->tm_min, __now->tm_sec, __tb.millitm,LogTitle::get(),type);\
                                                                             \
    ThreadColor::getInstance().set();                                        \
                                                                             \
    fprintf(stderr, "%d:%s  \"", __THREADID__,                               \
            std::string((*CallLevelKeeper::getCallLevel())*2, ' ').c_str()); \
    fprintf(stderr, ##__VA_ARGS__);                                          \
    fprintf(stderr, "\" ----%s:%d\n", __FILENAME__, __LINE__);               \
    fflush(stderr);                                                          \
                                                                             \
    ThreadColor::getInstance().reset();                                      \
}

#define A_ERROR(...)                                    \
    do{                                                 \
        if(LogLevel::get() >= ERROR_LEVEL)              \
        {                                               \
            std::lock_guard<std::mutex> l(PrintLock::get());\
            fprintf(stderr, "\e[31m");                  \
            A_PRINT(" ERR",__VA_ARGS__)                 \
        }                                               \
    }while(0)

#define A_WARN(...)                                     \
    do{                                                 \
        if(LogLevel::get() >= WARN_LEVEL)               \
        {                                               \
            std::lock_guard<std::mutex> l(PrintLock::get());\
            fprintf(stderr, "\e[33m");                  \
            A_PRINT("WARN",__VA_ARGS__)                 \
        }                                               \
    }while(0)

#define A_INFO(...)                                     \
    do{                                                 \
        if (LogLevel::get() >= INFO_LEVEL)              \
        {                                               \
            std::lock_guard<std::mutex> l(PrintLock::get());\
            A_PRINT("INFO",__VA_ARGS__)                 \
        }                                               \
    }while(0)

#define A_DEBUG(...)                                    \
    do{                                                 \
        if (LogLevel::get() >= DEBUG_LEVEL)             \
        {                                               \
            std::lock_guard<std::mutex> l(PrintLock::get());\
            A_PRINT("DBUG",__VA_ARGS__)                 \
        }                                               \
    }while(0)

#define A_LOG(...)                                      \
    do{                                                 \
        if (LogLevel::get() >= LOG_LEVEL)               \
        {                                               \
            std::lock_guard<std::mutex> l(PrintLock::get());\
            A_PRINT(" LOG",__VA_ARGS__)                 \
        }                                               \
    }while(0)

#endif

