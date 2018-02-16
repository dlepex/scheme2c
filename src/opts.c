/* opts.c
 * - scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */
#include "opts.h"
#include <stdio.h>
#include <string.h>
#include "opsys.h"
#include "base.h"
#include "loggers.h"


void print_opt_help(struct AppOption *opts)
{

	puts("\nAvailable options are:");
	for(int i = 0; opts[i].name; i++) {
		struct AppOption *o = &opts[i];
		DECL_AUTO_PCHAR(buf, 64);
		char *sargc = buf;
		switch(o->argc) {
		case -1: sargc = "many"; break;
		case  0: sargc = "flag";  break;
		default:
			sprintf(buf, "%i", o->argc);
		}
		char *socur = "";
		if(o->mask & OPT_OCCURS_MULTY) {
			socur="*";
		}
		DECL_AUTO_PCHAR(defs, 128);
		if(o->def) {
			strcpy(defs, "=");
			strcat(defs, o->def);
		}
		if(o->symb) {
			printf("--%-16s -%1c [%s{%s}%s]%s\t%s\n", o->name, o->symb, o->type, sargc,defs, socur,  o->help);
		} else {
			printf("--%-16s    [%s{%s}%s]%s\t%s\n", o->name, o->type, sargc,defs,socur,  o->help);
		}
	}
}

struct AppOption *opts_get(struct AppOption *opts, const char* name, char symb )
{
	for(int i = 0; opts[i].name; i++) {
		struct AppOption *o = &opts[i];
		if(name && strcmp(o->name, name) == 0) {
			return o;
		}
		if(symb && symb == o->symb) {
			return o;
		}
	}
	return NULL;
}

#define OPT_ALREADY_OCCUR 2


int opts_parse(opts_handler_t handler, struct AppOption *opts, int argc, char **argv)
{
	for(int i = 1; i < argc;) {
		const char *arg = argv [i];
		struct AppOption *o = NULL;
		if(arg[0]=='-') {
			const char *n_opt = 0;
			char s_opt = 0;
			if(arg[1] == 0) {
				i++; continue;
			}	else if (arg[1] == '-') {
				if(arg[2] == 0) {
					i++; continue;
				}
				n_opt = arg + 2;
				o = opts_get(opts, n_opt, 0);
			} else if (arg[2]==0) {
				s_opt  = arg[1];
				o = opts_get(opts, 0, s_opt);
			} else if(arg[0] == 0 ) {
				i++;
				continue;
			} else {
				for (int j = 1; arg[j]; j++) {
					o = opts_get(opts, 0, arg[j]);
					if (o == NULL) {
						struct AppOption fake = { .name=0, .symb=arg[j],
								.id = OPTION_NOT_FOUND };
						handler(&fake, 0, NULL);
					} else {

						if (!(o->mask & OPT_OCCURS_MULTY) && (o->mask
								& OPT_ALREADY_OCCUR)) {
							struct AppOption fake = { .name=o->name,
									.symb=o->symb, .id = OPTION_MULTIPLE,
									.argc=o->argc };
							handler(&fake, 0, NULL);
							return 1;
						} else {
							o->mask |= OPT_ALREADY_OCCUR;
						}
						if (o->argc != 0) {

							struct AppOption fake = { .name=o->name,
									.symb=arg[j], .id = OPTION_BAD_ARGC,
									.argc=o->argc };
							handler(&fake, 0, NULL);
						} else {
							handler(o, 0, NULL);
						}
					}
				}
				i++;
				continue;
			}
			if (o == NULL) {
				struct AppOption
						fake = {.name = n_opt, .symb=s_opt, .id = OPTION_NOT_FOUND };
				handler(&fake, 0, NULL);
				return 1;
			}

			if (!(o->mask & OPT_OCCURS_MULTY) && (o->mask & OPT_ALREADY_OCCUR)) {
				struct AppOption fake = { .name=o->name, .symb=o->symb,
						.id = OPTION_MULTIPLE, .argc=o->argc };
				handler(&fake, 0, NULL);
				return 1;
			} else {
				o->mask |= OPT_ALREADY_OCCUR;
			}

			const int k = i;
			const int lim = min(argc, k + (o->argc < 0 ? argc : o->argc) + 1);
			logf_debug(Sys_log, "opts_get %s %i", (o==NULL? "NULL":o->name), (o==NULL? -123: o->id) );
			for(i++;i<lim && argv[i][0]!='-'; i++);
			int c = i - k - 1;
			logf_debug(Sys_log, "opts_get opt.argc = %i", c);
			if (o->argc != -1 && o->argc != c) {
				struct AppOption
						fake = { .name = o->name, .symb=o->symb, .id = OPTION_BAD_ARGC, .argc=o->argc };
				handler(&fake, c, NULL);
				return 1;
			} else {
				handler(o, i - k - 1, argv + k + 1);
			}
		} else {
			int k = i;
			for(i++; i<argc && argv[i][0] != '-'; i++);
			struct AppOption fake = {.id = OPTION_EMPTY};
			handler(&fake, i - k, argv + k);
		}
	}
	return 0;
}
