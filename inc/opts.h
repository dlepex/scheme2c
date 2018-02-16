/* opts.h
 * - scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef OPTS_H_
#define OPTS_H_

#include "base.h"

#define OPT_OCCURS_MULTY 1

struct AppOption {
	const char *name;
	const char  symb;
	const int   id;
	const int   argc;
	const char  *type;
	const char *help;
	const char *def;
	ulong  mask;
};

#define OPTION_EMPTY -1
#define OPTION_PRINT_SYNTAX -2
#define OPTION_NOT_FOUND -3
#define OPTION_BAD_ARGC -4
#define OPTION_MULTIPLE -5
#define EXCEP_BAD_OPTION 250

typedef int (*opts_handler_t)(struct AppOption *op, int argc,  char **args);

int opts_parse(opts_handler_t handler,struct AppOption *,int argc, char **argv);
void print_opt_help(struct AppOption *opts);
#endif /* OPTS_H_ */
