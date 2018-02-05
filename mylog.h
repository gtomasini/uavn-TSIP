#ifndef MYLOG_H
#define MYLOG_H
#include <time.h> // time_t, tm, time, localtime, strftime
#include <stdio.h>
#include <string.h>

/* run this program using the console pauser or add your own getch, system("pause") or input loop */

// Returns the local date/time formatted as 2014-03-19 11:11:52
char* getFormattedTime(void);

// Remove path from filename
#define __SHORT_FILE__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// Main log macro
#define __LOG__(format, loglevel, ...) printf("%s %-5s [%s]" format "\n", getFormattedTime(), loglevel, __func__, ## __VA_ARGS__)
#define __LOGBALD__(format, ...) printf( format, ## __VA_ARGS__)

// Specific log macros with 
#define LOGBALD(format, ...) __LOGBALD__(format, ## __VA_ARGS__)
#define LOGDEBUG(format, ...) __LOG__(format, "DEBUG", ## __VA_ARGS__)
#define LOGDEBUG2(format, ...) __LOG__(format, "DEBUG2", ## __VA_ARGS__)
#define LOGDEBUG3(format, ...) __LOG__(format, "DEBUG3", ## __VA_ARGS__)
#define LOGWARN(format, ...) __LOG__(format, "WARN", ## __VA_ARGS__)
#define LOGERROR(format, ...) __LOG__(format, "ERROR", ## __VA_ARGS__)
#define LOGINFO(format, ...) __LOG__(format, "INFO", ## __VA_ARGS__)

#endif


