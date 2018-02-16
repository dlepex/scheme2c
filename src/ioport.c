/* ioport.c
 * - scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#include "ioport.h"
#include "sysexcep.h"
#include "loggers.h"
#include "charcat.h"
#include  <dirent.h>
#include <malloc.h>
#include <string.h>
UFILE *ustderr, *ustdout, *ustdin;

iport_scp In_console_port = NULL;
oport_scp Out_console_port = NULL;
oport_scp Err_console_port = NULL;

static   string_scp Buf_in;

void ioport_init(svm_t vm)
{
	global_once_preamble();

	ustdout = u_finit(stdout, 0, 0);
	ustdin = u_finit(stdin, 0, 0);
	ustderr = u_finit(stderr, 0, 0);

	type_reserve(vm, TYPEIX_IPORT, "<in-port>",  TYPE_CATEGORY_NONTERM | TYPE_CATEGORY_CONST,
			sizeof(struct InPortDatum), 0, field_offs_scp(iport_scp, path), _SLOTC_IPORT);
	type_reserve(vm, TYPEIX_OPORT, "<out-port>", TYPE_CATEGORY_NONTERM | TYPE_CATEGORY_CONST,
			sizeof(struct OutPortDatum), 0, field_offs_scp(oport_scp, path), _SLOTC_OPORT);

	type_setprint(type_from_ix(TYPEIX_OPORT),  Bin_print_defshort);
	type_setprint(type_from_ix(TYPEIX_IPORT),  Bin_print_defshort);
	log_info(Sys_log, "ioport module init ok.");

	 Buf_in = new_string(vm, Gc_malloc, _Kb_(8));

	 In_console_port = new_iport_f(Gc_perm, stdin, 0);
	 Out_console_port = new_oport_f(Gc_perm, stdout, 0);
	 Err_console_port = new_oport_f(Gc_perm, stderr, 0);


}

// if both args are null -> stdin

inline UChar32  fip_read_char (iport_scp ip);
inline UChar32  fip_console_read_char (iport_scp ip); // input console port
inline void fop_write_char(oport_scp op, UChar32 ch );
inline void fop_write_ustr(oport_scp op, const UChar *s, ulong count);
inline void fp_close(iport_scp port);

void fp_flush(oport_scp o)
{
	u_fflush(o->ufile);
}

datum_scp new_port_f(gc_t gc, uint tix, FILE *file, string_scp name)
{
	assert(file || name);
	iport_scp port = (iport_scp)	inew_inst(gc, tix);
	port->bits = PORT_BITM_FILE | PORT_BITM_TEXT;
	port->close = fp_close;
	port->posbuf = 0;
	if(file) {
		if(file == stdin || file==stdout || file==stderr) {
			port->close = NULL;
		}
		if(file == stdin) {
			port->buf = Buf_in;
		}
		port->file = file;
		port->ufile = u_finit(file, NULL, NULL);
		datum_set_term(port);
	} else if(name) {
		char path[MAX_FILE_PATH];
		ss_get_asci(name, path, MAX_FILE_PATH);
		port->ufile = u_fopen(path, tix == TYPEIX_IPORT ? "r" : "w", 0, 0);
		if(!port->ufile) {
			_mthrow_(EXCEP_PORT_NO_SUCH_FILE, "iport no such file");
		}
		port->file = u_fgetfile(port->ufile);
		port->path = name;
	}
	return (datum_scp) port;
}



iport_scp new_iport_f(gc_t gc, FILE *file, string_scp name)
{
	iport_scp p = (iport_scp) new_port_f(gc, TYPEIX_IPORT, file, name);
	if(file == stdin) {
		p->read_char = fip_read_char;
	} else {
		p->read_char = fip_read_char;
	}

	return p;
}


oport_scp new_oport_f(gc_t gc, FILE *file, string_scp name)
{
	oport_scp p = (oport_scp) new_port_f(gc, TYPEIX_OPORT, file, name);
	p->write_char = fop_write_char;
	p->write_uni = fop_write_ustr;
	p->flush = fp_flush;
	return p;
}

inline void fp_close(iport_scp port)
{
	FILE* f = port->file;
	if(f != stdin && f != stdout && f != stderr) {
		u_fclose(port->ufile);
	}
}

inline UChar32  fip_read_char (iport_scp ip)
{
	if( (ip->bits & IPORT_BITM_EOF) ) {
		return EOF_CHAR;
	}

	UChar32 c = u_fgetcx(ip->ufile);
	if(c == U_EOF || c == 0xFFFFFFFF) {
		ip->bits |= IPORT_BITM_EOF;
		return EOF_CHAR;
	}
	return c;
}

inline UChar32 fip_console_read_char(iport_scp ip)
{
	if( (ip->bits & IPORT_BITM_EOF) ) {
			return EOF_CHAR;
	}
	while(ss_ucs(ip->buf)[ip->posbuf] == 0)
	{
		u_fgets(ss_ucs(ip->buf), ss_capac(ip->buf), ip->ufile);
		ip->buf->length = u_strlen(ss_ucs(ip->buf));
		ip->posbuf = 0;
	}
	bigchar res;
	UChar *ucs = ss_ucs(ip->buf);
	UTF_NEXT_CHAR_UNSAFE(ucs, ip->posbuf, res);
	return res;
}

inline void fop_write_char(oport_scp op, UChar32 ch )
{
	u_fputc(ch, op->ufile);
}

inline void fop_write_ustr(oport_scp op,  const UChar *s, ulong count)
{
	u_file_write( s, (int)count, op->ufile);
}

inline void port_close(void *dm)
{
	uint tix = datum_get_typeix( (datum_scp)dm);
	assert((tix == TYPEIX_IPORT || tix == TYPEIX_OPORT) && "port required");
	iport_scp p = (iport_scp) dm;
	if(!p->close) {
		p->close(p);
	}
}

inline UChar32 __pure iport_read_char(iport_scp ip)
{
	return ip->read_char(ip);
}

inline bool    iport_eof(iport_scp ip)
{
	return ip->bits | IPORT_BITM_EOF;
}

inline void oport_flush(oport_scp op)
{
	op->flush(op);
}

inline void oport_write_char(oport_scp op,  UChar32 ch)
{
	if(op->bits & OPORT_BITM_LIM) {

		if(op->lim == 0) {
			oport_end_limit(op);
			oport_write_char(op, CHR_SPACE);
			oport_write_char(op, 182);
			oport_begin_limit(op, -1);
			u_fflush(op->ufile);
			return;
		} else if(op->lim < 0) {
			return;
		} else {
			op->lim --;
		}
	}
	op->write_char(op, ch);
}
inline void oport_write_uni(oport_scp op, const UChar *start, ulong count)
{
	bigchar ch;
	int i = 0;
	while(i < count) {
		UTF_NEXT_CHAR_UNSAFE(start, i, ch);
		oport_write_char(op, ch);
	}
}
inline void oport_write_asci(oport_scp op, const char* asci) {
	for(const char *p = asci; *p; p++)	{
		oport_write_char(op, _CHR_(*p));
	}
}


inline void oport_write_symbol(oport_scp p, symbol_d sym)
{
	string_scp d = symbol_getname(sym);
	oport_write_ss(p, d);
}

void __pure oport_print_int(oport_scp p, long l)
{
	char buf[64];
	sprintf(buf, "%ld", l);
	oport_write_asci(p, buf);
}

void __pure oport_print_hex(oport_scp p, long l)
{
	char buf[64];
	sprintf(buf, "%lx", l);
	oport_write_asci(p, buf);
}

void __pure oport_print_datum(svm_t vm, oport_scp op, descr d)
{
	if(d == NIHIL) {
		oport_write_asci(op, "#0p");
	}  else if(desc_is_char(d)) {
		// TODO char
		void __pure oport_print_char(svm_t vm, oport_scp op, bigchar ch);
		oport_print_char(vm, op, desc_to_char(d));
	} else if(desc_is_fixnum(d)) {
		//printing fix num;
		long n = desc_to_fixnum(d);
		oport_print_int(op, n);
	} else if(desc_is_local(d)) {
		oport_write_asci(op, "$L:");
		oport_print_int(op, desc_loc_fr(d));
		oport_write_asci(op, ":");
		oport_print_int(op, desc_loc_ix(d));
	} else if(desc_is_global(d)) {
		oport_write_asci(op, "$G:");
		oport_print_int(op, desc_to_glob(d));

	} else {
		if(desc_is_spec(d)) {
			d = symbolobj(d);
		}
		if(scp_valid(d)) {
			datum_print(vm, op, d);
		}
	}
	oport_flush(op);
}

void display(svm_t vm, oport_scp op, descr d)
{
	if(datum_is_string(d)) {
		oport_write_ss(op, ((string_scp)d));
	} else {
		oport_print_datum(vm, op, d);
	}

}





// print unquoted string in quoted form!
descr bin_print_string(svm_t vm, uint argc,__binargv(argv))
{
	oport_scp op =  (oport_scp) argv[0];
	string_scp s = (string_scp) argv[1];
	oport_write_asci(op, "\"");
	ulong i = 0, count = ss_len(s);
	bigchar ch;
	UChar  *sbuf = ss_ucs(s);
	while(i < count) {
		UTF_NEXT_CHAR_UNSAFE(sbuf, i, ch);
		const char* esc = chr_to_esc(ch);
		if(esc) {
			oport_write_asci(op, esc );
		} else if(!chrp_printable(ch)) {
			oport_write_asci(op, "\\x");
			oport_print_hex(op, ch);
		} else {
			oport_write_char(op, ch);
		}
	}
	oport_write_asci(op, "\"");
	return SYMB_VOID;
}


descr bin_print_symbol(svm_t vm, uint argc,__binargv(argv))
{

	oport_scp op =  (oport_scp) argv[0];
	symasc_scp s =  (symasc_scp) argv[1];
	oport_write_ss(op, s->name);
	return SYMB_VOID;
}


descr bin_print_real(svm_t vm, uint argc,__binargv(argv))
{
	real_scp datum = (real_scp)argv[1];
	oport_scp op =  (oport_scp) argv[0];
	char buf[64];
	double v = datum->value;

	if(isnan(v)) {
		sprintf(buf, "%cnan.0", signbit(v) ? '-' : '+');
	} else if(isinf(v)) {
		sprintf(buf, "%cinf.0", signbit(v) ? '-' : '+');
	} else {
		sprintf(buf, "%g", datum->value);
	}
	oport_write_asci(op, buf);

	return SYMB_VOID;
}


descr bin_print_cons(svm_t vm, uint argc,__binargv(argv))
{

	cons_scp datum = (cons_scp) argv[1];
	oport_scp op =  (oport_scp) argv[0];

	oport_write_char(op, '(' );


	ulong i = 0;
	cons_scp p = datum;
	descr el = NIHIL;
	do {
		if(i) {
			oport_write_char(op, ' ' );
		}
		el = p->car;
		oport_print_datum(vm,argv[0], el);
		p =(cons_scp) p->cdr;
		i++;
	} while ( datum_is_carcdr(p)  );

	if (p != NIHIL && p != SYMB_NIL) {
		oport_write_asci(op, " . " );
		oport_print_datum(vm, argv[0], p);
	}
	oport_write_char(op, ')' );

	return SYMB_VOID;
}

descr bin_print_vec(svm_t vm, uint argc,__binargv(argv) )
{
	vec_scp datum = (vec_scp) argv[1];
	oport_scp op =  (oport_scp) argv[0];
	oport_write_asci(op, "#(");
	for(ulong i = 0; i < vec_len(datum); i++) {
		if(i) {
			oport_write_char(op, ' ' );
		}
		oport_print_datum(vm, argv[0] , vec_tail(datum)[i]);
	}
	oport_write_asci(op, ")");
	return SYMB_VOID;
}

descr bin_print_buf(svm_t vm, uint argc,__binargv(argv) )
{
	buf_scp datum = (buf_scp) argv[1];
	oport_scp op =  (oport_scp) argv[0];
	oport_write_asci(op, "#vu8(");
	for(ulong i = 0; i < buf_len(datum); i++) {
		if(i) {
			oport_write_char(op, ' ' );
		}
		oport_print_datum(vm, argv[0] ,desc_from_fixnum(buf_tail(datum)[i]) );
	}
	oport_write_asci(op, ")");
	return SYMB_VOID;
}

descr bin_print_default(svm_t vm, uint argc, __binargv(argv))
{
	oport_scp op =  (oport_scp) argv[0];
	datum_scp dm = (datum_scp) argv[1];
	type_scp tp = datum_get_type(dm);
	if(datum_is_nonterm(dm)) {
		oport_write_asci(op, "(%");
		oport_write_asci(op, tp->asci );
		SLOT_FOREACH(dm)
			oport_write_char(op, ' ' );
			oport_print_datum(vm, argv[0] , slot);
		END_SLOT_FOREACH
		oport_write_asci(op, ")");
	}
	return SYMB_VOID;

}



descr bin_print_default_short(svm_t vm, uint argc, __binargv(argv))
{
	oport_scp op =  (oport_scp) argv[0];
	datum_scp dm = (datum_scp) argv[1];
	type_scp tp = datum_get_type(dm);
	char s[256];
	if(datum_is_nonterm(dm)) {
		oport_write_asci(op, "(%");
		sprintf(s, "%s at <%p>)", tp->asci ,(void*) dm);
		oport_write_asci(op, s);
	}
	return SYMB_VOID;

}


dir_t __Nul_dir = {0};




dir_t* dir_open(const char* path)
{
	DIR *opaq = opendir(path);
	if(!opaq) return NULL;

	dir_t *d = __newstruct(dir_t);
	d->opaq = opaq;
	d->path = path;
	return d;
}

dir_t * file_open(const char *path) {
	FILE *opaq = fopen(path, "r");
	if(!opaq) return NULL;
	dir_t *d = __newstruct(dir_t);
	d->opaq = opaq;
	d->path = path;
	return d;
}
void   dir_close(dir_t* d)
{
	if(d) {
		closedir((DIR*)d->opaq);
		free(d);
	}
}

dir_t*  dir_resolve(dir_t** dirs, const char *mbrpath)
{
/*
	if(!pathp_relative(mbrpath)) {
		return file_open(mbrpath);
	}
	for(dir_t **p = dirs;p; p++) {
		dir_t *cur = *p;
		const char *cpath = cur->path;
		DIR *d = (DIR*)cur->opaq;
		rewinddir(d);
		struct dirent *e;
		while(e = readdir(d)) {
			if(e->d_type == DT_REG) {

			}
		}
	}*/
	return 0;
}








