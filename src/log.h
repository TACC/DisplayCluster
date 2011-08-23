#ifndef LOG_H
#define LOG_H

#define LOG_DEBUG 1
#define LOG_INFO 2
#define LOG_WARN 3
#define LOG_ERROR 4
#define LOG_FATAL 5

#define LOG_THRESHHOLD 1

#define MAX_LOG_LENGTH 1024

extern void put_log(int level, const char *format, ...);

#define put_flog(l, fmt, ...) put_log(l, "%s: " fmt, __PRETTY_FUNCTION__, ##__VA_ARGS__)

#endif
