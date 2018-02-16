/* io.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef IO_H_
#define IO_H_


#include "opcore.h"

#define DIR_DIRECTORY 1
#define DIR_FILE      2
typedef struct  {
	ulong bits;
	void *opaq;
	const char *path;
	const char *name;
} dir_t;

#define dir_getfile(dir) ((FILE*)((dir)->opaq))
extern dir_t __Nul_dir;
#define NUL_DIR __Nul_dir;

#define EXCEP_PORT_NO_SUCH_FILE 835

#define PORT_BITM_TEXT   _ubit_(2)
#define PORT_BITM_FILE   _ubit_(0)
#define PORT_BITM_STR    _ubit_(1)



#define IPORT_BITM_EOF   _ubit_(3)
#define OPORT_BITM_LIM   _ubit_(4)
typedef struct OutPortDatum         *oport_scp;
typedef struct InPortDatum          *iport_scp;

#define EOF_CHAR -1

extern UFILE *ustderr, *ustdout, *ustdin;

typedef UChar32  (*iport_readchar_t) (iport_scp ip);
typedef void (*port_close_t)(iport_scp);

extern iport_scp In_console_port;
extern oport_scp Out_console_port, Err_console_port;

struct InPortDatum {
	tag_t tag;
	ulong bits;
	FILE  *file;
	UFILE *ufile;
	string_scp  path;
	port_close_t close;

	string_scp buf;
	ulong      posbuf;
	iport_readchar_t read_char;
};

#define _SLOTC_IPORT 1

typedef void (*oport_writechar_t) (oport_scp op, UChar32 ch );
typedef void (*oport_writeustr_t) (oport_scp op, const UChar *s, ulong count);
typedef void (*oport_proc0_t) (oport_scp op);

struct OutPortDatum {
	tag_t tag;
	ulong bits;
	FILE  *file;
	UFILE *ufile;
	string_scp  path;
	port_close_t close;


	long lim;
	oport_writechar_t write_char;
	oport_writeustr_t write_uni;
	oport_proc0_t flush;
};
#define _SLOTC_OPORT 1

#define oport_begin_limit(op, _lim)    (op)->bits |= OPORT_BITM_LIM; (op)->lim =(_lim)
#define oport_end_limit(op)           (op)->bits &= ~OPORT_BITM_LIM; (op)->lim=0

// paths & dirs


void path_getname(const char* path, char* name);
dir_t* dir_open(const char* path);
void   dir_close(dir_t* d);
// resolve file name
dir_t*  dir_resolve(dir_t** dirs, const char *mbrpath);




// ports
void ioport_init(svm_t vm);

// if both args are null -> stdin
iport_scp new_iport_f(gc_t gc, FILE *file, string_scp name);

oport_scp new_oport_f(gc_t gc, FILE *file, string_scp name);

inline void port_close(void *dm);

inline UChar32 iport_read_char(iport_scp ip);
inline bool    iport_eof(iport_scp ip);
inline void oport_flush(oport_scp op);
inline void oport_write_char(oport_scp op,  UChar32 ch);
inline void oport_write_uni(oport_scp op,  const UChar *start, ulong count);
#define oport_write_ss(op, s)    oport_write_uni(op, ss_ucs(s), ss_len(s))


inline void oport_write_asci(oport_scp op, const char* asci) ;
void __pure oport_print_int(oport_scp p, long l);
void __pure oport_print_datum(svm_t vm, oport_scp pp, descr pd);
void __pure oport_print_hex(oport_scp p, long l);

void display(svm_t vm, oport_scp op, descr d);


void vm_stk_print(svm_t vm, oport_scp op,  ulong count);
void vm_stk_printall(svm_t vm, oport_scp op);
descr bin_print_default_short(svm_t vm, uint argc, __binargv(argv));

#define datum_print(vm, port, dm) \
{ \
		descr pri = (descr)(datum_get_type(dm)->printer); \
		if(pri) { decl_bin_argv(argv) {port, dm};\
		vm_call(vm, pri, 2, argv);} \
}


descr bin_print_string(svm_t vm, uint argc,__binargv(argv));
descr bin_print_symbol(svm_t vm, uint argc,__binargv(argv));
descr bin_print_real(svm_t vm, uint argc,__binargv(argv));
descr bin_print_cons(svm_t vm, uint argc,__binargv(argv));
descr bin_print_vec(svm_t vm, uint argc,__binargv(argv) );
descr bin_print_buf(svm_t vm, uint argc,__binargv(argv) );





#endif /* IO_H_ */
