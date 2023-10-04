#if INTERFACE
#define INFOFD stderr
#endif

#include "log.h"

#define TRACE_ENTRY
#define TRACE_LOG(fmt, ...)
#define TRACE_EXIT
#define TRACE_ENTRY_MSG(fmt, ...)
#define TRACE_S7_DUMP(msg, x)

#define LOG_DEBUG(lvl, fmt, ...)
#define LOG_ERROR(lvl, fmt, ...)
#define LOG_INFO(lvl, fmt, ...)
#define LOG_TRACE(lvl, fmt, ...)
#define LOG_WARN(lvl, fmt, ...)

