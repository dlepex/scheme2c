/* loggers.h
 * scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef LOGGERS_H_
#define LOGGERS_H_

#include "logger.h"

//#define DBG_MEMMGR
//#define DBG_MEMMGR_SLOTSET
//#define MEMMGR_WREFS 0

#ifndef LOGLEVEL_GC
#define LOGLEVEL_GC LOG_LEVEL_INFO
#endif


#ifndef LOGLEVEL_VM
#define LOGLEVEL_VM LOG_LEVEL_INFO
#endif

#ifndef LOGLEVEL_SYS
#define LOGLEVEL_SYS LOG_LEVEL_DEBUG
#endif


extern logger_t Sys_log;
extern logger_t Vm_log;
extern logger_t Gc_log;

extern FILE *Gc_log_file;

#endif /* LOGGERS_H_ */
