/**
 * A simple & plain data (un)serilization lib.
 *
 * Making load and store process not that painful.
 *
 * Anyway, this is a tiny utils wrote for my dailylife.
 *
 * @blackball <bugway@gmail.com>
 */

#ifndef vpack_h
#define vpack_h

#include <limits.h>
#include <stdarg.h>

#ifdef __cplusplus
#define EXTERN_BEGIN extern "C" {
#else
#define EXTERN_END   }
#endif

#if CHAR_BIT != 8
# error "vpack requires 8 bit byte char type!"
#endif

EXTERN_BEGIN

/**
 * Pack plain data sequenece into file.
 *
 * @param fn filename, if exists, overwrite.
 * @param fmt format string
 * @return 0 for success, else -1 
 */
int vpack(const char *fn, const char *fmt, ...);

/**
 * Unpack plain data from a binary file.
 *
 * @param fn filename
 * @param fmt format string
 * @return 0 for success, else -1
 */
int vget(const char *fn, const char *fmt, ...);

EXTERN_END

#endif
