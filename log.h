/**
 * @file log.h
 * @brief Logging functions
 *
 * log.h v0.0.1
 * simple logging library - sleepntsheep 2022
 * logging should be simple and easy,
 * here you put in argument just as how you would printf it
 * panic(...) : log at panic level and abort
 * warn(...) : log at warn level
 * info(...) : log at info level
 * the *err(...) counterpart of each function
 * do the same thing except it call perror at the end
 * causing error from errno to be printed too
 */

#ifndef SHEEP_LOG_H
#define SHEEP_LOG_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define __FL__ __FILE__, __LINE__
/**
 * @brief log at panic level and abort
 */
#define panic(...)                                                             \
    do {                                                                       \
        __stderr_log("PANIC", __FL__, __VA_ARGS__);                            \
        exit(1);                                                               \
    } while (0)
/**
 * @brief log at panic level, print errno and abort
 */
#define panicerr(...)                                                          \
    do {                                                                       \
        __stderr_log("PANIC", __FL__, __VA_ARGS__);                            \
        perror("");                                                            \
        exit(1);                                                               \
    } while (0)
/**
 * @brief log at warn level
 */
#define warn(...)                                                              \
    do {                                                                       \
        __stderr_log("WARN", __FL__, __VA_ARGS__);                             \
    } while (0)
/**
 * @brief log at warn level, print errno
 */
#define warnerr(...)                                                           \
    do {                                                                       \
        __stderr_log("WARN", __FL__, __VA_ARGS__);                             \
        perror("");                                                            \
    } while (0)
/**
 * @brief log at info level
 */
#define info(...)                                                              \
    do {                                                                       \
        __stderr_log("INFO", __FL__, __VA_ARGS__);                             \
    } while (0)
/**
 * @brief log at info level, print errno
 */
#define infoerr(...)                                                           \
    do {                                                                       \
        __stderr_log("INFO", __FL__, __VA_ARGS__);                             \
        perror("");                                                            \
    } while (0)
/**
 * @brief log unimplemented control flow
 */
#define unimplemented(...) __stderr_log("UNIMPLEMENTED", __FL__, __VA_ARGS__);
/**
 * @brief log control flow that should be unreachable
 */
#define unreachable(...) __stderr_log("UNREACHABLE", __FL__, __VA_ARGS__);

void __stderr_log(const char *type, const char *file, const int line,
                  const char *fmt, ...);

#endif /* SHEEP_LOG_H */

#ifdef SHEEP_LOG_IMPLEMENTATION

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void __stderr_log(const char *type, const char *file, const int line,
                  const char *fmt, ...) {
    fprintf(stderr, "%s: %s:%d: ", type, file, line);
    va_list a;
    va_start(a, fmt);
    vfprintf(stderr, fmt, a);
    va_end(a);
    fprintf(stderr, "\n");
    fflush(stderr);
}

#endif /* SHEEP_LOG_IMPLEMENTATION */
