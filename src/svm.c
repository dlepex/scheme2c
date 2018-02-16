/* svm.c
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#include "svm.h"
#include "malloc.h"
#include "loggers.h"
#include "sysexcep.h"
#include "memmgr.h"
#include "opcore.h"

logger_t Vm_log = NULL;

// Global data def
// Spec symbol table
symasc_scp *__Sym_table__;
// Global VM table
descr *__Glob_table__;

ulong Globt_lim, Globt_ptr;

// Permanent GC
gc_t Gc_perm;

// global Environment
static env_scp Envglob = NULL;
// global Weak symbol table
static hashtable_scp Symbhtb = NULL;

closure_scp Bin_htb_eq_classic, Bin_htb_hash_classic, Bin_htb_eq_sym,
		Bin_htb_hash_sym, Bin_symb_finalize, Bin_print_def, Bin_print_defshort;

descr bin_htb_eq_classic(svm_t vm, uint argc, __binargv(argv));
descr bin_htb_hash_classic(svm_t vm, uint argc, __binargv(argv));
descr bin_htbsym_hash(svm_t vm, uint argc, __binargv(argv));
descr bin_htbsym_eq(svm_t vm, uint argc, __binargv(argv));
descr bin_symbol_finalize(svm_t vm, uint argc, __binargv(argv));

extern void gc_init();
extern void transformer_init(svm_t vm);
void datum_bin_init();
void basic_modules_init();

// symbol and type init
#include "__vm_init_spec.h"
#include "__libr.h"

void vm_default_params(struct SVM_Params *p) {
	p->perm_size = _Mb_(2);
	p->stack_size = _Kb_(32);
	p->symhash_count = 531; //3139;
	p->symtable_count = 1000;
	p->globtable_count = _Kb_(32);

}

void gvm_datum_init(svm_t vm, struct SVM_Params *p) {

	descr bin_print_default_short(svm_t vm, uint argc, __binargv(argv));
	// Hash bin's (always constant)
	Bin_htb_eq_classic = new_bin(2, bin_htb_eq_classic);
	Bin_htb_hash_classic = new_bin(1, bin_htb_hash_classic);
	Bin_htb_eq_sym = new_bin(2, bin_htbsym_eq);
	Bin_htb_hash_sym = new_bin(1, bin_htbsym_hash);
	Bin_print_defshort = new_bin(2, bin_print_default_short);

	// Creating Global Symbol Weak hash table.

	vec_scp buckets = new_vec(Gc_perm, p->symhash_count);
	// Constant vector of buckets will be extension to global slots.
	datum_set_globscan(buckets);
	//	global_add(vm, buckets);
	descr *bu = vec_tail(buckets);
	for (ulong i = 0; i < vec_len(buckets); i++) {
		bu[i] = __global(global_add(vm, NIHIL));
		assert(datum_is_box(bu[i]));
	}
	Symbhtb = new_htb_g(vm, Gc_perm, &buckets, &Bin_htb_hash_sym,
			&Bin_htb_eq_sym);

	// Creating global environment
	Envglob = new_env(vm, Gc_perm, NIHIL);
	Envglob->car = Symbhtb;
	assert(Envglob->cdr == 0);

}

void gvm_basic_modules_init(svm_t vm, struct SVM_Params* p) {
	extern void ioport_init(svm_t vm);
	extern void parser_init(svm_t vm);
	extern void interpret_init(svm_t vm);

	ioport_init(vm);
	parser_init(vm);
	transformer_init(vm);
	interpret_init(vm);
}

inline __pure env_scp env_global() {
	return Envglob;
}

void gvm_init(svm_t vm, struct SVM_Params *p) {
	global_once_preamble();
	// mem mgr init
	gc_init();
	// permanent gc creation.
	Gc_perm = gc_perm_make(p->perm_size);
	// global tables allocation
	__Glob_table__ = (descr*) tmalloc(p->globtable_count * sizeof(descr));
	Globt_lim = p->globtable_count;
	Globt_ptr = MAX_RESERVED_TYPE;
	__Glob_table__[0] = SYMB_UNDEF; // first  value in global table is always UNDEF!
	__Sym_table__ = (symasc_scp*) tmalloc(p->symtable_count * sizeof(descr));

	log_debug(Sys_log, "gvm init stage II tables created");

	// basic type initialization
	vm_init_types(vm);

	log_info(Sys_log, "gvm init stage II  basic types init");

	// init basic builtins
	gvm_datum_init(vm, p);

	// spec (that are table-based) symbols init
	vm_init_symbols(vm);

	// basic modules init
	gvm_basic_modules_init(vm, p);

	bin_init(vm);
	decl_gc_of_vm();
	string_scp ds = new_string_asci(vm, gc, "quote");
	assert(SYMB_QUOTE == symbol_from_string(vm, ds) && "SYMBOL ERR");

	log_info(Sys_log, "gvm init stage II  modules init");

	log_info(Sys_log, "vm init ok.");
}

static os_thread_key_t Vm_key = 0;
void vm_var_init() {
	global_once_preamble();
	os_thread_key_create(&Vm_key);
}

svm_t vm_init(struct SVM_Params *p, gc_t gc) {
	svm_t svm = (svm_t) __newstruct(struct SVM);
	_try_
			vm_var_init();
			os_thread_key_set(Vm_key, svm);
			ulong ssz = p->stack_size;
			svm->rstk_begin = (volatile descr **) malloc(ssz);
			svm->rstk_end = (volatile descr**) (((byte*) svm->rstk_begin) + (ssz));
			svm->rstk_ptr = svm->rstk_begin;
			svm->gc = gc;
			gc->vm = svm;
			log_info(Sys_log, "vm init stage I ok.");
			gvm_init(svm, p);
			_except_
		_catchall_
		log_error(Sys_log, "vm init failure");
		_rethrow_;
		_end_try_
		return svm;

	}

inline svm_t local_vm() {
	return (svm_t) os_thread_key_get(Vm_key);
}

inline void vm_get_globals(svm_t vm, descr **from, descr **to) {
	*from = __Glob_table__ + MAX_RESERVED_TYPE ;
	*to = __Glob_table__ + Globt_ptr;
}

inline descr global_get(ulong x) {
	if (__Glob_table__[x] == NIHIL) {
		_fthrow_(EXCEP_GLOBAL_UNDEF, "undefined global G$%lu", x);
	}
	return unbox(__Glob_table__[x] );
}

inline void global_set(svm_t vm, ulong x, __arg descr v) {
	if (__Glob_table__[x] == NIHIL) {
		_fthrow_(EXCEP_GLOBAL_UNDEF, "undefined global G$%lu", x);
	}
	box_set(__Glob_table__[x], v);
}

inline void global_init(svm_t vm, ulong x, __arg descr v) {
	if (__Glob_table__[x] != NIHIL) {
		_fthrow_(EXCEP_DUP_GLOBAL, "redefined global G$%lu", x);
	}
	box_scp glob = new_box(vm, Gc_perm, NIHIL);
	__Glob_table__[x] = glob;
	box_set(glob, v);
}

inline __pure ulong global_reserve(svm_t vm) {
	Globt_ptr++;
	if (Globt_ptr == Globt_lim) {
		// TODO grow global array;
		_mthrow_(EXCEP_INDEX_OUT_OF_RANGE, "global table can't grow for now");
	}
	return Globt_ptr - 1;
}

ulong global_add(svm_t vm, __arg descr val) {
	ulong res = global_reserve(vm);
	global_init(vm, res, val);
	return res;
}

void global_bind(svm_t vm, const char* name, descr val) {
	decl_gc_of_vm();
	SAVE_RSTK();
	__protect(symbol_d,symbol, symbol_from_string(vm, SS_ASCI(name)));
	env_put(vm, Envglob, symbol, desc_from_glob( global_add(vm, val)));
	UNWIND_RSTK();
}

inline env_scp new_env(svm_t vm, gc_t gc, __arg descr parent) {
	SAVE_RSTK();
	PUSH_RSTK(parent);
	env_scp result = (env_scp) inew_inst(gc, TYPEIX_ENVIRONMENT);
	result->cdr = parent;
	UNWIND_RSTK();
	return result;
}


#define envp_list(e) (cl_end(CAR(e)) || datum_is_carcdr(CAR(e)))

descr __pure env_lookup_d(svm_t vm, env_scp env, symbol_d sym, int *depth) {
	assert(env != NIHIL);
	*depth = -1;
	__loc env_scp p = env;
	__loc cons_scp q;
	descr result = NIHIL;
	lcyc_until_nihil(env_scp, p) {
		*depth = *depth + 1;
		descr container = p->car;
		if(cl_end(container)) continue;
		switch (datum_get_typeix(container)) {
			case TYPEIX_HASHTABLE:
				if (container == Symbhtb) {
					// Envglobal
					result = symbolobj(sym)->globix;
				} else {
					result = htb_get(vm, (hashtable_scp*) &container, &sym);
				}
				break;
			case TYPEIX_CONS:
				q = (cons_scp) container;
				lcyc_until_nihil(cons_scp, q) {
					if (CAR(CAR(q)) == sym) {
						result = CDR(CAR(q));
						break;
					}
				}
				break;
			}
			if (result)
				break;
		}
		return result;
}

descr __pure env_lookup(svm_t vm, env_scp env, symbol_d sym) {
	int depth = 0;
	return env_lookup_d(vm, env, sym, &depth);
}

	// EXCEP_DUP_GLOBAL;
void env_put(svm_t vm, __arg env_scp env, __arg symbol_d sym,
		__arg descr ref) {
	SAVE_RSTK();
	PUSH_RSTK3(env,sym,ref);
	decl_gc_of_vm();

	if (env == Envglob) {
		_mthrow_assert_(symbolobj(sym)->globix == NIHIL, EXCEP_DUP_GLOBAL, "global identifier redefinition!");
		symbolobj(sym)->globix = ref;
		htb_putpair(vm, gc, &Symbhtb, &sym);
	} else {

		__protect(descr, container,env->car);
		if(cl_end(container) || datum_is_carcdr(container)) {
			__loc descr tail =  CONS(CONS(sym, ref), container );
			FIELD_SET(env, car, tail);
		} else {
			switch (datum_get_typeix(container)) {
				case TYPEIX_HASHTABLE:
					htb_put(vm, gc, (hashtable_scp*)&container, &sym, &ref);
					break;
				default:
					assert(0 && "never here");
				}
		}


	}
	UNWIND_RSTK();
}

inline env_scp env_extend(svm_t vm, __arg env_scp env, __arg symbol_d id,
		__arg descr val) {
	decl_gc_of_vm();
	SAVE_RSTK();
	PUSH_RSTK(env);
	assert(envp_list(env));
	__protect(cons_scp,p, CONS(id, val));
	p = CONS(p, CAR(env));
	__protect(env_scp, res, new_env(vm, gc, CDR(env)));
	res->car = p;
	UNWIND_RSTK();
	return res;
}

// if tix < 0, tix will be calculated!,
// throw  EXCEP_DUP_TYPE_SYMBOL
type_scp __pure type_reserve(svm_t vm, int tix, const char *name,
		ulong category, ulong szobj, ulong szelem, ulong slot_offs,
		ulong slot_count) {
	assert(tix >= 1 && tix <= MAX_RESERVED_TYPE);
	assert("Array type has szelem" && imply(category&TYPE_CATEGORY_ARRAY, szelem != 0));
	assert("Nonterm type has slots" && imply(category&TYPE_CATEGORY_NONTERM, slot_offs != 0 &&
					imply(!(category&TYPE_CATEGORY_ARRAY), slot_count != 0)));
	type_scp t = (type_scp) inew_inst(Gc_perm, TYPEIX_TYPE );
	t->category = category;
	t->szobj = szobj;
	t->szelem = szelem;
	t->slot_offs = slot_offs;
	t->slot_count = slot_count;
	t->slot_count_offs = field_offs_scp(vec_scp, count);
	t->asci = name;
	t->tix = tix;
	__Glob_table__[tix] = t;
	type_init(t);
	return t;
}

	// symbols

descr bin_htbsym_hash(svm_t vm, uint argc, __binargv(argv)) {
	descr mbkey = argv[0];
	if (datum_is_instof_ix(mbkey, TYPEIX_WEAKBOX)) {
		mbkey = unboxif(mbkey);
	}
	if (datum_is_symbol(mbkey)) {
		mbkey = symbol_getname(mbkey);
	}
	return desc_from_fixnum(descr_hash_code(mbkey));
}

descr bin_htbsym_eq(svm_t vm, uint argc, __binargv(argv)) {
	descr mbkey = argv[0];
	symbol_d val = (symbol_d) unboxif(argv[1]);
	//	logf_debug(Sys_log, "== %i %i", datum_get_typeix(mbkey), datum_get_typeix(val));
	//	if(datum_is_instof_ix(mbkey, TYPEIX_WEAKBOX)) { mbkey = unboxif(mbkey); }
	if (datum_is_symbol(mbkey)) {
		mbkey = symbol_getname(mbkey);
	}
	return desc_cond(descr_eq(mbkey, (descr) symbol_getname(val)));
}

descr bin_symbol_finalize(svm_t vm, uint argc, __binargv(argv)) {
	assert(datum_is_instof_ix(argv[0], TYPEIX_SYMASSOC) && "symassc");
	//printf("here");
	htb_rem(vm, vm_gc(vm), &Symbhtb, &argv[0]);
	return SYMB_VOID;
}

symbol_d __pure symbol_get(svm_t vm, string_scp str) {
	descr d = htb_getpair(vm, &Symbhtb, _RS(str));
	return unboxif(d);
}

// symbol is not permanent use PUSH_ROOT! // create or get
symbol_d symbol_from_string(svm_t vm, __arg string_scp s) {
	decl_gc_of_vm();
	symbol_d sym = symbol_get(vm, s);
	if (sym) {
		return sym;
	}
	SAVE_RSTK();
	__protect(symasc_scp,result, new_symassoc(vm, gc, new_string_const(vm, gc, &s )));
	__protect(box_scp,box, new_wbox(vm, gc, result));
	htb_putpair(vm, gc, &Symbhtb, _RS(box));
	UNWIND_RSTK();
	return result;
}



inline void _symbol_reserve_ix(svm_t vm, __nongc string_scp str,
		__nongc symbol_d spec);

inline void symbol_reserve_asci(const char *asci, symbol_d sym) {
	decl_vm();
	string_scp s = new_string_asci(vm, Gc_perm, asci);
	_symbol_reserve_ix(vm, s, sym);
}

inline void symbol_reserve_uni(const UChar *uc, symbol_d sym) {
	decl_vm();
	string_scp s = new_string_uni(vm, Gc_perm, uc);
	_symbol_reserve_ix(vm, s, sym);
}

inline void _symbol_reserve_ix(svm_t vm, __nongc string_scp str,
		__nongc symbol_d spec) {
	decl_gc_of_vm();
	assert(desc_is_spec(spec));
	assert(datum_is_instof_ix(str, TYPEIX_STRING));
	ulong index = desc_to_spec(spec);
	assert(index <= MAX_RESERVED_SYM);
	assert(__Sym_table__[index] == 0);
	datum_set_const(str);
	__Sym_table__[index] = (descr) new_symassoc(vm, Gc_perm, str);

	htb_putpair(vm, gc, &Symbhtb, _RS(spec));
}

inline closure_scp new_bin(ulong argc, builtin_op_t bin) {
	__ret closure_scp cls = (closure_scp) inew_inst(Gc_perm, TYPEIX_CLOSURE);
	cls->category = CLOSR_CAT_BIN;
	cls->bin = bin;
	cls->argc = argc;
	cls->bits = 0;
	return cls;
}

inline closure_scp new_bin_multy(ulong argc, builtin_op_t bin) {
	__ret closure_scp cls = new_bin(argc, bin);
	cls->bits |= CLOSR_MULTI_ARGS;
	return cls;
}

