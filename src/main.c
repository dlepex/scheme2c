
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <utypes.h>
#include "svm.h"
#include "temp.h"
#include "logger.h"
#include "opsys.h"
#include "logger.h"
#include "unittest.h"
#include "opts.h"
#include "loggers.h"
#include "sysexcep.h"
#include "string.h"
#include <signal.h>
#include <limits.h>
#include <ustdio.h>
#include <locale.h>
#include "memmgr.h"
#include "ioport.h"
#include "parse.h"
#include "transform.h"
#include <dirent.h>
#include <string.h>
#include "interpret.h"
#include "curses.h"
long x, y, r;


long *px, *py;

void f1() {
	r = (x >> 1) + (y >> 1);
	x = r << 1;
}

void f2() {
	r = *px + *py;
	*px = r;
}

#define ERROR1 400
#define ERROR2 500

int err = ERROR2;

#define PRINTF(x, args) printf(x, ((args)))

void f3() {
	_mthrow_(err, "Error!");
	r = ((x & 0xFF) == 48);
	y += r;
}


void f4() {

	r = ((x & 0xFF) == 0);
	y += r;
}

logger_t l;
void testlog() {
	l = filelogger_make("sys", stdout, 0);
	logger_set_decorate(l, logger_get_decorate(LOG_DECOR_SRC));
	logger_set_level(l, LOG_LEVEL_INFO);
	logger_println(l, LOG_LEVEL_ERROR, "error logging 1", __FILE__, __LINE__);
	logger_println(l, LOG_LEVEL_ERROR, "error logging 2", __FILE__, __LINE__);
	logf_error(l, "%i %s", 241, " lipshz ");
	log_info(l, "error!");
	logf_warn(l, "privet %s", "3274");
	char buf[512];
	file_get_line(buf, __FILE__, __LINE__);
	printf("%s \n", buf);
	_mthrow_(340, "BIGGEST FAIL!");

}

#define O_HELP       9
#define O_VERBOSE 	10
#define O_ITERPRET 	11
#define O_TRANSLATE 12
#define O_DIRS     	13
#define O_APPS		14

#define O_STACKSZ   120
#define O_CONSTMEM  121

#define O_GC_YS     221
#define O_GC_KOY	  222
#define O_GC_LIM    223
#define O_GC_MXAGE  224
#define O_GC_OYPOOL 225

#define O_GCLOG     332
#define O_SYSLOG    336


struct AppOption app_opts[] =
{
	{.name="help", .symb='h', .id=O_HELP, .argc=0, .type="",.help="cmdline syntax and opts"},
	{.name="verbose", .symb='v', .id=O_VERBOSE, .argc=0, .type="",.help="verbose messages"},
	{.name="repl", .symb='r', .id=O_ITERPRET, .argc=0, .type="",.help="repl & interpret mode"},
	{.name="translate", .symb='t', .id=O_TRANSLATE, .argc=0, .type="",.help="translation mode"},

	{.name="dirs", .symb='d', .id=O_DIRS, .argc=1, .type="dir-path",.help="directory to search", .mask = OPT_OCCURS_MULTY },
	{.name="stk", .symb=0, .id=O_STACKSZ, .argc=1, .type="int,Kb",.help="c-root stack size", .def="256"},
// const memory
	{.name="constmem", .symb=0, .id=O_CONSTMEM, .argc=1, .type="int,Mb",.help="const mem size", .def="2"},
// gc
	{.name="gcys", .symb='y', .id=O_GC_YS, .argc=1, .type="int,Mb",.help="gc young space size", .def="4"},
	{.name="gclim", .symb='L', .id=O_GC_LIM, .argc=1, .type="int,Mb",.help="gc max old space size", .def = "128"},
	{.name="gckoy", .symb='K', .id=O_GC_KOY, .argc=1, .type="real,[2..)",.help="gc old-per-young koef", .def = "4"},
	{.name="gcmxage", .symb='X', .id=O_GC_MXAGE, .argc=1, .type="int,[1..7]",.help="gc young age max", .def = "2"},
	{.name="gcoypool", .symb='P', .id=O_GC_OYPOOL, .argc=1, .type="int,Mb",.help="gc old->young pool size", .def="1"},
// loggers
	{.name="gclog", .symb=0, .id=O_GCLOG, .argc=1, .type="file-path",.help="GC log redirect", .def = "stdout"},
	{.name="syslog", .symb=0, .id=O_SYSLOG, .argc=1, .type="file-path",.help="system log redirect"},
	{0}
		//{.name="", .symb='?', .id=1, .argc=0, .type="",.help=""}
};

void app_print_help(struct AppOption *op) {
	printf("musc - scheme interpreter & translator\n");
	printf("Syntax: musc [<options>] <scm_files>, where: \n");
	printf("<options> ::= [-'optSymbol' [<value>]* | --'optionFullname' [<value>]*]*\n");
	printf("Example: musc -i /path/myfile1.scm /path/myfile2.scm\n");
	print_opt_help(op);
}

struct PathList {
	char *path;
	void *next;
};

#define klist_end(l)   (l)->end
#define klist_begin(l)  (l)->begin
#define klist_init(pkl, p)   (pkl)->begin=(pkl)->end=(void**)(p)

#define _pfield(ptr, type, field) (&(((type*)(ptr))->field))

#define _pdata(fptr, type, field)   ((type*)(((byte*)(fptr)) - (ulong) &((type*)0)->field))

#define klist_append(pkl, ptr, type, field) \
	if(klist_empty(pkl)) {klist_init(pkl, _pfield(ptr, type, field));}\
	else {void *fptr =_pfield(ptr, type, field);   *(pkl)->end=fptr; (pkl)->end=(void**)fptr; *(pkl)->end=0; }

#define klist_foreach(l, p) \
	for(void **p = klist_begin(l); p; p = (*p))
#define klist_empty(l)  (klist_begin(l) == 0)

typedef struct KList {
	void **begin;
	void **end;
} klist_t;

struct ResultOpts {
	bool interp;
	bool transl;
	bool verbose;
	ulong  constmem, stk;
	ulong gc_novsz, gc_limsz, gc_poolsz;
	uint gc_mxage;
	double gc_koy;
	klist_t files, dirs;
	FILE *gclog;
	FILE *syslog;

};

static struct ResultOpts Inopt = {0};

#define EXCEPT_WRONG_OPT 211

char *o_check_dir_path(const char* path)
{
	FILE *f = fopen(path, "r");
	if(f) {
		fclose(f);
		return strdup(path);
	}
	_fthrow_(EXCEPT_WRONG_OPT, "dir absolute path not found: \"%s\"", path);
	return 0;
}

bool pathp_relative(const char* path)
{
	return path[0] != '/';
}

FILE* path_resolve(klist_t* dirslist,struct PathList *node)
{
	FILE *f = 0;
	char *fp = node->path;
	if(!pathp_relative(fp)) {
		f = fopen(fp, "r");
		if(f) {
			return f;
		}
		_fthrow_(EXCEPT_WRONG_OPT, "file absolute path unresolved: \"%s\"", fp);
	} else {
		char path[512];
		klist_foreach(&Inopt.dirs, fcur) {
			struct PathList *pcur = _pdata(fcur, struct PathList, next);
			*path = 0;
			strcat(strcat(strcpy(path, pcur->path), "/"), fp);
			f = fopen(path, "r");
			if(f) {
				free(node->path);
				node->path = strdup(path);
				break;
			}
		}
		if(f) {
			return f;
		}
		_fthrow_(EXCEPT_WRONG_OPT, "file relative path unresolved: \"%s\"", fp);
	}
	assert("never here" && 0);
	return 0;

}



int o_get_int(const char* s, int from, int to,  const char* err) {
	int result;
	if(sscanf(s, "%i", &result) != 1) {
		_mthrow_(EXCEPT_WRONG_OPT, err);
	}
	if(result < from || result > to) {
		_mthrow_(EXCEPT_WRONG_OPT, err);
	}
	return result;
}

double o_get_real( const char* s, double from, double to, const char* err) {
	double result;
	if(sscanf(s, "%lf", &result) != 1) {
		_mthrow_(EXCEPT_WRONG_OPT, err);
	}
	if(result < from || result > to) {
		_mthrow_(EXCEPT_WRONG_OPT, err);
	}
	return result;
}

FILE *o_get_log(const char* s, const char* err) {
	if(strcmp(s, "stdout")==0) {
		return stdout;
	}
	if(strcmp(s, "stderr")==0) {
		return stderr;
	}
	FILE *f = fopen(s, "w");
	if(f == NULL) {
		_mthrow_(EXCEPT_WRONG_OPT, err);
	}
	return f;
}



int appopts_handler(struct AppOption *op, int argc,  char **args)
{
	switch(op->id) {
	case OPTION_NOT_FOUND:
		app_print_help(app_opts);
		if(op->name) {
			printf("Error: Option not found --%s\n", op->name);
		} else {
			printf("Error: Option not found -%c\n", op->symb);
		}
		return 1;
	case OPTION_BAD_ARGC:
		app_print_help(app_opts);
		if (op->argc != -1) {
			printf("Error: bad argument count for --%s given %i needed %i\n",op->name,  argc, op->argc);
		} else {
			printf("Error: bad argument count for --%s given %i needed >=1\n",op->name,  argc);
		}
		return 1;
	case OPTION_MULTIPLE:
		app_print_help(app_opts);
		printf("Error: Multiple options --%s\n", op->name);
		return 1;
	case OPTION_EMPTY: {
		for(int i = 0; i < argc; i++) {
			struct PathList *pl  = __newstruct(struct PathList);
			pl->path = strdup(args[i]);
			klist_append(&Inopt.files, pl , struct PathList, next);
		}
		}break;
	case O_DIRS: {
		logf_debug(Sys_log, "dirs %i", argc);
		for(int i = 0; i < argc; i++) {
			struct PathList *pl  = __newstruct(struct PathList);
			pl->path = o_check_dir_path(args[i]);
			klist_append(&Inopt.dirs, pl , struct PathList, next);
		}
		}break;
	case O_HELP:
		app_print_help(app_opts);
	break;
	case O_VERBOSE: Inopt.verbose = true; break;
	case O_ITERPRET: Inopt.interp = true; break;
	case O_TRANSLATE: Inopt.transl = true; break;

	case O_STACKSZ:
		Inopt.stk = _Kb_(o_get_int(args[0], 1, INT_MAX, "Stack size is INTEGER >=1 KB"));
		break;
	case O_CONSTMEM:
		Inopt.constmem = _Mb_(o_get_int(args[0], 1, INT_MAX, "Const memory size is INTEGER >=1 MB"));
		break;
	case O_GC_YS:
		Inopt.gc_novsz = _Mb_(o_get_int(args[0], 1, INT_MAX, "GC Young space size is INTEGER >=1 MB"));
		break;
	case O_GC_LIM:
		Inopt.gc_limsz = _Mb_(o_get_int(args[0], 1, INT_MAX, "GC Max old size is INTEGER >=1 MB"));
		break;
	case O_GC_OYPOOL:
		Inopt.gc_poolsz = _Mb_(o_get_int(args[0], 1, INT_MAX, "GC Poolsz is INTEGER >=1 MB"));
		break;
	case O_GC_MXAGE:
		Inopt.gc_mxage = o_get_int(args[0], 0, 7, "GC young max age is INTEGER in [0..7]");
		break;
	case O_GC_KOY:
		Inopt.gc_koy = o_get_real(args[0], 2.0, INT_MAX, "GC old-per-young koef is REAL >= 2.0");
		break;
	case O_GCLOG:
		Gc_log_file = o_get_log(args[0], "--gclog mustbe file-path or 'stdout' or 'stderr'");
		break;
	case O_SYSLOG:
		Inopt.syslog = o_get_log(args[0], "--syslog mustbe file-path or 'stdout' or 'stderr'");
		break;
	default: {

		logf_info(Sys_log, "strange! unhandled option %i", op->id);
		/*printf("%s id=%i argc=%i \n", op->name , op->id, op->argc);
		for(int i = 0; i < argc; i++) {
			printf("\t%s\n", args[i]);
		}*/
	}}



	return 0;
}

logger_t Sys_log = 0;

void create_loggers() {
	Vm_log =  filelogger_make("vm", Inopt.syslog, "vm.log");
	logger_set_level(Vm_log, LOGLEVEL_VM);
	logger_set_decorate(Vm_log, logger_get_decorate(LOG_DECOR_SRC));

	Gc_log = filelogger_make("gc", Inopt.gclog, "gc.log");
	logger_set_level(Gc_log, LOGLEVEL_GC);
	logger_set_decorate(Gc_log, logger_get_decorate(LOG_DECOR_SIMPLE));
	log_debug(Sys_log, "loggers GC VM created");

}

void sig_handler(int sig) {
	switch(sig) {
	case SIGSEGV:
		log_severe(Sys_log, "OS segmentation fault");
		break;
	case SIGFPE:
		log_severe(Sys_log, "OS arithmetic fault SIGFPE");
		break;
	default:
		logf_warn(Sys_log,  "OS signal caught: %i", sig);
		break;

	}
}

void set_signals() {
	signal(SIGSEGV, sig_handler);
	signal(SIGFPE, sig_handler);
}







svm_t initialize_vm();
void console_loop();
void file_loop(svm_t vm);

int main(int argc, char **argv) {

	msecinit();
	// init signal handlers
	set_signals();
	// initialize excetions for this thread
	excep_thread_init();
	// init unit testing
	unittest_init();

/*
	initscr(); cbreak();// noecho();
	  nonl();
	             intrflush(stdscr, FALSE);

*/
	//printf("TYPO %ul" , tag_get_typeix(134220513));

	_try_
		Sys_log = filelogger_make("sys", 0, "dbg.log");
		logger_set_level(Sys_log, LOGLEVEL_SYS);
		logger_set_decorate(Sys_log, logger_get_decorate(LOG_DECOR_SRC));
		Inopt.gc_mxage = -1;
		if(opts_parse(appopts_handler, app_opts,  argc, argv)) {
			return 1;
		}
		create_loggers();
		initialize_vm();

		if(Inopt.verbose) {
			anf_do_verbose();
		}



		// init_ioports();
		decl_vm();
		decl_gc_of_vm();
		klist_foreach(&Inopt.files, link) {
			struct PathList* p = _pdata(link, struct PathList, next);
			path_resolve(&Inopt.dirs, p);
			logf_debug(Sys_log, "src file path resolved: %s", p->path);
		}

		file_loop( vm);
//		gc_test(vm);



		if(!Inopt.transl && Inopt.interp) {
			console_loop(vm);
		}
	_except_
	_catch_(EXCEPT_WRONG_OPT)
		app_print_help(app_opts);
		printf("CMDLINE ERROR: %s\n", excep_msg());
	_catchall_
		log_excep_error(Sys_log);
	_end_try_


	return EXIT_SUCCESS;
}

void file_loop(svm_t vm)
{
	SAVE_RSTK();
	decl_gc_of_vm();
	oport_scp cc_oport;
	if(Inopt.transl) {
		cc_oport = new_oport_f(Gc_perm, NULL, new_string_asci(vm, Gc_perm, "__genbody.h"));
		Geninit_oport = new_oport_f(Gc_perm, NULL, new_string_asci(vm, Gc_perm,"__geninit.h"));
	}
	if(!(klist_empty(&Inopt.files))) {
		__protect(parser_scp, pr, 0);
		__protect(iport_scp, ip, 0);
		klist_foreach(&Inopt.files, link) {
			struct PathList* p = _pdata(klist_begin(&Inopt.files), struct PathList, next);
			ip = new_iport_f(gc, 0, new_string_asci(vm, gc, p->path));

			pr = new_parser(vm, ip);

			assert(iport_read_char(ip) != -1);

			oport_write_asci(Out_console_port, "Reading file: ");
			oport_write_asci(Out_console_port, p->path);
			oport_write_asci(Out_console_port, "\n");
			oport_write_asci(Out_console_port, "=====================================================\n");
			while (!pars_eof(pr)) {
				__loc descr d = pars_read_datum(vm, &pr);
				if (d == NIHIL) {
					continue;
				}

				descr r = SYMB_VOID;
				if (!parsp_error(pr)) {
					oport_write_asci(Out_console_port, "\nExpression: ");
					oport_print_datum(vm, Out_console_port, d);
					if(Inopt.transl) {
						vm_compile(vm, d, cc_oport);
					} else {
						r = vm_eval(vm, d);
					}
					if(r != SYMB_VOID) {
						oport_write_asci(Out_console_port, "\n---> ");
						oport_print_datum(vm, Out_console_port, r);

					}
				}
			}
			oport_write_asci(Out_console_port, "\n=================================================\n");


		}

	}
	UNWIND_RSTK();

}

void console_loop(svm_t vm)
{
	printf("Mini Scheme REPL (read-eval-print-loop):\n");

	decl_gc_of_vm();
	SAVE_RSTK();
	__protect(parser_scp, icpr, new_parser(vm, In_console_port));
	pars_set_always_fatal(icpr);
	__protect(descr, d, NIHIL);
	__protect(descr, d1, NIHIL);
	__protect(transformer_scp, tr, new_transformer(vm));



	for(;;) {
		printf("\n>> ");

		//bigchar c = iport_read_char(In_console_port);

		d = pars_read_datum(vm, &icpr);

		if (!parsp_error(icpr) && d) {
			trf_check_syntax(vm, _RS(tr), _RS(d) );

			//oport_write_asci(_DS(pwp), "\n");
			//oport_print_datum(vm, _DS(pwp), d);

			oport_print_datum(vm, Out_console_port, d);
			if(!trfp_error(tr) ) {
				trf_transform_to_anf(vm, &tr, CONSCAR(d));
				//oport_print_datum(vm, _DS(pwp), tr->gfc->lcod);
				oport_write_asci(Out_console_port, "\n---> ");
				//fcode_print(vm, Out_console_port, tr->gfc);
				descr r = vm_run(vm, tr->gfc);
				if(r!=SYMB_VOID) {
					oport_print_datum(vm, Out_console_port, r);
				}
				oport_write_asci(Out_console_port, "\n");
			}

		}
	}
	UNWIND_RSTK();
}

svm_t initialize_vm() {
	struct SVM_Params vmpar = {0};

	vm_default_params(&vmpar);

	if(Inopt.stk) {vmpar.stack_size = Inopt.stk;}
	if(Inopt.constmem) {vmpar.perm_size = Inopt.constmem;}

	gcmov_param_t gcpar = {0};

	gc_mov_param_init(&gcpar);
	if(Inopt.gc_novsz) {gcpar.nov_sz = Inopt.gc_novsz;}
	if(Inopt.gc_limsz) {gcpar.limsz = Inopt.gc_limsz;}
	if(Inopt.gc_mxage >= 0) {gcpar.newagemax = Inopt.gc_mxage;}
	if(Inopt.gc_poolsz) {gcpar.oref_sz = Inopt.gc_poolsz;}
	if(Inopt.gc_koy) {gcpar.old_per_new = Inopt.gc_koy;}



	gc_t gcmov = gc_mov_make(&gcpar);

	logf_info(Sys_log,
				"GC created nov: %ld lim: %ld mxage: %d pool: %ld koy: %g kliv: %g",
				gcpar.nov_sz, gcpar.limsz, gcpar.newagemax, gcpar.oref_sz, gcpar.old_per_new, gcpar.live_per_all);

	svm_t vm = vm_init(&vmpar, gcmov);

	logf_info(Sys_log,
				"VM created permmem: %ld cstk: %ld",
				vmpar.perm_size, vmpar.stack_size);

	return vm;

}


char * ultobis(unsigned long int ul, char *s)
{
	int i = 0, p = 0;
	while(ul) {
		if( (i % 4 == 0) && (i != 0)) {
			if(i % 8 == 0) {
				s[p++] = ',';
			} else
				s[p++] = '`';
		}
		if ( (ul) & (1ul << 63)) {
			s[p++] = '1';
		} else {
			s[p++] = '0';
		}
		i ++;
		ul <<= 1;
	}
	s[p] = 0;
	return s;
}


void *test_gc(void *__data)
{
/*	test_prelude(test_gc);

	struct SVM_Params spar;
	vm_default_params(&spar);
	svm_t svm = vm_init(&spar);
	gc_t g = vm_gcperm(svm);
	string_scp s =  new_string(g, 33);

	ss_set_asci(s, "vec");
	ss_set_asci(s, "vector");
	UFILE *out = u_finit(stdout, 0, 0);
	ss_hash_code(s);
	printf( "++++%i %i %i++++", ss_hash_code(s), s->hash, descr_hash_code(desc_from_symindex(1)));





	descr y1 = vm_symbol_asci(svm, "#!vec" ), y2, y3;
//	string_scp ys = vm_symbol_str(svm, y1);

	ss_set_asci(svm->swapsymstr, "11121212");

	for(int i = 0; i < 4; i++) {
		u_fprintf(out, "@ %i @", ss_ucs(s)[i]);
	}
	u_fprintf(out, "\n %C@ $%i %S  @\n y1=%ld\n", 0x3BB,
			ss_capac(svm->swapsymstr), ss_ucs(vm_symbol_str(svm, SYMB_SGN_SYN_SPLICING)), desc_to_symindex(y1));

	y2 = vm_symbol_asci(svm,  "+#!vec1sd" );
	y2 = vm_symbol_asci(svm,  "#!vec2");
	y3 = vm_symbol_asci(svm,  "#!vec");
	assert(ss_eq(new_string_asci(g, "#!vec"),
			vm_symbol_str(svm, y1) ));
	assert(y1 == y3);
	u_fputs(ss_ucs(svm->swapsymstr), out);
	u_fputs(ss_ucs(s), out);
	int i = 0;
	UChar32 ch;
	do {
		U16_NEXT_UNSAFE(c_uchars(s->buf),i,ch);
	} while(ch);

	u_fprintf(out, "%s===%i==%i=%i=%i=%i\n",ss_ucs(s), i, u_strlen(ss_ucs(s)), s->uclen, ss_hash_code(s), ss_hash_code(s));
	gc_stat_t stat = {0};
	gc_getstat(g, &stat);
	u_fprintf(out, "===%f",  stat.allocated_percent);
	u_fclose(out);
	test_postlude;*/
}

static tester_t tests[] = { test_gc, NULL };

void itests()
{
	test_group_t tg = testgroup_make("gc");
	testgroup_adda(tg, tests);
	testgroup_run(tg);
}




