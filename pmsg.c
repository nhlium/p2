/*********************************************************
 * Pmsg for assignments in the Databases course INF-2700 *
 * UIT - The Arctic University of Norway                 *
 * Author: Weihai Yu                                     *
 *********************************************************/

#include "pmsg.h"
#include <stdio.h>
#include <stdarg.h>

extern pmsg_level msglevel; /* the higher, the more messages... */

void put_msg(pmsg_level level, char* format, ...) {
  va_list args;

  if (level>msglevel)
    return;

  va_start(args, format);
  switch (level)
    {
    case FATAL: fprintf(stderr, "FATAL: "); break;
    case ERROR: fprintf(stderr, "ERROR: "); break;
    case WARN:  fprintf(stderr, "WARN:  "); break;
    case INFO:  fprintf(stderr, "INFO:  "); break;
    case DEBUG: fprintf(stderr, "DEBUG: "); break;
    case FORCE: break;
    }

  vfprintf(stderr, format, args);
  va_end(args);
}


void append_msg(pmsg_level level, char* format, ...) {
  va_list args;

  if (level>msglevel)
    return;

  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}
