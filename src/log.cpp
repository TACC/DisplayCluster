#include "log.h"
#include <stdarg.h>
#include <stdio.h>

void put_log(int level, const char *format, ...)
{
    if(level < LOG_THRESHHOLD)
        return;

    char log_string[MAX_LOG_LENGTH];

    // actual log message
    va_list ap;
    va_start(ap, format);
    vsnprintf(log_string, MAX_LOG_LENGTH, format, ap);
    va_end(ap);

    printf("%s\n", log_string);

    return;
}
