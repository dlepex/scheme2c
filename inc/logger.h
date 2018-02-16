/* logger.h
 * scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include "base.h"
#include "stdio.h"
#include "opsys.h"


#define EXCEP_LOGGER 219


#ifdef _LOG_DEBUG
#define logf_debug(logger, fmtstr, ...) logf_all(logger, LOG_LEVEL_DEBUG,  fmtstr, __VA_ARGS__)
#define log_debug(logger, text) log_all(logger, LOG_LEVEL_DEBUG, text)
#else
#define logf_debug(logger, fmtstr, ...)
#define log_debug(logger, text)
#endif

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_SEVERE 5

#define LOG_DECOR_SIMPLE 1
#define LOG_DECOR_SRC    2
#define LOG_DECOR_SRCLINE 3


#define logf_all(logger, level, fmtstr, ...) \
{\
	 char _s_logf[MAX_LOGSTR_SIZE]; \
	sprintf(_s_logf, fmtstr, __VA_ARGS__); \
	logger_println(logger, level, _s_logf, __FILE__, __LINE__);\
}

#define log_all(logger, level, text) logger_println(logger, level, text, __FILE__, __LINE__)

#define log_severe(logger, text) log_all(logger, LOG_LEVEL_SEVERE, text)
#define log_error(logger, text) log_all(logger, LOG_LEVEL_ERROR, text)
#define log_warn(logger, text) log_all(logger, LOG_LEVEL_WARN, text)
#define log_info(logger, text) log_all(logger, LOG_LEVEL_INFO, text)


#define logf_severe(logger, fmtstr, ...) logf_all(logger, LOG_LEVEL_SEVERE,  fmtstr, __VA_ARGS__)
#define logf_error(logger, fmtstr, ...) logf_all(logger, LOG_LEVEL_ERROR,  fmtstr, __VA_ARGS__)
#define logf_warn(logger, fmtstr, ...) logf_all(logger, LOG_LEVEL_WARN,  fmtstr, __VA_ARGS__)
#define logf_info(logger, fmtstr, ...) logf_all(logger, LOG_LEVEL_INFO,  fmtstr, __VA_ARGS__)

#define log_excep(logger, level) { \
	inline exdata_t excep_get_data(void); \
	exdata_t d = excep_get_data();\
	 char s12[MAX_LOGSTR_SIZE]; *s12=0; file_get_line(s12, d->srcpath, d->srcline );\
	logf_all(logger, level, "exception %i occured - %s\n  line << %s >>\n", d->code, d->msg, s12); \
	}
#define log_excep_error(logger)   log_excep(logger, LOG_LEVEL_ERROR);
#define log_excep_severe(logger)   log_excep(logger, LOG_LEVEL_SEVERE);

typedef void  *logger_t;
typedef void(*logdecor_t)() ;
inline void logger_println(logger_t log, int level,  const char *msg, const char* _file_, int _line_);
logger_t filelogger_make(const char *lname, FILE* f,const char *path);
void logger_free(logger_t);
void logger_set_decorate(logger_t l, logdecor_t d);
logdecor_t logger_get_decorate(int what);
void logger_set_level(logger_t l, int level);

int file_get_line(char *result, const char * path, int line);

#define FOVAL_TYPE_INT  1
#define FOVAL_TYPE_DOUBLE 2
#define FOVAL_TYPE_STRING 3
#define FOVAL_FORMAT_INT_I 1
#define FOVAL_FORMAT_DOUBLE_G   1
#define MAX_LOGSTR_SIZE 712


typedef struct
{
	int type;
	int format;
	union {
		double double_v;
		int    int_v;
		char   *string_v;
		void   *ptr_v;
	} value;
} formatval_t;



#define fov_i(v) {FOVAL_TYPE_INT, FOVAL_FORMAT_INT_I, {.int_v=v}}
#define fov_g(v) {FOVAL_TYPE_DOUBLE, FOVAL_FORMAT_DOUBLE_G, {.double_v=v}}
#define fov_s(v) {FOVAL_TYPE_STRING, 0, {.string_v=v}}


#define EFORMATSYNTAX 1
#define EFORMATTRUNC 2
#define EFORMATDATA 3

int format_string(const char *src, const formatval_t *fmt, char *dest, size_t maxdest);



#endif /* LOGGER_H_ */
