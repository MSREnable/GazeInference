#ifndef __X_LOGGING_H__
#define __X_LOGGING_H__

#define LOG_VERBOSE(...)
#define LOG_DEBUG(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)

// _WIN32, __unix__, __APPLE__, __linux__, __FreeBSD__, __ANDROID__

#ifdef __ANDROID__

#include <android/log.h>

#define LOG_TAG "TAG"
#ifndef NDEBUG // Only expose other log values in debug
#define LOG_VERBOSE(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOG_DEBUG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOG_WARN(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#endif
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


#elif __APPLE__

#include <CoreFoundation/CoreFoundation.h>

extern "C" {
    void NSLog(CFStringRef format, ...);
    void CLSLog(CFStringRef format, ...);
}

#ifdef DEBUG // Only expose other log values in debug
#define LOG_VERBOSE(format, ...) NSLog(CFSTR(format), ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) NSLog(CFSTR(format), ##__VA_ARGS__)
#define LOG_WARN(format, ...) NSLog(CFSTR(format), ##__VA_ARGS__)
#define LOG_ERROR(format, ...) NSLog(CFSTR(format), ##__VA_ARGS__)
#else
#define LOG_ERROR(format, ...) CLSLog(CFSTR(format), ##__VA_ARGS__)
#endif

#elif _WIN32

#include <windows.h>
#include <stdio.h>

template<typename... Args> void DebugPrint(const char* type, const char* format, Args... args) {
    size_t label_size = snprintf(NULL, 0, "[%s] ", type);
    size_t message_size = snprintf(NULL, 0, format, args...);

    // Create msg using formatting
    char* buffer = (char*)calloc(label_size + message_size + 1, sizeof(char)); // +1 for /0
    sprintf_s(buffer, label_size + 1, "[%s] ", type);
    sprintf_s(buffer + label_size, message_size + 1, format, args...);
    OutputDebugStringA(buffer);
}


#define LOG_VERBOSE(format, ...) DebugPrint("VERBOSE", format, ##__VA_ARGS__);
#define LOG_DEBUG(format, ...) DebugPrint("DEBUG", format, ##__VA_ARGS__);
#define LOG_WARN(format, ...) DebugPrint("WARNING",format, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) DebugPrint("ERROR", format, ##__VA_ARGS__);

#else // Non-mobile platform

#include <iostream>

#define LOG_VERBOSE(...) std::clog << __VA_ARGS__ << std::endl;
#define LOG_DEBUG(...) std::clog << __VA_ARGS__ << std::endl;
#define LOG_WARN(...) std::clog << __VA_ARGS__ << std::endl;
#define LOG_ERROR(...) std::cerr << __VA_ARGS__ << std::endl;

#endif

#endif // __X_LOGGING_H__