/* libr.c
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#include "interpret.h"
#include "sysexcep.h"

#include "temp.h"
#include <time.h>






//typedef descr (*builtin_op_t) (svm_t vm, uint argc, __binargv(argv));

typedef union {
	double d; // double
	long   f; // fixnum
} unum_t;

#define NUMT_FIXNUM 0
#define NUMT_DOUBLE 1

#define _BIN_NUM_RET(tcur, res) \
		switch(tcur) { \
			case NUMT_FIXNUM : return desc_from_fixnum(res.f);\
			case NUMT_DOUBLE:  return new_real(vm_gc(vm), res.d);\
			default: assert(0 && "never here");\
		}


#define _BIN_NUM_ARG(tcur, arg, cur, tag ) \
			if(desc_is_fixnum(cur)) {\
				switch(tcur) {\
				case NUMT_FIXNUM : arg.f = desc_to_fixnum(cur); break;\
				case NUMT_DOUBLE:  arg.d = (double)desc_to_fixnum(cur);break;\
				}\
			} else if(datum_is_real(cur)) {\
				if(tcur == NUMT_FIXNUM) {res.d = (double) res.f; } \
				tcur = NUMT_DOUBLE;\
				arg.d = ((real_scp)cur)->value;\
			} else {\
				PANIC_BAD_ARG_ASCI( tag ": number expected"); \
			}

#define _BIN_NUM_REAL(cur, real, tag) \
			if(desc_is_fixnum(cur)) {\
				 real = desc_to_fixnum(cur);\
			} else if(datum_is_real(cur)) {\
				real = ((real_scp)cur)->value;\
			} else {\
				PANIC_BAD_ARG_ASCI( tag ": number expected"); \
			}

#define numberp(x)  (desc_is_fixnum(x) || datum_is_number(x))

descr bin_num_plus(svm_t vm, uint argc, __binargv(argv))
{
	unum_t res = {0}, arg = {0};
	int tcur = NUMT_FIXNUM;
	for(uint i = 0; i <argc; i++)
	{
		_BIN_NUM_ARG(tcur, arg, argv[i], "+");
		switch(tcur) {
		case NUMT_FIXNUM : res.f += arg.f; break;
		case NUMT_DOUBLE:  res.d += arg.d; break;
		}
	}
	_BIN_NUM_RET(tcur, res);
}

descr bin_num_mult(svm_t vm, uint argc, __binargv(argv))
{
	unum_t res = {.f = 1}, arg = {0};
	int tcur = NUMT_FIXNUM;
	for(uint i = 0; i <argc; i++)
	{
		_BIN_NUM_ARG(tcur, arg, argv[i], "*");
		switch(tcur) {
		case NUMT_FIXNUM : res.f *= arg.f; break;
		case NUMT_DOUBLE:  res.d *= arg.d; break;
		}
	}
	_BIN_NUM_RET(tcur, res);
}

descr bin_num_minus(svm_t vm, uint argc, __binargv(argv))
{
	unum_t res = {0}, arg = {0};
	int tcur = NUMT_FIXNUM;
	_BIN_NUM_ARG(tcur, arg, argv[0], "-");
	res = arg;
	if(argc == 1) {
		switch(tcur) {
		case NUMT_FIXNUM : res.f = -arg.f; break;
		case NUMT_DOUBLE:  res.d = -arg.d; break;
		}
	} else {
		for(uint i = 1; i <argc; i++)
		{
			_BIN_NUM_ARG(tcur, arg, argv[i], "-");
			switch(tcur) {
			case NUMT_FIXNUM : res.f -= arg.f; break;
			case NUMT_DOUBLE:  res.d -= arg.d; break;
			}
		}
	}
	_BIN_NUM_RET(tcur, res);
}

descr bin_num_div(svm_t vm, uint argc, __binargv(argv))
{
	unum_t res = {0}, arg = {0};
	int tcur = NUMT_FIXNUM;
	_BIN_NUM_ARG(tcur, arg, argv[0], "/");
	res = arg;
	if(argc == 1) {
		switch(tcur) {
		case NUMT_FIXNUM : res.d = 1.0 / (double) res.f; tcur = NUMT_DOUBLE; break;
		case NUMT_DOUBLE:  res.d = 1.0 / (double) res.d; break;
		}
	} else {
		for(uint i = 1; i <argc; i++)
		{
			_BIN_NUM_ARG(tcur, arg, argv[i], "/");
			switch(tcur) {
			case NUMT_FIXNUM:  res.f /= arg.f; break;
			case NUMT_DOUBLE:  res.d /= arg.d; break;
			}
		}
	}
	_BIN_NUM_RET(tcur, res);
}



descr bin_num_less(svm_t vm, uint argc, __binargv(argv))
{
	bool res = true;
	double prev = 0, dcur = 0;
	descr cur = argv[0];
	_BIN_NUM_REAL(cur, prev, "<" );
	for(uint i = 1; i <argc; i++)
	{
		cur = argv[i];
		_BIN_NUM_REAL(cur, dcur, "<" );
		if(!(prev < dcur)) { res = false; break;};
		prev = dcur;
	}
	return desc_cond(res);
}

descr bin_num_lesseq(svm_t vm, uint argc, __binargv(argv))
{
	bool res = true;
	double prev = 0, dcur = 0;
	descr cur = argv[0];
	_BIN_NUM_REAL(cur, prev, "<=" );
	for(uint i = 1; i <argc; i++)
	{
		cur = argv[i];
		_BIN_NUM_REAL(cur, dcur, "<=" );
		if(!(prev <= dcur)) { res = false; break;};
		prev = dcur;
	}
	return desc_cond(res);
}

descr bin_num_eq(svm_t vm, uint argc, __binargv(argv))
{
	bool res = true;
	double prev = 0, dcur = 0;
	descr cur = argv[0];
	_BIN_NUM_REAL(cur, prev, "=" );
	for(uint i = 1; i <argc; i++)
	{
		cur = argv[i];
		_BIN_NUM_REAL(cur, dcur, "=" );
		if(!(prev == dcur)) { res = false; break;};
		prev = dcur;
	}
	return desc_cond(res);
}

descr bin_num_greatereq(svm_t vm, uint argc, __binargv(argv))
{
	bool res = true;
	double prev = 0, dcur = 0;
	descr cur = argv[0];
	_BIN_NUM_REAL(cur, prev, ">=" );
	for(uint i = 1; i <argc; i++)
	{
		cur = argv[i];
		_BIN_NUM_REAL(cur, dcur, ">=" );
		if(!(prev >= dcur)) { res = false; break;};
		prev = dcur;
	}
	return desc_cond(res);
}

descr bin_num_greater(svm_t vm, uint argc, __binargv(argv))
{
	bool res = true;
	double prev = 0, dcur = 0;
	descr cur = argv[0];
	_BIN_NUM_REAL(cur, prev, ">" );
	for(uint i = 1; i <argc; i++)
	{
		cur = argv[i];
		_BIN_NUM_REAL(cur, dcur, ">" );
		if(!(prev > dcur)) { res = false; break;};
		prev = dcur;
	}
	return desc_cond(res);
}

#define REAL(x)   new_real(vm_gc(vm),x)
descr bin_num_sqrt(svm_t vm, uint argc, __binargv(argv))
{

	double r;
	_BIN_NUM_REAL(argv[0],r, "sqrt" );
	return REAL(sqrt(r));
}



descr bin_numberp(svm_t vm, uint argc, __binargv(argv))
{
	return desc_cond(numberp(argv[0]));
}

descr bin_realp(svm_t vm, uint argc, __binargv(argv))
{
	return desc_cond(datum_is_real(argv[0]));
}

descr bin_fixnump(svm_t vm, uint argc, __binargv(argv))
{
	return desc_cond(desc_is_fixnum(argv[0]));
}

descr bin_eqv(svm_t vm, uint argc, __binargv(argv))
{
	return desc_cond(argv[0] == argv[1]);
}

descr bin_eq(svm_t vm, uint argc, __binargv(argv))
{
	return desc_cond(descr_eq(argv[0], argv[1]));
}

descr bin_equal(svm_t vm, uint argc, __binargv(argv))
{
	return desc_cond(descr_eqrec(argv[0], argv[1]));
}

descr bin_and(svm_t vm, uint argc, __binargv(argv))
{
	for(uint i = 0; i <argc; i++)
	{
		if( argv[i] == SYMB_F) return SYMB_F;

	}
	return argv[argc-1];
}

descr bin_or(svm_t vm, uint argc, __binargv(argv))
{
	for(uint i = 0; i <argc; i++)
	{
		if( argv[i] != SYMB_F) return argv[i];

	}
	return SYMB_F;
}

descr bin_not(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	return d==SYMB_F?SYMB_T:SYMB_F ;
}

descr bin_crand(svm_t vm, uint argc, __binargv(argv))
{
	return desc_from_fixnum(rand() % 40000);
}

descr bin_type_ix(svm_t vm, uint argc, __binargv(argv))
{
	return (scp_valid(argv[0])) ? desc_from_fixnum(datum_get_typeix(argv[0])) : SYMB_F;
}

descr bin_type_str(svm_t vm, uint argc, __binargv(argv))
{

	if(!scp_valid(argv[0])) {
		return SYMB_F;
	}
	decl_gc_of_vm();
	SAVE_RSTK();
	__protect(descr, dm, argv[0]);

	const char* asci = datum_get_type(dm)->asci;
	if(!asci) asci = "<?>";
	__loc string_scp result = SS_ASCI(asci);
	UNWIND_RSTK();
	return result;
}


descr bin_list(svm_t vm, uint argc, __binargv(argv))
{
	if(argc == 0) {
		return SYMB_NIL;
	}
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSHA_RST(argv, 0, argc);
	__protect(cons_scp, r, CONSCAR(argv[0]));
	datum_set_const(r);
	__protect(cons_scp, prev, r);
	for(ulong i = 1; i < argc; i++) {
		__loc cons_scp q = CONSCAR(argv[i]);
		datum_set_const(q);
		FIELD_SET(prev, cdr, q);
		prev = q;
	}

	UNWIND_RSTK();
	return r;
}


descr bin_mulist(svm_t vm, uint argc, __binargv(argv))
{
	if(argc == 0) {
		return SYMB_NIL;
	}
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSHA_RST(argv, 0, argc);
	__protect(cons_scp, r, CONSCAR(argv[0]));
	__protect(cons_scp, prev, r);
	for(ulong i = 1; i < argc; i++) {
		__loc cons_scp q = CONSCAR(argv[i]);
		FIELD_SET(prev, cdr, q);
		prev = q;
	}

	UNWIND_RSTK();
	return r;
}


descr bin_listp(svm_t vm, uint argc, __binargv(argv))
{
	return desc_cond(clp_proper(argv[0]));
}


descr bin_pairp(svm_t vm, uint argc, __binargv(argv))
{
	return desc_cond(datum_is_carcdr(argv[0]));
}

descr bin_nullp(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	return desc_cond(cl_end(d));
}

descr bin_cons(svm_t vm, uint argc, __binargv(argv))
{
	decl_gc_of_vm();
	__loc descr r = CONS(argv[0], argv[1]);
	datum_set_const(r);
	return r;
}

descr bin_length(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("length: list expected");
	long l = cli_len(d);
	if(l < 0) PANIC_ASSERT_ASCI("length: proper list expected");
	return desc_from_fixnum(l);
}

descr bin_ilength(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("length: list expected");
	long l = cli_len(d);
	return desc_from_fixnum(l);
}

descr bin_pair(svm_t vm, uint argc, __binargv(argv))
{
	decl_gc_of_vm();
	descr r = CONS(argv[0], argv[1]);
	return r;
}

descr bin_set_car(svm_t vm, uint argc, __binargv(argv))
{
	cons_scp c = argv[0];
	descr d = argv[1];
	if(!datum_is_carcdr(c ) || datum_is_const(c)) PANIC_BAD_ARG_ASCI("set-car!: mutable pair expected");
	FIELD_SET(c, car, d);
	return SYMB_VOID;
}

descr bin_set_cdr(svm_t vm, uint argc, __binargv(argv))
{
	cons_scp c = argv[0];
	descr d = argv[1];
	if(!datum_is_carcdr(c ) || datum_is_const(c)) PANIC_BAD_ARG_ASCI("set-cdr!: mutable pair expected");
	FIELD_SET(c, cdr, d);
	return SYMB_VOID;
}

descr bin_car(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("car: list expected");
	return CAR(d);
}



descr bin_cdr(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("cdr: list expected");
	return CDR(d);
}

descr bin_caar(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("caar: list expected");
	d = CAR(d);
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("caar: caarable value expected");
	return CAR(d);
}

descr bin_cddr(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("cddr: list expected");
	d = CDR(d);
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("cddr: cddrable value expected");
	return CDR(d);
}
descr bin_cadr(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("cadr: list expected");
	d = CDR(d);
	if(!datum_is_carcdr(d ) ) PANIC_BAD_ARG_ASCI("cadr: cadrable expected");
	return CAR(d);
}


descr bin_quote(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(datum_is_id(d)) d = id_symbol(d);
	return d;
}

descr bin_unquote(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(datum_is_carcdr(d)) {
		decl_gc_of_vm();
		return CONSCAR(d);
	} else {
		return d;
	}
}

descr bin_unquote_splicing(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!clp_proper(d))  PANIC_BAD_ARG_ASCI("unquote_splicing: proper list expected");
	return d;
}


descr bin_list_clone(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_cl(d ) ) PANIC_BAD_ARG_ASCI("list-clone: list expected");
	return cl_clone(vm, d);
}

descr bin_list_last(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_cl(d ) ) PANIC_BAD_ARG_ASCI("list-last: list expected");
	descr r = cl_last( d);
	return r == NIHIL ? SYMB_NIL : r;
}




descr bin_list_cat(svm_t vm, uint argc, __binargv(argv))
{
	if(argc == 0) {
		return SYMB_NIL;
	}

	SAVE_RSTK();
	decl_gc_of_vm();
	__protect(cons_scp,r, NIHIL);
	__protect(cons_scp,q, NIHIL);
	__protect(cons_scp,p, NIHIL);
	__protect(cons_scp,d, NIHIL);
	PUSHA_RST(argv, 0, argc)
	ulong i = 0;
	for(i = 0; i <argc; i++) {
		p = argv[i];
		p = datum_is_cl(p) ? cl_clone(vm, p) : CONSCAR(p);
		q = cl_last(p);
		if(!cl_end(q)) {
			r = p;
			break;
		}
	}

	if(i == argc) {
		UNWIND_RSTK();
		return SYMB_NIL;
	}
	i++;

	for(; i < argc; i++) {
		p = argv[i];
		p = datum_is_cl(p) ? cl_clone(vm, p) : CONSCAR(p);
		p = cl_clone(vm, p);
		d = cl_last(p);
		if(!cl_end(d)) {
			FIELD_SET(q, cdr, p); q = d;
		}
	}
	UNWIND_RSTK();
	return r;
}


descr bin_vector_set(svm_t vm, uint argc, __binargv(argv))
{
	descr vec = argv[0];
	descr ix = argv[1];
	if(!datum_is_vector(vec) || !datum_is_mutable(vec)) PANIC_BAD_ARG_ASCI("vector-set!: arg#1 must be MUTABLE vector");

	if(!desc_is_fixnum(ix)) PANIC_BAD_ARG_ASCI("vector-set!: arg#2 must be fixnum");
	long i = desc_to_fixnum(ix);
	if(i < 0 || i >= vec_len(vec)) PANIC_BAD_ARG_ASCI("vector-set!: arg#2 index out of bounds");
	VEC_SET(vec, i, argv[2]);
	return SYMB_VOID;
}

descr bin_vector_ref(svm_t vm, uint argc, __binargv(argv))
{
	descr vec = argv[0];
	descr ix = argv[1];
	if(!datum_is_vector(vec)) PANIC_BAD_ARG_ASCI("vector-ref: arg#1 must be vector");
	if(!desc_is_fixnum(ix)) PANIC_BAD_ARG_ASCI("vector-ref: arg#2 must be fixnum");
	long i = desc_to_fixnum(ix);
	if(i < 0 || i >= vec_len(vec)) PANIC_BAD_ARG_ASCI("vector-ref: arg#2 index out of bounds");
	return vec_tail(vec)[i];
}

descr bin_vector_length(svm_t vm, uint argc, __binargv(argv))
{
	descr vec = argv[0];
	if(!datum_is_vector(vec)) PANIC_BAD_ARG_ASCI("vector-length: arg#1 must be vector");
	return desc_from_fixnum(vec_len(vec));
}

descr bin_make_vector(svm_t vm, uint argc, __binargv(argv))
{
	SAVE_RSTK();
	decl_gc_of_vm();
	descr count = argv[0];
	__loc descr fill;
	if(argc == 2) {
		fill = argv[1];
		PUSH_RSTK(fill);
	} else if(argc > 2) {
		PANIC_BAD_ARG_ASCI("make_vector: one or two arguments expected");
	}
	if(!desc_is_fixnum(count) || desc_to_fixnum(count) < 0) PANIC_BAD_ARG_ASCI("make_vector: arg#1 must be nonnegative fixnum (length)");
	ulong c = desc_to_fixnum(count);
	__loc vec_scp v = new_vec(gc, c);
	descr *tail = vec_tail(v);
	if(argc == 2) {
		for(ulong i = 0; i < c; i++) {
			tail[i] = fill;
		}
	}
	UNWIND_RSTK();
	return v;
}



descr bin_vector(svm_t vm, uint argc, __binargv(argv))
{

	decl_gc_of_vm();
	SAVE_RSTK();
	PUSHA_RST(argv, 0, argc);
	__protect(vec_scp, r, new_vec(gc, argc));
	descr *tail = vec_tail(r);
	for(ulong i = 0; i < argc; i++) {
		tail[i] = argv[i];
	}
	UNWIND_RSTK();
	return r;
}



descr bin_vectorp(svm_t vm, uint argc, __binargv(argv))
{
	return desc_cond(datum_is_vector(argv[0]));
}


descr bin_eval(svm_t vm, uint argc, __binargv(argv))
{
	return vm_eval(vm, argv[0]);
}



descr bin_make_immutable(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(scp_valid(d)) {
		datum_set_const(d);
	}
	return d;
}

descr bin_box(svm_t vm, uint argc, __binargv(argv))
{
	decl_gc_of_vm();
	return MAKEBOX(argv[0]);
}


descr bin_unbox(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_box(d)) PANIC_BAD_ARG_ASCI("unbox: box expected");
	return unbox(d);
}

descr bin_unboxif(svm_t vm, uint argc, __binargv(argv))
{
	descr d = argv[0];
	if(!datum_is_box(d)) {
		return d;
	} else {
		return unbox(d);
	}
}

descr bin_make_hashtable(svm_t vm, uint argc, __binargv(argv))
{
	decl_gc_of_vm();
	descr count = argv[0];
	if(!desc_is_fixnum(count) || desc_to_fixnum(count) < 0) PANIC_BAD_ARG_ASCI("make-hashtable: size expected");
	return new_htbc(vm, gc, desc_to_fixnum(count));
}

descr bin_hashtable_set(svm_t vm, uint argc, __binargv(argv))
{
	SAVE_RSTK();
	decl_gc_of_vm();
	__protect(descr, h,  argv[0]);
	__protect(descr, key,  argv[1]);
	__protect(descr, val,  argv[2]);
	if(!datum_is_instof_ix(h, TYPEIX_HASHTABLE)) PANIC_BAD_ARG_ASCI("hashtable-set!: hashtable expected");
	val = htb_put(vm, gc, &h, &key, &val);
	UNWIND_RSTK();
	return val;
}

descr bin_hashtable_ref(svm_t vm, uint argc, __binargv(argv))
{
	SAVE_RSTK();
	decl_gc_of_vm();
	__protect(descr, h,  argv[0]);
	__protect(descr, key,  argv[1]);
	if(!datum_is_instof_ix(h, TYPEIX_HASHTABLE)) PANIC_BAD_ARG_ASCI("hashtable-ref: hashtable expected");
	key = htb_get(vm, &h, &key);
	UNWIND_RSTK();
	return key;
}





descr bin_proc_print(svm_t vm, uint argc, __binargv(argv))
{
	closure_scp d =  (closure_scp) argv[0];
	if(!datum_is_instof_ix(d, TYPEIX_CLOSURE)) PANIC_BAD_ARG_ASCI("closure expected");
	if(d->bin) { oport_write_asci(Out_console_port,"\n<built-in>\n");return SYMB_VOID;;}
	fcode_print(vm, Out_console_port, ((closure_scp)d)->fcod);
	icod_print(vm,  ((closure_scp)d)->fcod,Out_console_port );
	oport_write_asci(Out_console_port,"\n");
	return SYMB_VOID;
}

descr bin_time(svm_t vm, uint argc, __binargv(argv))
{
	decl_gc_of_vm();
	return new_real(gc, msecs_relstartup());
}

descr bin_gc_minor(svm_t vm, uint argc, __binargv(argv))
{
	gc_minor(vm);

	return SYMB_VOID;
}

descr bin_gc_major(svm_t vm, uint argc, __binargv(argv))
{
	gc_major(vm);

	return SYMB_VOID;
}

descr bin_wbox(svm_t vm, uint argc, __binargv(argv))
{
	return new_wbox(vm, vm_gc(vm), argv[0]);
}

descr bin_display(svm_t vm, uint argc, __binargv(argv))
{
	for(uint i = 0; i < argc; i++) {
		display(vm, Out_console_port, argv[i]);
	}
	return SYMB_VOID;
}

descr bin_assert(svm_t vm, uint argc, __binargv(argv))
{
	if(argv[0]==SYMB_F) {
		oport_scp op = Out_console_port;
		oport_write_asci(op, "Assertion failed: ");
		display(vm, Out_console_port, argv[1]);
		oport_write_asci(op, "\n");
		PANIC_ASSERT("%s", "user assert");
	}
	return SYMB_VOID;
}


void libr_init(svm_t vm)
{
	srand(time(0));
	GLOB_BIND("number?", new_bin_multy(1, bin_numberp));
	GLOB_BIND("real?", new_bin_multy(1, bin_realp));
	GLOB_BIND("fixnum?", new_bin_multy(1, bin_fixnump));
	GLOB_BIND("+", new_bin_multy(0, bin_num_plus));
	GLOB_BIND("*", new_bin_multy(0, bin_num_mult));
	GLOB_BIND("-", new_bin_multy(1, bin_num_minus));
	GLOB_BIND("/", new_bin_multy(1, bin_num_div));
	GLOB_BIND("sqrt", new_bin(1, bin_num_sqrt));
	GLOB_BIND("crand", new_bin(0, bin_crand));
	GLOB_BIND("<", new_bin_multy(2, bin_num_less));
	GLOB_BIND("=", new_bin_multy(2, bin_num_eq));
	GLOB_BIND("<=", new_bin_multy(2, bin_num_lesseq));
	GLOB_BIND(">", new_bin_multy(2, bin_num_greater));
	GLOB_BIND(">=", new_bin_multy(2, bin_num_greatereq));


	GLOB_BIND("eqv?", new_bin(2, bin_eqv));
	GLOB_BIND("eq?", new_bin(2, bin_eq));
	GLOB_BIND("equal?", new_bin(2, bin_equal));

	GLOB_BIND("and", new_bin_multy(0, bin_and));
	GLOB_BIND("or", new_bin_multy(0, bin_or));
	GLOB_BIND("not", new_bin(1, bin_not));


	GLOB_BIND("type-ix", new_bin(1, bin_type_ix));
	GLOB_BIND("type-str", new_bin(1, bin_type_str));


	GLOB_BIND("list", new_bin_multy(0, bin_list));
	GLOB_BIND("mulist", new_bin_multy(0, bin_mulist));
	GLOB_BIND("cons", new_bin(2, bin_cons));
	GLOB_BIND("pair", new_bin(2, bin_pair));
	GLOB_BIND("set-car!", new_bin(2, bin_set_car));
	GLOB_BIND("set-cdr!", new_bin(2, bin_set_cdr));
	GLOB_BIND("car", new_bin(1, bin_car));
	GLOB_BIND("cdr", new_bin(1, bin_cdr));
	GLOB_BIND("caar", new_bin(1, bin_caar));
	GLOB_BIND("cadr", new_bin(1, bin_cadr));
	GLOB_BIND("cddr", new_bin(1, bin_cddr));
	GLOB_BIND("list?", new_bin(1, bin_listp));
	GLOB_BIND("null?", new_bin(1, bin_nullp));
	GLOB_BIND("pair?", new_bin(1, bin_pairp));
	GLOB_BIND("length", new_bin(1, bin_length));
	GLOB_BIND("ilength", new_bin(1, bin_ilength));
	GLOB_BIND("list-clone", new_bin(1, bin_list_clone));
	GLOB_BIND("list-last", new_bin(1, bin_list_last));
	GLOB_BIND("list-cat", new_bin_multy(0, bin_list_cat));
	GLOB_BIND("quote", new_bin(1, bin_quote));
	GLOB_BIND("unquote", new_bin(1, bin_unquote));
	GLOB_BIND("unquote-splicing", new_bin(1, bin_unquote_splicing));

	GLOB_BIND("vector-set!", new_bin(3, bin_vector_set));
	GLOB_BIND("vector-ref", new_bin(2, bin_vector_ref));
	GLOB_BIND("vector-length", new_bin(1, bin_vector_length));
	GLOB_BIND("make-vector", new_bin_multy(1, bin_make_vector));
	GLOB_BIND("vector?", new_bin(1, bin_vectorp));
	GLOB_BIND("vector", new_bin_multy(0, bin_vector));


	GLOB_BIND("box", new_bin(1, bin_box));
	GLOB_BIND("wbox", new_bin(1, bin_wbox));
	GLOB_BIND("unbox", new_bin(1, bin_unbox));
	GLOB_BIND("unboxif", new_bin(1, bin_unboxif));

	GLOB_BIND("make-immutable", new_bin(1, bin_make_immutable));


	GLOB_BIND("hashtable-set!", new_bin(3, bin_hashtable_set));
	GLOB_BIND("hashtable-ref", new_bin(2, bin_hashtable_ref));
	GLOB_BIND("make-hashtable", new_bin(1, bin_make_hashtable));
	GLOB_BIND("ptr0", NIHIL  );


	GLOB_BIND("proc-print",  new_bin(1, bin_proc_print) );
	GLOB_BIND("eval", new_bin(1, bin_eval));

	GLOB_BIND("gc-major",  new_bin(0, bin_gc_major) );
	GLOB_BIND("gc-minor", new_bin(0, bin_gc_minor));

	GLOB_BIND("time", new_bin(0, bin_time));

	GLOB_BIND("display", new_bin_multy(0, bin_display));
	GLOB_BIND("assert", new_bin(2, bin_assert));

}


