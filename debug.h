/* 
 * File:   debug.h
 * Author: abit
 *
 * Created on 2014/12/02, 15:06
 */

#ifndef DEBUG_H
#define    DEBUG_H

#if defined(DBG_ENABLE) && !defined(NODEBUG)
  #include <stdio.h>
  #define WITH_DBG_PRINTF
#else
  #undef  WITH_DBG_PRINTF
#endif

#define DBG0 0x01
#define DBG1 0x02
#define DBG2 0x04
#define DBG3 0x08
#define DBG4 0x10
#define DBG5 0x20
#define DBG6 0x40
#define DBG7 0x80
#define DBGOFF 0x00
#define DBGALL 0xff


#if defined(WITH_DBG_PRINTF)
#ifdef __linux
  #define dbg(type, ...) DEBUG_PRINTF(type, __VA_ARGS__, "\n")
  #define DEBUG_PRINTF(type, fmt, ...) \
          if ((type) & (DBG_ENABLE)) \
            fprintf(stderr, "DBG: %s(): "fmt"%s", __func__,  __VA_ARGS__)
#else
  #define dbg(type, ...) DEBUG_PRINTF(type, __VA_ARGS__, "\n\r")
  #define DEBUG_PRINTF(type, fmt, ...) \
          if ((type) & (DBG_ENABLE)) \
            printf("DBG: %s(): "fmt"%s", __func__,  __VA_ARGS__)
#endif
#else
  #define dbg(...)
#endif

#endif    /* DEBUG_H */

