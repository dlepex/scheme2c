/* opcore.h
 * musc- scheme translator & interpreter
 * author: denis.lepekhin@gmail.com
 * created on: 2009
 */

#ifndef OPCORE_H_
#define OPCORE_H_

#include "vmdefs.h"


typedef struct HashTableDatum  		*hashtable_scp;

#define HTB_BITM_NO_REPUT  1

struct HashTableDatum {
	tag_t tag;
	ulong bits;
	/*slots*/
	vec_scp  b;
	closure_scp hashfun, eqfun;
};
#define _SLOTC_HTB 3

#define htb_set_no_reput(htb)     (htb)->bits |= HTB_BITM_NO_REPUT
#define htb_rowc(ht)  vec_len((ht)->b)

// list cycles
#define lcyc(lt, var) \
	for(; scp_valid(var); var = (lt) var->cdr)

#define lcyc_until_nihil(lt, var) \
	for(;var != NIHIL; var = (lt) var->cdr)

#define lcycprev(lt, var, prev) \
	for(; scp_valid(var); prev = var, var = (lt) var->cdr)

// symbols ops
symbol_d symbol_get(svm_t vm, __arg string_scp s); // throws EXCEP_SYMTABLE_OVERFLOW
symbol_d symbol_from_string(svm_t vm, __arg string_scp s);
inline void symbol_reserve_asci(const char *asci,  symbol_d sym);
inline void symbol_reserve_uni( const UChar *uc, symbol_d sym);

#define symbolobj(d)   (desc_is_scp(d) ? ((symasc_scp)(d))  : __Sym_table__[desc_to_spec(d)])

#define symbol_getname(s)    (symbolobj(s)->name)
// types
type_scp __pure type_reserve(svm_t vm, int tix, const char *name, ulong category,
		ulong szobj, ulong szelem, ulong slot_offs, ulong slot_count);

// globals ops

inline descr global_get(ulong x);

#define __global(x) 			(__Glob_table__[x])
#define global_defined(x)  (__Glob_table__[x]!=NIHIL)

inline void global_init(svm_t vm, ulong x, __arg descr v);

inline void global_set(svm_t vm,  ulong x, __arg descr v);
inline __pure ulong   global_reserve(svm_t vm);

ulong global_add(svm_t vm, __arg descr val);
void global_bind(svm_t vm, const char* name, descr val );
#define GLOB_BIND(_name, _val) global_bind(vm, _name, _val)


// constrctors

inline symasc_scp new_symassoc(svm_t vm, gc_t gc, __arg string_scp name);

string_scp new_string(svm_t vm, gc_t gc, ulong uclen);
string_scp new_string_asci(svm_t vm, gc_t gc, const char* c_str);
#define SS_ASCI(asci)  new_string_asci(vm,gc, asci)
string_scp new_string_uni(svm_t vm, gc_t gc, const UChar* ustr);
string_scp new_string_const(svm_t vm, gc_t gc,__sbp(string_scp) ps);

real_scp   	   new_real(  gc_t gc, double d);
cons_scp			new_cons(svm_t vm, gc_t gc, __arg descr car, __arg descr cdr);

#define CONS(_car,_cdr)  new_cons(vm, gc, _car, _cdr)
#define CONSCAR(_car)    new_cons(vm, gc, _car, NIHIL)
cons_scp new_cons2(svm_t vm, gc_t gc,__arg descr car1, __arg descr car2, __arg descr cdr2);
inline vec_scp   	   new_vec(   gc_t gc, ulong count);
inline buf_scp new_buf(gc_t gc, ulong sz);

/* vector routines */
void vec_fill(vec_scp vec, descr d);

/* string operations */
int       ss_hash_code(string_scp str);
void       ss_clear(string_scp str);
bool      ss_eq(string_scp str1, string_scp str2);
string_scp ss_set_asci(string_scp s,  const char* asci);
string_scp ss_set_u(string_scp s, const UChar* uni);
string_scp ss_setn_asci(string_scp s, ulong n, const char* asci);
void ss_get_asci(string_scp s, char* buf, int bufsz);
string_scp ss_setn_u(string_scp s, ulong n, const UChar* uni);
void ss_add(svm_t vm, gc_t gc,__sbp(string_scp) str, bigchar ch);
void ss_dec(string_scp str, ulong dec);

/* hash and equality */
int buf_hash_code(buf_scp b, int prev, ulong n);
int descr_hash_code(descr d);
bool descr_eq(descr d1, descr d2);
inline bool descr_eqrec(descr d1, descr d2);
inline bool id_eq( descr id1, descr id2);

// basic hash table (HTB) operations
hashtable_scp new_htb_g(svm_t vm, gc_t gc, __sbp(vec_scp) buckets,
		__sbp(closure_scp) hashfun, __sbp(closure_scp) eqfun );
hashtable_scp new_htb(svm_t vm, gc_t gc, ulong count, __sbp(closure_scp) hashfun, __sbp(closure_scp) eqfun);
hashtable_scp new_htbc(svm_t vm, gc_t gc, ulong count);
descr htb_getpair(svm_t vm, __sbp(hashtable_scp) h,__sbp(descr) key);
descr htb_get(svm_t vm, __sbp(hashtable_scp) h, __sbp(descr) key);
descr htb_rem(svm_t vm, gc_t gc, __sbp(hashtable_scp) h, __sbp(descr) key);
descr htb_putpair(svm_t vm, gc_t gc, __sbp(hashtable_scp) h, __sbp(descr) pair);
descr htb_put(svm_t vm, gc_t gc, __sbp(hashtable_scp) h, __sbp(descr) key, __sbp(descr) value);

arlist_scp new_arlist(svm_t vm, gc_t gc, ulong count);
void arlist_grow(svm_t vm, gc_t gc, __sbp(arlist_scp) al, float multiple);
inline ulong arlist_add(svm_t vm, gc_t gc, __sbp(arlist_scp) al, __sbp(descr) d);
inline void arlist_skip_at(arlist_scp al, ulong index);
inline void arlist_push(svm_t vm, gc_t gc, __arg arlist_scp al, __arg descr d );

inline closure_scp new_bin(ulong argc, builtin_op_t bin);
inline closure_scp new_bin_multy(ulong argc, builtin_op_t bin);
inline box_scp new_wbox(svm_t vm, gc_t gc, __arg descr val);
inline box_scp new_box(svm_t vm, gc_t gc, __arg descr boxed);
#define MAKEBOX(boxed)  new_box(vm,gc, boxed)


// environment ops
inline env_scp new_env(svm_t vm, gc_t gc, __arg descr parent);
inline env_scp env_extend(svm_t vm, env_scp parent, symbol_d id, descr val);
inline env_scp __pure env_global();
descr __pure env_lookup_d(svm_t vm, env_scp env, symbol_d sym, int *depth);
descr __pure env_lookup(svm_t vm, env_scp env, symbol_d sym);
void env_put(svm_t vm, __arg env_scp env, __arg symbol_d psym, __arg descr pref  );

inline vec_scp cl_to_vec(svm_t vm, gc_t gc, __sbp(cons_scp) l);

inline buf_scp cl_to_vu8(svm_t vm, gc_t gc, __sbp(cons_scp) l);
inline bool clp_proper(cons_scp cons);
inline long __pure cli_len(cons_scp cons);
inline cons_scp __pure cl_last(cons_scp cl);
inline cl_scp cl_clone(svm_t vm, __arg cons_scp cons);

cl_scp args_to_cl(svm_t vm, descr *args, ulong from, ulong lim);


// array stack datum

astk_scp astk_push(svm_t vm, __arg astk_scp stk, __arg descr val);
descr astk_pop(__arg astk_scp stk);
astk_scp new_astk(gc_t gc, ulong count);


#endif /* OPCORE_H_ */
