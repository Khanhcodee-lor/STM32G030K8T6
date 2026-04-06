#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <stdarg.h>
#include <stdio.h>

void DebugLog_Init(void);
void DebugLog_Printf(const char *fmt, ...);

#ifdef DEBUG
#define DEBUG_LOG(...) DebugLog_Printf(__VA_ARGS__)
#else
#define DEBUG_LOG(...)
#endif

#endif /* DEBUG_LOG_H */
