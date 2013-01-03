#ifndef __debug_h__
#define __debug_h__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef NO_DEBUG
#define debug(MESSAGE, ...)
#else
#define debug(MESSAGE, ...) fprintf(stderr, "[DEBUG] (%s at %s:%d): " MESSAGE "\n", __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif

#define clean_errno() (0 == errno ? "None" : strerror(errno))
#define log_err(MESSAGE, ...) fprintf(stderr, "[ERROR] (%s at %s:%d: errno: %s): " MESSAGE "\n", __func__, __FILE__, __LINE__, clean_errno(), __VA_ARGS__)
#define log_warn(MESSAGE, ...) fprintf(stderr, "[WARN] (%s at %s:%d: errno: %s): " MESSAGE "\n", __func__, __FILE__, __LINE__, clean_errno(), __VA_ARGS__)
#define log_info(MESSAGE, ...) fprintf(stderr, "[INFO] (%s at %s:%d): " MESSAGE "\n", __func__, __FILE__, __LINE__, __VA_ARGS__)
#define check(CONDITION, MESSAGE, ...) if(!(CONDITION)) { log_err(MESSAGE, __VA_ARGS__); errno = 0; goto error; }
#define sentinel(MESSAGE, ...)  { log_err(MESSAGE, __VA_ARGS__); errno = 0; goto error; }
#define check_mem(CONDITION) check((CONDITION), "Out of memory.", "")
#define check_debug(CONDITION, MESSAGE, ...) if(!(CONDITION)) { debug(MESSAGE, __VA_ARGS__); errno = 0; goto error; }

#endif
