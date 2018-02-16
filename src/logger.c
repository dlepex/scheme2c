/* logger.c
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>

#include "logger.h"
#include "sysexcep.h"
int format_string(const char *src, const formatval_t *fmt, char *dest, size_t maxdest)
{
	char *sp = (char*)src, *dp = dest;
	char buf[64];
	char *plus;
	for(;*sp; sp++) {

		if( *sp=='%' && *(sp+1)=='{') {
			sp +=2;
			char *nump = sp;
			for(;*sp != '}' && *sp; sp++);
			if(!*sp) {
				return EFORMATSYNTAX;
			}
			int index = atoi(nump);
			formatval_t f = fmt[index];

			plus = NULL;
			switch(f.type) {
			case FOVAL_TYPE_INT: {
					sprintf(buf, "%i", f.value.int_v);
					plus = buf;
				}
				break;
			case FOVAL_TYPE_DOUBLE: {
					sprintf(buf, "%g", f.value.double_v);
					plus = buf;
				}
				break;
			case FOVAL_TYPE_STRING: {
					plus = f.value.string_v;
				}
				break;
			}
			if(!plus) {
				return EFORMATDATA;
			}
			strcpy(dp, plus);
			dp += strlen(plus);
		} else {
			*(dp++) = *sp;
			maxdest--;
			if(!maxdest) {
				return EFORMATTRUNC;
			}
		}
	}
	return 0;
}

#define LOGGER_TYPE_FILE 0
struct Logger;
struct LogPrintParam;

typedef void (*decorate_t)(struct Logger*, struct LogPrintParam*, char*result);
typedef void (*printr_t)(struct Logger*, struct LogPrintParam*);

struct LogPrintParam {
	int level;
	const char *msg, *_file_;
	int _line_;

};
struct Logger {
	int type;
	const char *name;
	int  level;
	printr_t print;
	decorate_t decorate;
};

struct FileLogger {
	int type;
	const char *name;
	int level;
	printr_t print;
	decorate_t decorate;
	FILE *file;
	bool spec;
};


inline void logger_println(logger_t log, int level,  const char *msg, const char* _file_, int _line_)
{
	if( ((struct Logger*)log)->level <= level) {
		struct LogPrintParam p = {level, msg, _file_, _line_};
		((struct Logger*)log)->print(log, &p);
	}
}

void FileLogger_print(struct Logger* l, struct LogPrintParam *par) {
	struct FileLogger *fl =(struct FileLogger*) l;
	if(fl->decorate) {
		char buf[MAX_LOGSTR_SIZE];
		fl->decorate((struct Logger*)fl, par, buf);
		fputs(buf, fl->file);
	} else {
		fprintf(fl->file, "%s\n", par->msg);
	}
	fflush(fl->file);
}

inline const char *logger_levelstr(int level) {
	switch(level)
	{
	case LOG_LEVEL_DEBUG:
		return "debug";
	case LOG_LEVEL_INFO:
		return "info";
	case LOG_LEVEL_WARN:
		return "warn";
	case LOG_LEVEL_ERROR:
		return "error";
	case LOG_LEVEL_SEVERE:
		return "severe";
	default:
		assert(0);
	}
}

void simple_decorate(struct  Logger* fl, struct LogPrintParam *par, char *result)
{

	sprintf(result, "%s[%s] - %s\n", fl->name, logger_levelstr(par->level), par->msg);
}

void src_decorate(struct  Logger* fl, struct LogPrintParam *par, char *result)
{

	sprintf(result, "%s[%s][%s:%i] - %s\n",
			fl->name,
			logger_levelstr(par->level),
			par->_file_,
			par->_line_,
			par->msg);
}

void srcline_decorate(struct  Logger* fl, struct LogPrintParam *par, char *result)
{
	char buf[MAX_LOGSTR_SIZE];*buf=0;
	file_get_line(buf, par->_file_, par->_line_);
	sprintf(result, "%s[%s][%s:%i] - %s\nline: %s\n",
			fl->name,
			logger_levelstr(par->level),
			par->_file_,
			par->_line_,
			par->msg,
			buf);
}



logger_t filelogger_make(const char *lname, FILE* f,const char *path)
{
	assert(f || path);
	assert(goodstr(lname));
	struct FileLogger *fl = (struct FileLogger*) malloc(sizeof(struct FileLogger));
	if(!f) {
		if(!path) {
			_mthrow_(EXCEP_LOGGER, "logger constructor: path = NULL");
		}
		fl->file = fopen(path, "w");
		if(fl->file == NULL) {
			_mthrow_(EXCEP_LOGGER, "logger constructor: file not found");
		}
		fl->spec = false;
	} else {
		fl->file = f;
		fl->spec = true;
	}
	fl->name = lname;
	fl->print = FileLogger_print;
	fl->type = LOGGER_TYPE_FILE;
	fl->decorate = NULL;
	fl->level = LOG_LEVEL_DEBUG;
	return (void*)fl;
}

void logger_set_decorate(logger_t l, logdecor_t d) {
	((struct Logger*)l)->decorate = (decorate_t)d;
}

void logger_set_level(logger_t l, int level) {
	assert(level >= 0 && level <= 5);
	((struct Logger*)l)->level = level;
}

void logger_free(logger_t l) {
	struct Logger *lo = (struct Logger*)l;
	if(lo->type == LOGGER_TYPE_FILE) {
		struct FileLogger *fl = (struct FileLogger*)lo;
		if(!fl->spec) {
			fclose(fl->file);
		}
	}
	free(l);
}

logdecor_t logger_get_decorate(int what)
{
	switch(what)
	{
	case LOG_DECOR_SIMPLE:
		return (logdecor_t)simple_decorate;
	case LOG_DECOR_SRC:
		return (logdecor_t)src_decorate;
	case LOG_DECOR_SRCLINE:
		return (logdecor_t)srcline_decorate;
	default:
		assert(0);
	}
}

inline void string_norm_end(char* buf)
{
	int l = strlen(buf);
	if(l >= 1) {
		l--;
		for(int i = l; i >= 0 && i >= (l-3); i--) {
			if(buf[i]=='\n' || buf[i] == '\r') {
				buf[i]=0;
			}
		}
	}
}

int file_get_line(char *result, const char * path, int line)
{
	*result = 0;
	FILE *f = fopen(path, "r");
	if(!f) {
		return 1;
	}
	line --;
	int r = 0;
	for(int i = 0; i < line && fgets(result, MAX_LOGSTR_SIZE, f); i++);
	if(!fgets(result, MAX_LOGSTR_SIZE, f)) {
		r = 2;
	}
	string_norm_end(result);
	fclose(f);
	return r;
}
